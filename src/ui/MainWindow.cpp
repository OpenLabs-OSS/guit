#include "MainWindow.h"

#include "CloneDialog.h"
#include "PlaceholderPage.h"
#include "../app/AppSettings.h"
#include "../controllers/RepositoryController.h"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSplitter>
#include <QStatusBar>

namespace Guit
{

MainWindow::MainWindow(RepositoryController *controller,
                       ChangesController *changes,
                       HistoryController *history,
                       BranchController *branches,
                       RemoteController *remotes,
                       TagController *tags,
                       StashController *stashes,
                       MergeController *merge,
                       AppSettings *settings,
                       ThemeManager *themes,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_changes(changes)
    , m_history(history)
    , m_branches(branches)
    , m_remotes(remotes)
    , m_tags(tags)
    , m_stashes(stashes)
    , m_merge(merge)
    , m_settings(settings)
    , m_themes(themes)
    , m_sidebar(new Sidebar(this))
    , m_stack(new QStackedWidget(this))
    , m_overview(new OverviewPage(this))
    , m_changesPage(new ChangesPage(changes, merge, this))
    , m_historyPage(new HistoryPage(history, merge, this))
    , m_branchesPage(new BranchesPage(branches, merge, this))
    , m_tagsPage(new TagsPage(tags, {}, this))
    , m_stashPage(new StashPage(stashes, this))
    , m_remotesPage(new RemotesPage(remotes, branches, this))
{
    setWindowTitle(tr("Guit"));
    resize(1100, 700);

    if (!m_settings->windowGeometry().isEmpty())
        restoreGeometry(m_settings->windowGeometry());

    // Stack order matches Sidebar::Page values.
    m_stack->addWidget(m_overview);
    m_stack->addWidget(m_changesPage);
    m_stack->addWidget(m_historyPage);
    m_stack->addWidget(m_branchesPage);
    m_stack->addWidget(m_tagsPage);
    m_stack->addWidget(m_stashPage);
    m_stack->addWidget(m_remotesPage);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 880});
    setCentralWidget(splitter);

    buildMenus();
    buildStatusBar();

    connect(m_sidebar, &Sidebar::pageSelected, this, &MainWindow::onPageSelected);
    connect(m_overview, &OverviewPage::openRequested, this, &MainWindow::onOpenRepository);
    connect(m_overview, &OverviewPage::recentOpened, this, &MainWindow::onOpenRecent);
    connect(m_controller, &RepositoryController::repositoryOpened, this, &MainWindow::onRepositoryOpened);
    connect(m_controller, &RepositoryController::repositoryClosed, this, &MainWindow::onRepositoryClosed);
    connect(m_controller, &RepositoryController::openFailed, this, &MainWindow::onOpenFailed);
    connect(m_controller, &RepositoryController::recentRepositoriesChanged, this, &MainWindow::onRecentChanged);
    connect(m_controller, &RepositoryController::notice, this, &MainWindow::showNotice);
    connect(m_controller, &RepositoryController::cloneProgress, this, &MainWindow::onCloneProgress);
    connect(m_controller, &RepositoryController::cloneFinished, this, &MainWindow::onCloneFinished);
    connect(m_controller, &RepositoryController::cloneFailed, this, &MainWindow::onCloneFailed);

    // Operation feedback: status-bar notices for success, error boxes with
    // Git details for failures. Branch errors are displayed by BranchesPage
    // itself (it may offer force-delete), so they are not duplicated here.
    connect(m_changes, &ChangesController::staged, this,
            [this](const QString &message, const QString &) { showNotice(message); });
    connect(m_changes, &ChangesController::committed, this,
            [this](const QString &message, const QString &, const QString &) { showNotice(message); });
    connect(m_changes, &ChangesController::operationFailed, this,
            [this](const QString &reason, const QString &details, const QString &) { showError(reason, details); });
    connect(m_changes, &ChangesController::headChanged, this, [this]() {
        refreshOverview();
        updateStatusBar();
    });
    connect(m_history, &HistoryController::operationFailed, this,
            [this](const QString &reason, const QString &details) { showError(reason, details); });
    connect(m_branches, &BranchController::branchOperationDone, this,
            [this](const QString &message, const QString &) { showNotice(message); });
    connect(m_branches, &BranchController::headChanged, this, [this]() {
        refreshOverview();
        updateStatusBar();
    });

