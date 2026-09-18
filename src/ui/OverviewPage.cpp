#include "Theme.h"
#include "OverviewPage.h"

#include "GitignoreDialog.h"
#include "WorktreeDialog.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QMessageBox>

namespace Guit
{

OverviewPage::OverviewPage(RepositoryInfoController *info, AppSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_info(info)
    , m_settings(settings)
    , m_gitBanner(new QLabel(this))
    , m_repoTitle(new QLabel(this))
    , m_repoDetails(new QLabel(this))
    , m_commandLabel(new QLabel(this))
    , m_openButton(new QPushButton(tr("Open Repository…"), this))
    , m_terminalButton(new QPushButton(tr("Open Terminal"), this))
    , m_gitignoreButton(new QPushButton(tr(".gitignore…"), this))
    , m_recentList(new QListWidget(this))
    , m_recentLabel(new QLabel(tr("Recent repositories"), this))
    , m_factsBox(new QGroupBox(tr("Repository information"), this))
    , m_factsLabel(new QLabel(m_factsBox))
    , m_lfsBox(new QGroupBox(tr("Git LFS"), this))
    , m_lfsLabel(new QLabel(m_lfsBox))
    , m_lfsPattern(new QLineEdit(m_lfsBox))
    , m_submoduleBox(new QGroupBox(tr("Submodules"), this))
    , m_submoduleList(new QListWidget(m_submoduleBox))
    , m_worktreeBox(new QGroupBox(tr("Worktrees"), this))
    , m_worktreeList(new QListWidget(m_worktreeBox))
    , m_reflogBox(new QGroupBox(tr("Reflog (advanced)"), this))
    , m_reflogList(new QListWidget(m_reflogBox))
{
    m_gitBanner->setWordWrap(true);
    m_repoTitle->setWordWrap(true);
    m_repoDetails->setWordWrap(true);
    m_repoDetails->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_factsLabel->setWordWrap(true);
    m_factsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lfsLabel->setWordWrap(true);
    m_commandLabel->setWordWrap(true);
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    Theme::applyMono(m_commandLabel);
    m_reflogList->setFont(Theme::monoFont());

    Theme::applyTitle(m_repoTitle);
    Theme::applySection(m_recentLabel);

    m_terminalButton->setToolTip(tr("Open this repository in your terminal."));
    m_gitignoreButton->setToolTip(tr("Edit .gitignore with common-pattern presets."));
    m_lfsPattern->setPlaceholderText(tr("Pattern to track, e.g. *.psd"));

    auto *openRow = new QHBoxLayout();
    openRow->addWidget(m_openButton);
    openRow->addWidget(m_terminalButton);
    openRow->addWidget(m_gitignoreButton);
    openRow->addStretch(1);

    auto *factsLayout = new QVBoxLayout(m_factsBox);
    factsLayout->addWidget(m_factsLabel);

    auto *lfsTrackButton = new QPushButton(tr("Track"), m_lfsBox);
    lfsTrackButton->setToolTip(tr("Route matching files through Git LFS (git lfs track)."));
    auto *lfsRow = new QHBoxLayout();
    lfsRow->addWidget(m_lfsPattern, 1);
    lfsRow->addWidget(lfsTrackButton);
    auto *lfsLayout = new QVBoxLayout(m_lfsBox);
    lfsLayout->addWidget(m_lfsLabel);
    lfsLayout->addLayout(lfsRow);

    auto *subUpdateButton = new QPushButton(tr("Update"), m_submoduleBox);
    subUpdateButton->setToolTip(tr("Fetch and check out submodule commits (git submodule update --init)."));
    auto *subSyncButton = new QPushButton(tr("Sync"), m_submoduleBox);
    subSyncButton->setToolTip(tr("Copy changed remote URLs into submodules (git submodule sync)."));
    auto *subRow = new QHBoxLayout();
    subRow->addWidget(subUpdateButton);
    subRow->addWidget(subSyncButton);
    subRow->addStretch(1);
    auto *subLayout = new QVBoxLayout(m_submoduleBox);
    subLayout->addWidget(m_submoduleList);
    subLayout->addLayout(subRow);

    auto *wtAddButton = new QPushButton(tr("Add…"), m_worktreeBox);
    auto *wtRemoveButton = new QPushButton(tr("Remove…"), m_worktreeBox);
    auto *wtRow = new QHBoxLayout();
    wtRow->addWidget(wtAddButton);
    wtRow->addWidget(wtRemoveButton);
    wtRow->addStretch(1);
    auto *wtLayout = new QVBoxLayout(m_worktreeBox);
    wtLayout->addWidget(m_worktreeList);
    wtLayout->addLayout(wtRow);

    auto *reflogLayout = new QVBoxLayout(m_reflogBox);
    auto *reflogHint = new QLabel(tr("The reflog records where HEAD pointed — your safety net for undoing mistakes."), m_reflogBox);
    reflogHint->setWordWrap(true);
    reflogLayout->addWidget(reflogHint);
    reflogLayout->addWidget(m_reflogList);

    auto *scrollContent = new QWidget(this);
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(Theme::pageMargin(), Theme::sectionSpacing(), Theme::pageMargin(), Theme::pageMargin());
    scrollLayout->setSpacing(Theme::sectionSpacing());
    scrollLayout->addWidget(m_gitBanner);
    scrollLayout->addWidget(m_repoTitle);
    scrollLayout->addWidget(m_repoDetails);
    scrollLayout->addWidget(m_commandLabel);
    scrollLayout->addLayout(openRow);
    scrollLayout->addWidget(m_factsBox);
    scrollLayout->addWidget(m_lfsBox);
    scrollLayout->addWidget(m_submoduleBox);
    scrollLayout->addWidget(m_worktreeBox);
    scrollLayout->addWidget(m_reflogBox);
    scrollLayout->addWidget(m_recentLabel);
    scrollLayout->addWidget(m_recentList, 1);
    scrollLayout->addStretch(1);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(scrollContent);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(scroll);

    connect(m_openButton, &QPushButton::clicked, this, &OverviewPage::openRequested);
    connect(m_terminalButton, &QPushButton::clicked, this, &OverviewPage::terminalRequested);
    connect(m_gitignoreButton, &QPushButton::clicked, this, &OverviewPage::onOpenGitignore);
    connect(m_recentList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        if (item != nullptr)
            emit recentOpened(item->data(Qt::UserRole).toString());
    });
    connect(m_info, &RepositoryInfoController::infoChanged, this, &OverviewPage::onInfoChanged);
    connect(m_info, &RepositoryInfoController::operationDone, this,
            [this](const QString &, const QString &command) { m_commandLabel->setText(tr("Git: %1").arg(command)); });
    connect(lfsTrackButton, &QPushButton::clicked, this, &OverviewPage::onTrackLfs);
    connect(subUpdateButton, &QPushButton::clicked, this, &OverviewPage::onUpdateSubmodules);
    connect(subSyncButton, &QPushButton::clicked, this, &OverviewPage::onSyncSubmodules);
    connect(wtAddButton, &QPushButton::clicked, this, &OverviewPage::onAddWorktree);
    connect(wtRemoveButton, &QPushButton::clicked, this, &OverviewPage::onRemoveWorktree);

    applyMode();
}

void OverviewPage::setData(const Data &data)
{
    const bool gitMissing = !data.gitAvailable;
    m_gitBanner->setProperty("danger", gitMissing);
    Theme::repolish(m_gitBanner);
    if (gitMissing) {
        m_gitBanner->setText(tr("Git was not found. Install Git and make sure it is on PATH, then restart Guit."));
    } else {
        m_gitBanner->setText(tr("Git %1 ready.").arg(data.gitVersion));
    }

    if (!data.hasRepository) {
        m_repoTitle->setText(tr("No repository open"));
        m_repoDetails->setText(tr("Open a Git repository to inspect it. Guit uses your installed Git — nothing is hidden from you."));
        m_commandLabel->setText(tr("Git: git rev-parse --show-toplevel"));
    } else {
        m_repoTitle->setText(data.rootPath);
        m_repoDetails->setText(tr("Branch: %1\nHEAD: %2%3")
                                   .arg(data.branchDisplay, data.headDisplay,
                                        data.commitHash.isEmpty() ? QString()
                                                                  : QStringLiteral("\nCommit: ") + data.commitHash));
        m_commandLabel->setText(tr("Git: git branch --show-current · git rev-parse --verify HEAD"));
    }
    m_terminalButton->setEnabled(data.hasRepository);
    m_gitignoreButton->setEnabled(data.hasRepository);

    m_recentList->clear();
    for (const QString &recent : data.recentRepositories) {
        auto *item = new QListWidgetItem(recent, m_recentList);
        item->setData(Qt::UserRole, recent);
        item->setToolTip(tr("Double-click to open this repository."));
    }
    const bool hasRecents = !data.recentRepositories.isEmpty();
    m_recentLabel->setVisible(hasRecents);
    m_recentList->setVisible(hasRecents);

    m_factsBox->setVisible(data.hasRepository);
    applyMode();
    if (data.hasRepository)
        refresh();
}

void OverviewPage::refresh()
{
    m_info->refresh();
}

void OverviewPage::applyMode()
{
    // Beginner mode hides lower-level machinery; advanced mode reveals it.
    const bool advanced = m_settings->advancedMode();
    m_lfsBox->setVisible(advanced);
    m_submoduleBox->setVisible(advanced);
    m_worktreeBox->setVisible(advanced);
    m_reflogBox->setVisible(advanced);
}

QString OverviewPage::humanSize(qlonglong bytes)
{
    if (bytes < 0)
        return tr("unknown");
    if (bytes < 1024)
        return tr("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return tr("%1 KiB").arg(bytes / 1024);
    if (bytes < 1024LL * 1024 * 1024)
        return tr("%1 MiB").arg(bytes / (1024 * 1024));
    return tr("%1 GiB").arg(bytes / (1024LL * 1024 * 1024));
}

void OverviewPage::onInfoChanged(const RepoInfo &info, const LfsInfo &lfs,
                                 const QList<SubmoduleInfo> &submodules,
                                 const QList<WorktreeInfo> &worktrees,
                                 const QList<ReflogEntry> &reflog,
                                 const QString &gitignore)
{
    if (!info.valid)
        return;
    m_factsLabel->setText(tr("Root: %1\nGit dir: %2 (%3)%4\nCommits: %5 · Remotes: %6 · Branches: %7 · Tags: %8 · Stashes: %9")
                              .arg(info.rootPath, info.gitDir, humanSize(info.gitDirSizeBytes),
                                   info.isBare ? QStringLiteral(" · bare") : QString(),
                                   info.commitCount < 0 ? tr("unknown") : QString::number(info.commitCount))
                              .arg(info.remoteCount)
                              .arg(info.branchCount)
                              .arg(info.tagCount)
                              .arg(info.stashCount));

    if (!lfs.available)
        m_lfsLabel->setText(tr("Git LFS is not installed. Large-file features are unavailable."));
    else if (!lfs.enabledInRepo)
        m_lfsLabel->setText(tr("Git LFS %1 installed, but this repository does not use it.").arg(lfs.version));
    else
        m_lfsLabel->setText(tr("Git LFS %1 · %2 tracked file(s).").arg(lfs.version).arg(lfs.trackedFiles));

    m_submoduleList->clear();
    if (submodules.isEmpty()) {
        auto *item = new QListWidgetItem(tr("(no submodules)"), m_submoduleList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
    for (const SubmoduleInfo &module : submodules) {
        const QString state = !module.initialized() ? tr("not initialized")
            : module.inSync()                       ? tr("in sync")
                                                    : tr("checked out at a different commit");
        auto *item = new QListWidgetItem(QStringLiteral("%1 (%2)").arg(module.path, state), m_submoduleList);
        item->setToolTip(module.hash);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }

    m_worktreeList->clear();
    for (const WorktreeInfo &tree : worktrees) {
        QString label = tree.path;
        if (tree.isMain)
            label += tr(" (main)");
        label += QStringLiteral(" [%1]").arg(tree.branch);
        if (tree.locked)
            label += tr(" (locked)");
        auto *item = new QListWidgetItem(label, m_worktreeList);
        item->setData(Qt::UserRole, tree.path);
        item->setToolTip(tree.hash);
    }

    m_reflogList->clear();
    for (const ReflogEntry &entry : reflog) {
        const QString date = entry.date.isValid() ? entry.date.toString(QStringLiteral("yyyy-MM-dd hh:mm")) + QStringLiteral(" ") : QString();
        auto *item = new QListWidgetItem(QStringLiteral("%1 %2 %3%4").arg(entry.ref, entry.hash.left(7), date, entry.message),
                                         m_reflogList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setToolTip(entry.hash);
    }
    if (reflog.isEmpty()) {
        auto *item = new QListWidgetItem(tr("(reflog is empty)"), m_reflogList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }

    m_gitignoreContent = gitignore;
    applyMode();
}

void OverviewPage::onOpenGitignore()
{
    GitignoreDialog dialog(m_info, m_gitignoreContent, this);
    dialog.exec();
    refresh();
}

void OverviewPage::onAddWorktree()
{
    WorktreeDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_info->addWorktree(dialog.path(), dialog.source(), dialog.newBranch());
}

void OverviewPage::onRemoveWorktree()
{
    const QList<QListWidgetItem *> selected = m_worktreeList->selectedItems();
    if (selected.isEmpty())
        return;
    const QString path = selected.constFirst()->data(Qt::UserRole).toString();
    QMessageBox confirm(QMessageBox::Warning, tr("Remove worktree"),
                        tr("Remove the worktree at %1? Uncommitted changes there are lost.").arg(path),
                        QMessageBox::NoButton, this);
    auto *removeButton = confirm.addButton(tr("Remove"), QMessageBox::AcceptRole);
    auto *forceButton = confirm.addButton(tr("Force Remove"), QMessageBox::DestructiveRole);
    confirm.addButton(QMessageBox::Cancel);
    confirm.exec();
    if (confirm.clickedButton() == removeButton)
        m_info->removeWorktree(path, false);
    else if (confirm.clickedButton() == forceButton)
        m_info->removeWorktree(path, true);
}

void OverviewPage::onTrackLfs()
{
    if (!m_lfsPattern->text().trimmed().isEmpty()) {
        m_info->trackLfs(m_lfsPattern->text());
        m_lfsPattern->clear();
    }
}

void OverviewPage::onUpdateSubmodules()
{
    m_info->updateSubmodules(true);
}

void OverviewPage::onSyncSubmodules()
{
    m_info->syncSubmodules();
}

} // namespace Guit
