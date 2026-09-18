#include "Theme.h"
#include "ChangesPage.h"

#include "../git/GitModels.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QShortcut>
#include <QVBoxLayout>

namespace Guit
{

ChangesPage::ChangesPage(ChangesController *controller, MergeController *merge, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_merge(merge)
    , m_unstagedList(new QTreeWidget(this))
    , m_stagedList(new QTreeWidget(this))
    , m_unstagedLabel(new QLabel(this))
    , m_stagedLabel(new QLabel(this))
    , m_diff(new DiffViewer(this))
    , m_subject(new QLineEdit(this))
    , m_body(new QTextEdit(this))
    , m_amend(new QCheckBox(tr("Amend previous commit"), this))
    , m_commitButton(new QPushButton(tr("Commit"), this))
    , m_commandLabel(new QLabel(this))
{
    m_subject->setPlaceholderText(tr("Commit message — a short summary of the change"));
    m_subject->setToolTip(tr("Required. Good messages explain what changed and why."));
    m_body->setPlaceholderText(tr("Extended description (optional)"));
    m_body->setMaximumHeight(80);
    m_amend->setToolTip(tr("Fold the staged changes into the previous commit instead of creating a new one."));
    m_commandLabel->setWordWrap(true);
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    Theme::applyMono(m_commandLabel);

    m_commitButton->setProperty("primary", true);
    Theme::applySection(m_unstagedLabel);
    Theme::applySection(m_stagedLabel);
    for (QTreeWidget *list : {m_unstagedList, m_stagedList}) {
        list->setColumnCount(2);
        list->setHeaderLabels({tr("Status"), tr("File")});
        list->setHeaderHidden(true);
        list->setRootIsDecorated(false);
        list->setSelectionMode(QAbstractItemView::ExtendedSelection);
        list->setAlternatingRowColors(true);
        list->setSortingEnabled(false);
        list->header()->setStretchLastSection(true);
        list->header()->resizeSection(0, 92);
    }

    auto *stageButton = new QPushButton(tr("Stage"), this);
    stageButton->setToolTip(tr("Move the selected changes into the staging area (git add)."));
    auto *discardButton = new QPushButton(tr("Discard…"), this);
    discardButton->setProperty("destructive", true);
    discardButton->setToolTip(tr("Permanently throw away the selected working-tree changes."));
    auto *unstageButton = new QPushButton(tr("Unstage"), this);
    unstageButton->setToolTip(tr("Move the selected changes back out of the staging area (git restore --staged)."));
    auto *stageAllButton = new QPushButton(tr("Stage All"), this);
    auto *unstageAllButton = new QPushButton(tr("Unstage All"), this);
    auto *refreshButton = new QPushButton(tr("Refresh"), this);

    auto *topBar = new QHBoxLayout();
    topBar->addWidget(refreshButton);
    topBar->addWidget(stageAllButton);
    topBar->addWidget(unstageAllButton);
    topBar->addStretch(1);

    // Conflict lifecycle bar: visible only while a merge/rebase/
    // cherry-pick/revert is stopped at conflicts. Guides the user through
    // resolve -> stage -> continue, or abort.
    m_conflictLabel = new QLabel(this);
    m_conflictLabel->setWordWrap(true);
    auto *oursButton = new QPushButton(tr("Use Ours"), this);
    oursButton->setToolTip(tr("Resolve the selected file with your side (git checkout --ours)."));
    auto *theirsButton = new QPushButton(tr("Use Theirs"), this);
    theirsButton->setToolTip(tr("Resolve the selected file with their side (git checkout --theirs)."));
    auto *continueButton = new QPushButton(tr("Continue"), this);
    continueButton->setToolTip(tr("Stage resolved files, then continue the operation."));
    m_skipButton = new QPushButton(tr("Skip"), this);
    m_skipButton->setToolTip(tr("Skip the current commit (rebase only)."));
    auto *abortButton = new QPushButton(tr("Abort"), this);
    abortButton->setToolTip(tr("Give up and restore the pre-operation state."));
    auto *conflictButtons = new QHBoxLayout();
    conflictButtons->addWidget(oursButton);
    conflictButtons->addWidget(theirsButton);
    conflictButtons->addWidget(continueButton);
    conflictButtons->addWidget(m_skipButton);
    conflictButtons->addWidget(abortButton);
    conflictButtons->addStretch(1);
    auto *conflictLayout = new QVBoxLayout();
    conflictLayout->addWidget(m_conflictLabel);
    conflictLayout->addLayout(conflictButtons);
    m_conflictBar = new QWidget(this);
    m_conflictBar->setObjectName(QStringLiteral("ConflictBar"));
    m_conflictBar->setLayout(conflictLayout);
    m_conflictBar->setVisible(false);

    auto *unstagedButtons = new QHBoxLayout();
    unstagedButtons->addWidget(stageButton);
    unstagedButtons->addWidget(discardButton);
    unstagedButtons->addStretch(1);

    auto *unstagedBox = new QVBoxLayout();
    unstagedBox->addWidget(m_unstagedLabel);
    unstagedBox->addWidget(m_unstagedList, 1);
    unstagedBox->addLayout(unstagedButtons);

    auto *stagedButtons = new QHBoxLayout();
    stagedButtons->addWidget(unstageButton);
    stagedButtons->addStretch(1);

    auto *stagedBox = new QVBoxLayout();
    stagedBox->addWidget(m_stagedLabel);
    stagedBox->addWidget(m_stagedList, 1);
    stagedBox->addLayout(stagedButtons);

    auto *listsSplitter = new QSplitter(Qt::Vertical, this);
    auto *unstagedPane = new QWidget(this);
    unstagedPane->setLayout(unstagedBox);
    auto *stagedPane = new QWidget(this);
    stagedPane->setLayout(stagedBox);
    listsSplitter->addWidget(unstagedPane);
    listsSplitter->addWidget(stagedPane);

    auto *commitBox = new QVBoxLayout();
    commitBox->addWidget(m_subject);
    commitBox->addWidget(m_body);
    commitBox->addWidget(m_amend);
    commitBox->addWidget(m_commitButton);
    commitBox->addWidget(m_commandLabel);
    auto *commitPane = new QWidget(this);
    commitPane->setLayout(commitBox);

    auto *rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(m_diff);
    rightSplitter->addWidget(commitPane);
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 0);

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(listsSplitter);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({300, 700});

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::pageMargin(), Theme::sectionSpacing(), Theme::pageMargin(), Theme::pageMargin());
    layout->setSpacing(Theme::controlSpacing());
    layout->addWidget(m_conflictBar);
    layout->addLayout(topBar);
    layout->addWidget(mainSplitter, 1);