    // Remotes: notices, errors, and async network progress with cancel.
    connect(m_remotes, &RemoteController::remoteOperationDone, this,
            [this](const QString &message, const QString &) { showNotice(message); });
    connect(m_remotes, &RemoteController::operationFailed, this,
            [this](const QString &reason, const QString &details, const QString &) { showError(reason, details); });
    connect(m_remotes, &RemoteController::networkProgress, this, &MainWindow::onNetworkProgress);
    connect(m_remotes, &RemoteController::networkFinished, this, &MainWindow::onNetworkFinished);
    connect(m_remotes, &RemoteController::headChanged, this, &MainWindow::onMergeHeadChanged);

    // Tags: notices and errors; pushes route to the network-owning RemoteController.
    connect(m_tags, &TagController::tagOperationDone, this,
            [this](const QString &message, const QString &) { showNotice(message); });
    connect(m_tags, &TagController::operationFailed, this,
            [this](const QString &reason, const QString &details, const QString &) { showError(reason, details); });
    connect(m_tags, &TagController::pushRequested, m_remotes, &RemoteController::pushTag);

    // Stash.
    connect(m_stashes, &StashController::stashOperationDone, this,
            [this](const QString &message, const QString &) { showNotice(message); });
    connect(m_stashes, &StashController::operationFailed, this,
            [this](const QString &reason, const QString &details, const QString &) { showError(reason, details); });
    connect(m_stashes, &StashController::headChanged, this, &MainWindow::onMergeHeadChanged);

    // Merge/rebase/reset/revert/cherry-pick: conflicts switch to the
    // Changes page with the resolution bar; everything refreshes HEAD state.
    connect(m_merge, &MergeController::operationDone, this,
            [this](const QString &message, const QString &) { showNotice(message); });
    connect(m_merge, &MergeController::operationFailed, this,
            [this](const QString &reason, const QString &details, const QString &) { showError(reason, details); });
    connect(m_merge, &MergeController::conflictStarted, this, &MainWindow::onMergeConflict);
    connect(m_merge, &MergeController::headChanged, this, &MainWindow::onMergeHeadChanged);

    refreshOverview();
    updateStatusBar();
    onRecentChanged();
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *openAction = new QAction(tr("&Open Repository…"), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenRepository);
    fileMenu->addAction(openAction);

    m_recentMenu = fileMenu->addMenu(tr("Open &Recent"));

    auto *cloneAction = new QAction(tr("&Clone Repository…"), this);
    connect(cloneAction, &QAction::triggered, this, &MainWindow::onCloneRepository);
    fileMenu->addAction(cloneAction);

    auto *initAction = new QAction(tr("&Initialize Repository…"), this);
    connect(initAction, &QAction::triggered, this, &MainWindow::onInitRepository);
    fileMenu->addAction(initAction);

    auto *closeAction = new QAction(tr("&Close Repository"), this);
    connect(closeAction, &QAction::triggered, this, &MainWindow::onCloseRepository);
    fileMenu->addAction(closeAction);

    auto *refreshAction = new QAction(tr("&Refresh"), this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshCurrentPage);
    fileMenu->addAction(refreshAction);

    fileMenu->addSeparator();
    auto *quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAction);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    auto *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    const struct {
        const char *label;
        ThemeManager::Theme theme;
    } themeActions[] = {
        {"&System", ThemeManager::Theme::System},
        {"&Light", ThemeManager::Theme::Light},
        {"&Dark", ThemeManager::Theme::Dark},
    };
    for (const auto &entry : themeActions) {
        auto *action = new QAction(tr(entry.label), this);
        action->setCheckable(true);
        action->setChecked(m_themes->current() == entry.theme);
        action->setData(static_cast<int>(entry.theme));
        themeGroup->addAction(action);
        viewMenu->addAction(action);
        connect(action, &QAction::triggered, this, [this, entry]() { onThemeAction(entry.theme); });
    }

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *aboutAction = new QAction(tr("&About Guit"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAction);
}

void MainWindow::buildStatusBar()
{
    m_repoLabel = new QLabel(tr("No repository"), statusBar());
    m_branchLabel = new QLabel(statusBar());
    m_gitLabel = new QLabel(statusBar());
    statusBar()->addWidget(m_repoLabel, 1);
    statusBar()->addPermanentWidget(m_branchLabel);
    statusBar()->addPermanentWidget(m_gitLabel);
}

void MainWindow::onOpenRepository()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Git Repository"));
    if (!dir.isEmpty())
        m_controller->openRepository(dir);
}

