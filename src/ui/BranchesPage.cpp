#include "BranchesPage.h"

#include "BranchDialog.h"
#include "MergeDialog.h"
#include "RebaseDialog.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace Guit
{

BranchesPage::BranchesPage(BranchController *controller, MergeController *merge, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_merge(merge)
    , m_branchList(new QListWidget(this))
    , m_fromBox(new QComboBox(this))
    , m_toBox(new QComboBox(this))
    , m_compareLabel(new QLabel(this))
    , m_diff(new DiffViewer(this))
    , m_commandLabel(new QLabel(this))
{
    m_commandLabel->setStyleSheet(QStringLiteral("font-family: Consolas, monospace;"));
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    m_compareLabel->setWordWrap(true);

    auto *newButton = new QPushButton(tr("New…"), this);
    newButton->setToolTip(tr("Create a branch (git switch -c)."));
    auto *switchButton = new QPushButton(tr("Switch"), this);
    switchButton->setToolTip(tr("Move HEAD to the selected branch (git switch)."));
    auto *renameButton = new QPushButton(tr("Rename…"), this);
    auto *deleteButton = new QPushButton(tr("Delete…"), this);
    deleteButton->setToolTip(tr("Delete the selected branch. Deleting an unmerged branch requires confirmation."));
    auto *mergeButton = new QPushButton(tr("Merge…"), this);
    mergeButton->setToolTip(tr("Merge a branch into the current one (git merge)."));
    auto *rebaseButton = new QPushButton(tr("Rebase onto…"), this);
    rebaseButton->setToolTip(tr("Replay the current branch on top of another (git rebase). Rewrites history."));
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    auto *compareButton = new QPushButton(tr("Compare"), this);
    compareButton->setToolTip(tr("Show how far apart the two selected branches are, and their differences."));

    auto *actions = new QHBoxLayout();
    actions->addWidget(newButton);
    actions->addWidget(switchButton);
    actions->addWidget(renameButton);
    actions->addWidget(deleteButton);
    actions->addWidget(mergeButton);
    actions->addWidget(rebaseButton);
    actions->addWidget(refreshButton);
    actions->addStretch(1);

    auto *leftPane = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(m_branchList, 1);
    leftLayout->addLayout(actions);

    auto *compareRow = new QHBoxLayout();
    compareRow->addWidget(new QLabel(tr("From:"), this));
    compareRow->addWidget(m_fromBox, 1);
    compareRow->addWidget(new QLabel(tr("To:"), this));
    compareRow->addWidget(m_toBox, 1);
    compareRow->addWidget(compareButton);

    auto *rightPane = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(compareRow);
    rightLayout->addWidget(m_compareLabel);
    rightLayout->addWidget(m_diff, 1);
    rightLayout->addWidget(m_commandLabel);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({300, 700});

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    connect(m_controller, &BranchController::branchesChanged, this, &BranchesPage::onBranchesChanged);
    connect(m_controller, &BranchController::comparisonReady, this, &BranchesPage::onComparisonReady);
    connect(m_controller, &BranchController::branchOperationDone, this,
            [this](const QString &, const QString &command) { m_commandLabel->setText(tr("Git: %1").arg(command)); });
    connect(m_controller, &BranchController::operationFailed, this, &BranchesPage::onBranchFailed);
    connect(newButton, &QPushButton::clicked, this, &BranchesPage::onCreate);
    connect(switchButton, &QPushButton::clicked, this, &BranchesPage::onSwitch);
    connect(renameButton, &QPushButton::clicked, this, &BranchesPage::onRename);
    connect(deleteButton, &QPushButton::clicked, this, &BranchesPage::onDelete);
    connect(mergeButton, &QPushButton::clicked, this, &BranchesPage::onMerge);
    connect(rebaseButton, &QPushButton::clicked, this, &BranchesPage::onRebase);
    connect(refreshButton, &QPushButton::clicked, this, &BranchesPage::refresh);
    connect(compareButton, &QPushButton::clicked, this, &BranchesPage::onCompare);
    connect(m_branchList, &QListWidget::itemDoubleClicked, this, &BranchesPage::onSwitch);
}

void BranchesPage::refresh()
{
    m_controller->refresh();
}

QString BranchesPage::selectedBranch() const
{
    const QList<QListWidgetItem *> selected = m_branchList->selectedItems();
    if (selected.isEmpty())
        return {};
    return selected.constFirst()->data(Qt::UserRole).toString();
}

void BranchesPage::onBranchesChanged(const QList<BranchInfo> &branches)
{
    m_branches = branches;
    m_branchList->clear();
    if (branches.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No branches found."), m_branchList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
    QFont currentFont = m_branchList->font();
    currentFont.setBold(true);
    for (const BranchInfo &branch : branches) {
        QString label = branch.name;
        if (branch.isCurrent)
            label = QStringLiteral("● ") + label + tr(" (current)");
        else if (branch.isRemote)
            label += tr(" (remote)");
        else if (!branch.upstream.isEmpty())
            label += QStringLiteral(" → ") + branch.upstream;
        auto *item = new QListWidgetItem(label, m_branchList);
        item->setData(Qt::UserRole, branch.name);
        item->setToolTip(branch.commitHash);
        if (branch.isCurrent)
            item->setFont(currentFont);
    }

    const QString previousFrom = m_fromBox->currentText();
    const QString previousTo = m_toBox->currentText();
    m_fromBox->clear();
    m_toBox->clear();
    for (const BranchInfo &branch : branches) {
        if (branch.isLocal()) {
            m_fromBox->addItem(branch.name);
            m_toBox->addItem(branch.name);
        }
    }
    if (!previousFrom.isEmpty())
        m_fromBox->setCurrentText(previousFrom);
    if (!previousTo.isEmpty())
        m_toBox->setCurrentText(previousTo);
}

void BranchesPage::onComparisonReady(const QString &from, const QString &to,
                                     const AheadBehind &aheadBehind,
                                     const QList<FileDiff> &diffs)
{
    if (aheadBehind.valid) {
        m_compareLabel->setText(tr("“%1” is %2 ahead and %3 behind “%4”.\nGit: git rev-list --left-right --count %4...%5")
                                    .arg(to)
                                    .arg(aheadBehind.ahead)
                                    .arg(aheadBehind.behind)
                                    .arg(from, to));
    } else {
        m_compareLabel->setText(tr("Could not compare “%1” and “%2”.").arg(from, to));
    }
    m_diff->setDiffs(diffs);
}

void BranchesPage::onCreate()
{
    BranchDialog dialog(m_controller, BranchDialog::Mode::Create, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_controller->create(dialog.branchName(), dialog.startPoint(), dialog.checkout());
}

void BranchesPage::onSwitch()
{
    const QString branch = selectedBranch();
    if (!branch.isEmpty())
        m_controller->switchTo(branch);
}

void BranchesPage::onRename()
{
    const QString branch = selectedBranch();
    if (branch.isEmpty())
        return;
    BranchDialog dialog(m_controller, BranchDialog::Mode::Rename, this);
    dialog.setOldName(branch);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_controller->rename(branch, dialog.branchName());
}

void BranchesPage::onDelete()
{
    const QString branch = selectedBranch();
    if (branch.isEmpty())
        return;
    QMessageBox confirm(QMessageBox::Warning, tr("Delete branch"),
                        tr("Delete branch “%1”?").arg(branch),
                        QMessageBox::No | QMessageBox::Yes, this);
    confirm.setInformativeText(tr("The branch pointer is removed. Commits stay reachable through "
                                  "the reflog for a while, but treat this as permanent."));
    confirm.button(QMessageBox::Yes)->setText(tr("Delete"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_pendingForceDelete = branch;
    m_controller->remove(branch, false);
}

void BranchesPage::onBranchFailed(const QString &reason, const QString &details, const QString &command)
{
    // A safe delete refuses unmerged branches. Offer the explicit force
    // path (git branch -D) with its own confirmation instead of failing
    // silently — the user stays in control of the destructive option.
    // (Branch errors are displayed here; MainWindow does not duplicate them.)
    if (!m_pendingForceDelete.isEmpty() && reason.contains(QStringLiteral("not fully merged"))) {
        const QString branch = m_pendingForceDelete;
        m_pendingForceDelete.clear();
        QMessageBox forceBox(QMessageBox::Warning, tr("Branch is not fully merged"),
                             tr("“%1” contains commits that are not merged anywhere.").arg(branch),
                             QMessageBox::Cancel, this);
        forceBox.setInformativeText(tr("Force-deleting discards those commits from this branch. Continue?"));
        auto *forceButton = forceBox.addButton(tr("Force Delete"), QMessageBox::DestructiveRole);
        forceBox.exec();
        if (forceBox.clickedButton() == forceButton)
            m_controller->remove(branch, true);
        return;
    }
    m_pendingForceDelete.clear();
    QMessageBox message(QMessageBox::Warning, tr("Branch operation failed"), reason, QMessageBox::Ok, this);
    const QString fullDetails = details.isEmpty() ? command : details + QStringLiteral("\n") + command;
    if (!fullDetails.isEmpty())
        message.setDetailedText(fullDetails);
    message.exec();
}

void BranchesPage::onCompare()
{
    if (m_fromBox->currentText().isEmpty() || m_toBox->currentText().isEmpty())
        return;
    m_controller->compare(m_fromBox->currentText(), m_toBox->currentText());
}

void BranchesPage::onMerge()
{
    QStringList localBranches;
    QString current;
    for (const BranchInfo &branch : m_branches) {
        if (branch.isLocal())
            localBranches.append(branch.name);
        if (branch.isCurrent)
            current = branch.name;
    }
    MergeDialog dialog(localBranches, current, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_merge->merge(dialog.branch(), dialog.noFastForward());
}

void BranchesPage::onRebase()
{
    QStringList localBranches;
    QString current;
    for (const BranchInfo &branch : m_branches) {
        if (branch.isLocal())
            localBranches.append(branch.name);
        if (branch.isCurrent)
            current = branch.name;
    }
    RebaseDialog dialog(localBranches, current, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QMessageBox confirm(QMessageBox::Warning, tr("Rebase"),
                        tr("Replay “%1” onto “%2”?").arg(current, dialog.onto()),
                        QMessageBox::Cancel | QMessageBox::Yes, this);
    confirm.setInformativeText(tr("Rebase rewrites history. Only continue for commits you have not shared."));
    confirm.button(QMessageBox::Yes)->setText(tr("Rebase"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_merge->rebase(dialog.onto());
}

} // namespace Guit