    connect(m_controller, &ChangesController::statusChanged, this, &ChangesPage::onStatusChanged);
    connect(m_controller, &ChangesController::diffLoaded, this, &ChangesPage::onDiffLoaded);
    connect(m_unstagedList, &QTreeWidget::itemSelectionChanged, this, &ChangesPage::onUnstagedSelection);
    connect(m_stagedList, &QTreeWidget::itemSelectionChanged, this, &ChangesPage::onStagedSelection);
    connect(stageButton, &QPushButton::clicked, this, &ChangesPage::onStage);
    connect(unstageButton, &QPushButton::clicked, this, &ChangesPage::onUnstage);
    connect(stageAllButton, &QPushButton::clicked, this, &ChangesPage::onStageAll);
    connect(unstageAllButton, &QPushButton::clicked, this, &ChangesPage::onUnstageAll);
    connect(discardButton, &QPushButton::clicked, this, &ChangesPage::onDiscard);
    connect(refreshButton, &QPushButton::clicked, this, &ChangesPage::refresh);
    connect(m_commitButton, &QPushButton::clicked, this, &ChangesPage::onCommit);
    connect(m_controller, &ChangesController::committed, this, [this](const QString &, const QString &command, const QString &) {
        m_subject->clear();
        m_body->clear();
        m_amend->setChecked(false);
        m_commandLabel->setText(tr("Git: %1").arg(command));
    });
    connect(m_controller, &ChangesController::staged, this, [this](const QString &, const QString &command) {
        m_commandLabel->setText(tr("Git: %1").arg(command));
    });
    connect(m_merge, &MergeController::stateChanged, this, &ChangesPage::onConflictState);
    connect(m_merge, &MergeController::operationDone, this,
            [this](const QString &, const QString &command) { m_commandLabel->setText(tr("Git: %1").arg(command)); });
    connect(oursButton, &QPushButton::clicked, this, &ChangesPage::onUseOurs);
    connect(theirsButton, &QPushButton::clicked, this, &ChangesPage::onUseTheirs);
    connect(continueButton, &QPushButton::clicked, this, &ChangesPage::onContinue);
    connect(m_skipButton, &QPushButton::clicked, this, &ChangesPage::onSkip);
    connect(abortButton, &QPushButton::clicked, this, &ChangesPage::onAbort);
    auto *commitShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), this);
    commitShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(commitShortcut, &QShortcut::activated, this, &ChangesPage::onCommit);
}