void MainWindow::onCloneRepository()
{
    CloneDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_cloneProgress = new QProgressDialog(tr("Cloning…"), tr("Cancel"), 0, 0, this);
    m_cloneProgress->setWindowTitle(tr("Clone Repository"));
    m_cloneProgress->setWindowModality(Qt::WindowModal);
    m_cloneProgress->setMinimumDuration(0);
    connect(m_cloneProgress, &QProgressDialog::canceled, m_controller, &RepositoryController::cancelClone);
    m_cloneProgress->show();
    m_controller->cloneRepository(dialog.sourceUrl(), dialog.targetDirectory());
}

void MainWindow::onInitRepository()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Directory to Initialize"));
    if (dir.isEmpty())
        return;
    const QString branch = QInputDialog::getText(this, tr("Initialize Repository"),
                                                 tr("Initial branch name (empty for Git default):"));
    m_controller->initRepository(dir, branch);
}

void MainWindow::onCloseRepository()
{
    m_controller->closeRepository();
}

void MainWindow::onOpenRecent(const QString &path)
{
    if (QDir(path).exists()) {
        m_controller->openRepository(path);
    } else {
        showError(tr("The directory no longer exists: %1").arg(path), {});
        m_controller->removeRecentRepository(path);
    }
}

void MainWindow::onRepositoryOpened(const QString &rootPath)
{
    statusBar()->showMessage(tr("Opened %1").arg(rootPath), 5000);
    refreshOverview();
    updateStatusBar();
    refreshCurrentPage();
}

void MainWindow::onRepositoryClosed()
{
    refreshOverview();
    updateStatusBar();
    refreshCurrentPage();
}

void MainWindow::onOpenFailed(const QString &reason, const QString &details)
{
    showError(reason, details);
}

void MainWindow::onCloneProgress(const QString &text)
{
    if (m_cloneProgress != nullptr)
        m_cloneProgress->setLabelText(text.trimmed().split(QLatin1Char('\n'), Qt::SkipEmptyParts).constLast());
}

void MainWindow::onCloneFinished(const QString &directory, const QString &command)
{
    if (m_cloneProgress != nullptr) {
        m_cloneProgress->close();
        m_cloneProgress->deleteLater();
        m_cloneProgress = nullptr;
    }
    statusBar()->showMessage(tr("Cloned into %1").arg(directory), 8000);
    Q_UNUSED(command);
}

void MainWindow::onCloneFailed(const QString &reason, const QString &details, const QString &command)
{
    if (m_cloneProgress != nullptr) {
        m_cloneProgress->close();
        m_cloneProgress->deleteLater();
        m_cloneProgress = nullptr;
    }
    const QString fullDetails = details.isEmpty() ? command : details + QStringLiteral("\n") + command;
    showError(reason, fullDetails);
}

void MainWindow::onRecentChanged()
{
    refreshOverview();
    m_recentMenu->clear();
    const QStringList recents = m_controller->recentRepositories();
    if (recents.isEmpty()) {
        auto *empty = m_recentMenu->addAction(tr("(No recent repositories)"));
        empty->setEnabled(false);
        return;
    }
    for (const QString &recent : recents) {
        auto *action = m_recentMenu->addAction(recent);
        connect(action, &QAction::triggered, this, [this, recent]() { onOpenRecent(recent); });
    }
    m_recentMenu->addSeparator();
    auto *clearAction = m_recentMenu->addAction(tr("Clear Recent"));
    connect(clearAction, &QAction::triggered, m_controller, &RepositoryController::clearRecentRepositories);
}

void MainWindow::showPage(Sidebar::Page page)
{
    m_sidebar->setCurrentPage(page);
}

void MainWindow::onPageSelected(Sidebar::Page page)
{
    m_stack->setCurrentIndex(static_cast<int>(page));
    refreshCurrentPage();
}

void MainWindow::onNetworkProgress(const QString &text)
{
    if (m_networkProgress == nullptr) {
        m_networkProgress = new QProgressDialog(tr("Working…"), tr("Cancel"), 0, 0, this);
        m_networkProgress->setWindowTitle(tr("Remote Operation"));
        m_networkProgress->setWindowModality(Qt::WindowModal);
        m_networkProgress->setMinimumDuration(0);
        connect(m_networkProgress, &QProgressDialog::canceled, m_remotes, &RemoteController::cancelNetwork);
        m_networkProgress->show();
    }
    const QString last = text.trimmed().split(QLatin1Char('\n'), Qt::SkipEmptyParts).constLast();
    m_networkProgress->setLabelText(last);
}

void MainWindow::onNetworkFinished(const OperationResult &result)
{
    closeNetworkProgress();
    if (result.ok) {
        statusBar()->showMessage(result.message, 8000);
    } else {
        showError(result.message, result.command);
    }
    refreshCurrentPage();
}

