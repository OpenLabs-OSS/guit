#include "MainWindow.h"

#include "PlaceholderPage.h"
#include "../app/AppSettings.h"
#include "../app/ThemeManager.h"
#include "../controllers/RepositoryController.h"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>

namespace Guit
{

MainWindow::MainWindow(RepositoryController *controller, AppSettings *settings, ThemeManager *themes, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_settings(settings)
    , m_themes(themes)
    , m_sidebar(new Sidebar(this))
    , m_stack(new QStackedWidget(this))
    , m_overview(new OverviewPage(this))
{
    setWindowTitle(tr("Guit"));
    resize(1100, 700);

    if (!m_settings->windowGeometry().isEmpty())
        restoreGeometry(m_settings->windowGeometry());

    m_stack->addWidget(m_overview); // index 0 == Sidebar::Page::Overview
    m_stack->addWidget(new PlaceholderPage(Sidebar::pageTitle(Sidebar::Page::Changes),
                                           Sidebar::pageExplanation(Sidebar::Page::Changes),
                                           QStringLiteral("git status --porcelain=v1"),
                                           tr("Staging workflow arrives in Milestone 2."), this));
    m_stack->addWidget(new PlaceholderPage(Sidebar::pageTitle(Sidebar::Page::History),
                                           Sidebar::pageExplanation(Sidebar::Page::History),
                                           QStringLiteral("git log --format=..."),
                                           tr("Commit history arrives in Milestone 2."), this));
    m_stack->addWidget(new PlaceholderPage(Sidebar::pageTitle(Sidebar::Page::Branches),
                                           Sidebar::pageExplanation(Sidebar::Page::Branches),
                                           QStringLiteral("git for-each-ref refs/heads refs/remotes"),
                                           tr("Branch management arrives in Milestone 2."), this));
    m_stack->addWidget(new PlaceholderPage(Sidebar::pageTitle(Sidebar::Page::Tags),
                                           Sidebar::pageExplanation(Sidebar::Page::Tags),
                                           QStringLiteral("git tag --list"),
                                           tr("Tag management arrives in Milestone 3."), this));
    m_stack->addWidget(new PlaceholderPage(Sidebar::pageTitle(Sidebar::Page::Stashes),
                                           Sidebar::pageExplanation(Sidebar::Page::Stashes),
                                           QStringLiteral("git stash list"),
                                           tr("Stash management arrives in Milestone 3."), this));
    m_stack->addWidget(new PlaceholderPage(Sidebar::pageTitle(Sidebar::Page::Remotes),
                                           Sidebar::pageExplanation(Sidebar::Page::Remotes),
                                           QStringLiteral("git remote -v · git fetch origin"),
                                           tr("Remote operations arrive in Milestone 3."), this));

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

    auto *refreshAction = new QAction(tr("&Refresh"), this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, m_controller, &RepositoryController::refresh);
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
}

void MainWindow::onRepositoryClosed()
{
    refreshOverview();
    updateStatusBar();
}

void MainWindow::onOpenFailed(const QString &reason, const QString &details)
{
    showError(reason, details);
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

void MainWindow::onPageSelected(Sidebar::Page page)
{
    m_stack->setCurrentIndex(static_cast<int>(page));
}

void MainWindow::onThemeAction(ThemeManager::Theme theme)
{
    m_themes->setTheme(theme);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Guit"),
                       tr("Guit — a beginner-friendly Git client that shows what Git is doing.\n\nMilestone 1 foundation build."));
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

} // namespace Guit