void ChangesPage::refresh()
{
    m_controller->refresh();
}

void ChangesPage::refreshConflicts()
{
    m_merge->refreshState();
}

void ChangesPage::onConflictState(const OperationState &state)
{
    m_conflictBar->setVisible(state.isActive());
    if (!state.isActive())
        return;
    m_skipButton->setVisible(state.operation == PendingOperation::Rebasing);
    m_conflictLabel->setText(tr("%1 in progress.\nResolve each conflicted file (edit it, or pick Ours/Theirs), "
                                "stage the result, then Continue. Abort restores everything.")
                                 .arg(pendingOperationLabel(state.operation)));
}

void ChangesPage::onUseOurs()
{
    const QStringList paths = selectedPaths(m_unstagedList);
    if (!paths.isEmpty())
        m_merge->resolveOurs(paths.constFirst());
}

void ChangesPage::onUseTheirs()
{
    const QStringList paths = selectedPaths(m_unstagedList);
    if (!paths.isEmpty())
        m_merge->resolveTheirs(paths.constFirst());
}

void ChangesPage::onContinue()
{
    m_merge->continueOperation();
}

void ChangesPage::onSkip()
{
    m_merge->skipOperation();
}

void ChangesPage::onAbort()
{
    QMessageBox confirm(QMessageBox::Question, tr("Abort operation"),
                        tr("Abort the in-progress operation and restore the previous state?"),
                        QMessageBox::No | QMessageBox::Yes, this);
    confirm.button(QMessageBox::Yes)->setText(tr("Abort"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_merge->abortOperation();
}

QPair<QString, QColor> ChangesPage::statusBadge(const FileStatusEntry &entry, bool stagedSide)
{
    const ThemeSpec &spec = Theme::currentSpec();
    if (entry.isConflicted())
        return {QStringLiteral("CONFLICT"), spec.danger};
    const FileState state = stagedSide ? entry.stagedState : entry.worktreeState;
    switch (state) {
    case FileState::Modified:  return {QStringLiteral("M"), spec.warning};
    case FileState::Added:      return {QStringLiteral("A"), spec.success};
    case FileState::Deleted:    return {QStringLiteral("D"), spec.danger};
    case FileState::Renamed:    return {QStringLiteral("R"), spec.accent};
    case FileState::Copied:     return {QStringLiteral("C"), spec.accent};
    case FileState::Untracked:  return {QStringLiteral("??"), spec.mutedText};
    case FileState::Ignored:    return {QStringLiteral("!"), spec.mutedText};
    case FileState::Unmodified:
    case FileState::Conflicted: break;
    }
    return {QStringLiteral("M"), spec.warning};
}

void ChangesPage::fillList(QTreeWidget *list, const QList<FileStatusEntry> &entries, bool stagedSide)
{
    list->clear();
    QFont badgeFont = list->font();
    badgeFont.setWeight(QFont::DemiBold);
    for (int i = 0; i < entries.size(); ++i) {
        const FileStatusEntry &entry = entries.at(i);
        const auto badge = statusBadge(entry, stagedSide);
        QString path = entry.path;
        if (!entry.originalPath.isEmpty())
            path = QStringLiteral("%1 → %2").arg(entry.originalPath, entry.path);
        auto *item = new QTreeWidgetItem(list, {badge.first, path});
        item->setData(0, Qt::UserRole, i);
        item->setForeground(0, badge.second);
        item->setFont(0, badgeFont);
        if (entry.isConflicted())
            item->setToolTip(0, tr("Conflicted — resolve it here (edit, Ours/Theirs), then stage."));
        else if (stagedSide)
            item->setToolTip(0, tr("In the staging area — will be included in the next commit."));
        else
            item->setToolTip(0, tr("Not staged — select and Stage to include it in the next commit."));
    }
}

void ChangesPage::onStatusChanged(const StatusSnapshot &snapshot)
{
    m_unstaged = snapshot.unstaged();
    m_staged = snapshot.staged();

    m_unstagedLabel->setText(tr("Unstaged (%1)").arg(m_unstaged.size()));
    m_stagedLabel->setText(tr("Staged (%1)").arg(m_staged.size()));

    fillList(m_unstagedList, m_unstaged, false);
    fillList(m_stagedList, m_staged, true);

    const bool hasChanges = !m_unstaged.isEmpty() || !m_staged.isEmpty();
    if (!hasChanges) {
        auto *item = new QTreeWidgetItem(m_unstagedList, {QString(), tr("Working tree clean — nothing to commit.")});
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_diff->clear();
    }
    m_commitButton->setEnabled(!m_staged.isEmpty());
}

void ChangesPage::onDiffLoaded(const QList<FileDiff> &diffs, ChangesController::DiffKind)
{
    m_diff->setDiffs(diffs);
}

void ChangesPage::onUnstagedSelection()
{
    const QList<QTreeWidgetItem *> selected = m_unstagedList->selectedItems();
    if (selected.isEmpty())
        return;
    m_stagedList->clearSelection();
    const int index = selected.constFirst()->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_unstaged.size())
        m_controller->loadDiff(m_unstaged.at(index).path, ChangesController::DiffKind::Unstaged);
}

void ChangesPage::onStagedSelection()
{
    const QList<QTreeWidgetItem *> selected = m_stagedList->selectedItems();
    if (selected.isEmpty())
        return;
    m_unstagedList->clearSelection();
    const int index = selected.constFirst()->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_staged.size())
        m_controller->loadDiff(m_staged.at(index).path, ChangesController::DiffKind::Staged);
}