void MainWindow::closeNetworkProgress()
{
    if (m_networkProgress != nullptr) {
        m_networkProgress->close();
        m_networkProgress->deleteLater();
        m_networkProgress = nullptr;
    }
}

void MainWindow::onMergeConflict(const QString &message, const QString &)
{
    // Route to the Changes page where the conflict bar guides resolution.
    showPage(Sidebar::Page::Changes);
    m_changesPage->refreshConflicts();
    statusBar()->showMessage(message, 15000);
}

void MainWindow::onMergeHeadChanged()
{
    refreshOverview();
    updateStatusBar();
    refreshCurrentPage();
    m_changesPage->refreshConflicts();
}

void MainWindow::refreshCurrentPage()
{
    if (m_controller->repository()->isValid()) {
        switch (m_sidebar->currentPage()) {
        case Sidebar::Page::Changes:
            m_changesPage->refresh();
            m_changesPage->refreshConflicts();
            break;
        case Sidebar::Page::History:
            m_historyPage->refresh();
            break;
        case Sidebar::Page::Branches:
            m_branchesPage->refresh();
            break;
        case Sidebar::Page::Tags:
            m_tagsPage->refresh();
            break;
        case Sidebar::Page::Stashes:
            m_stashPage->refresh();
            break;
        case Sidebar::Page::Remotes: {
            m_branches->refresh();
            m_remotes->refresh();
            QStringList remoteNames;
            for (const RemoteInfo &remote : m_remotes->remotes())
                remoteNames.append(remote.name);
            m_tagsPage->setRemoteNames(remoteNames);
            m_remotesPage->refresh();
            break;
        }
        case Sidebar::Page::Overview:
            refreshOverview();
            break;
        }
    } else {
        refreshOverview();
    }
    updateStatusBar();
}

void MainWindow::onThemeAction(ThemeManager::Theme theme)
{
    m_themes->setTheme(theme);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Guit"),
                       tr("Guit — a beginner-friendly Git client that shows what Git is doing.\n\nMilestone 2 build: core Git workflows."));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings->setWindowGeometry(saveGeometry());
    QMainWindow::closeEvent(event);
}

void MainWindow::refreshOverview()
{
    OverviewPage::Data data;
    GitRepository *repository = m_controller->repository();
    data.gitAvailable = repository->client()->hasGit();
    if (data.gitAvailable) {
        data.gitVersion = repository->client()->version().toString();
        data.gitExecutable = repository->client()->gitExecutable();
    }
    data.hasRepository = repository->isValid();
    if (data.hasRepository) {
        const HeadInfo head = repository->head();
        data.rootPath = repository->rootPath();
        if (head.unborn) {
            data.branchDisplay = head.branch.isEmpty() ? tr("(no commits yet)") : head.branch;
            data.headDisplay = tr("unborn (no commits yet)");
        } else if (head.detached) {
            data.branchDisplay = tr("(detached HEAD)");
            data.headDisplay = tr("detached");
        } else {
            data.branchDisplay = head.branch;
            data.headDisplay = tr("on branch %1").arg(head.branch);
        }
        data.commitHash = head.commitHash;
    }
    data.recentRepositories = m_controller->recentRepositories();
    m_overview->setData(data);
}

void MainWindow::updateStatusBar()
{
    GitRepository *repository = m_controller->repository();
    if (!repository->isValid()) {
        m_repoLabel->setText(tr("No repository"));
        m_branchLabel->clear();
    } else {
        m_repoLabel->setText(repository->rootPath());
        const HeadInfo head = repository->head();
        if (head.unborn)
            m_branchLabel->setText(head.branch.isEmpty() ? tr("No commits yet") : head.branch + tr(" (no commits yet)"));
        else if (head.detached)
            m_branchLabel->setText(tr("Detached HEAD"));
        else
            m_branchLabel->setText(head.branch);
    }
    if (repository->client()->hasGit())
        m_gitLabel->setText(tr("Git %1").arg(repository->client()->version().toString()));
    else
        m_gitLabel->setText(tr("Git not found"));
}

void MainWindow::showError(const QString &reason, const QString &details)
{
    // User-facing message first; technical Git output available as details.
    QMessageBox message(QMessageBox::Warning, tr("Guit"), reason, QMessageBox::Ok, this);
    if (!details.isEmpty())
        message.setDetailedText(details);
    message.exec();
}

void MainWindow::showNotice(const QString &message)
{
    statusBar()->showMessage(message, 8000);
}

} // namespace Guit