QStringList ChangesPage::selectedPaths(QTreeWidget *list) const
{
    const QList<FileStatusEntry> &source = (list == m_stagedList) ? m_staged : m_unstaged;
    QStringList paths;
    for (QTreeWidgetItem *item : list->selectedItems()) {
        const int index = item->data(0, Qt::UserRole).toInt();
        if (index >= 0 && index < source.size())
            paths.append(source.at(index).path);
    }
    return paths;
}

QList<FileStatusEntry> ChangesPage::selectedEntries(QTreeWidget *list) const
{
    const QList<FileStatusEntry> &source = (list == m_stagedList) ? m_staged : m_unstaged;
    QList<FileStatusEntry> result;
    for (QTreeWidgetItem *item : list->selectedItems()) {
        const int index = item->data(0, Qt::UserRole).toInt();
        if (index >= 0 && index < source.size())
            result.append(source.at(index));
    }
    return result;
}

void ChangesPage::onStage()
{
    m_controller->stage(selectedPaths(m_unstagedList));
}

void ChangesPage::onUnstage()
{
    m_controller->unstage(selectedPaths(m_stagedList));
}

void ChangesPage::onStageAll()
{
    m_controller->stageAll();
}

void ChangesPage::onUnstageAll()
{
    m_controller->unstageAll();
}

void ChangesPage::onDiscard()
{
    // Discard is destructive: confirm explicitly and explain consequences.
    QList<FileStatusEntry> entries = selectedEntries(m_unstagedList);
    entries.append(selectedEntries(m_stagedList));
    if (entries.isEmpty())
        return;
    QStringList names;
    for (const FileStatusEntry &entry : entries)
        names.append(entry.path);
    QMessageBox confirm(QMessageBox::Warning, tr("Discard changes"),
                        tr("Permanently throw away the changes in %n file(s)?", nullptr, entries.size()),
                        QMessageBox::Discard | QMessageBox::Cancel, this);
    confirm.setInformativeText(tr("Tracked files are restored to HEAD; untracked files are deleted. "
                                  "This cannot be undone."));
    confirm.setDetailedText(names.join(QLatin1Char('\n')));
    confirm.button(QMessageBox::Discard)->setText(tr("Discard"));
    if (confirm.exec() != QMessageBox::Discard)
        return;
    m_controller->discard(entries);
}

void ChangesPage::onCommit()
{
    m_controller->commit(m_subject->text(), m_body->toPlainText(), m_amend->isChecked());
}

} // namespace Guit
