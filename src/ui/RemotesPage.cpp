#include "RemotesPage.h"

#include "PushDialog.h"
#include "RemoteDialog.h"
#include "../controllers/BranchController.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

namespace Guit
{

RemotesPage::RemotesPage(RemoteController *controller, BranchController *branches, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_branches(branches)
    , m_remoteList(new QListWidget(this))
    , m_detailLabel(new QLabel(this))
    , m_commandLabel(new QLabel(this))
    , m_pruneBox(new QCheckBox(tr("Prune deleted remote branches on fetch"), this))
    , m_fetchButton(new QPushButton(tr("Fetch"), this))
    , m_pullButton(new QPushButton(tr("Pull"), this))
    , m_pushButton(new QPushButton(tr("Push…"), this))
    , m_deleteButton(new QPushButton(tr("Delete Remote Branch…"), this))
{
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setStyleSheet(QStringLiteral("font-family: Consolas, monospace;"));
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    m_pruneBox->setToolTip(tr("Remove local remote-tracking branches whose upstream branch is gone (git fetch --prune)."));

    auto *addButton = new QPushButton(tr("Add…"), this);
    auto *editButton = new QPushButton(tr("Edit URL…"), this);
    auto *removeButton = new QPushButton(tr("Remove…"), this);
    removeButton->setToolTip(tr("Forget this remote. Local branches and history are kept."));
    auto *renameButton = new QPushButton(tr("Rename…"), this);
    m_fetchButton->setToolTip(tr("Download commits and tags without changing local branches (git fetch)."));
    m_pullButton->setToolTip(tr("Fetch and integrate into the current branch (git pull)."));
    m_pushButton->setToolTip(tr("Send local commits to the remote (git push)."));
    auto *refreshButton = new QPushButton(tr("Refresh"), this);

    auto *configRow = new QHBoxLayout();
    configRow->addWidget(addButton);
    configRow->addWidget(editButton);
    configRow->addWidget(removeButton);
    configRow->addWidget(renameButton);
    configRow->addStretch(1);

    auto *networkRow = new QHBoxLayout();
    networkRow->addWidget(m_fetchButton);
    networkRow->addWidget(m_pullButton);
    networkRow->addWidget(m_pushButton);
    networkRow->addWidget(m_deleteButton);
    networkRow->addWidget(refreshButton);
    networkRow->addStretch(1);

    auto *leftPane = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(m_remoteList, 1);
    leftLayout->addLayout(configRow);

    auto *rightPane = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_detailLabel);
    rightLayout->addWidget(m_pruneBox);
    rightLayout->addLayout(networkRow);
    rightLayout->addWidget(m_commandLabel);
    rightLayout->addStretch(1);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setSizes({300, 700});

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    connect(m_controller, &RemoteController::remotesChanged, this, &RemotesPage::onRemotesChanged);
    connect(m_controller, &RemoteController::remoteOperationDone, this,
            [this](const QString &, const QString &command) { m_commandLabel->setText(tr("Git: %1").arg(command)); });
    connect(m_remoteList, &QListWidget::itemSelectionChanged, this, &RemotesPage::updateDetails);
    connect(addButton, &QPushButton::clicked, this, &RemotesPage::onAdd);
    connect(editButton, &QPushButton::clicked, this, &RemotesPage::onEdit);
    connect(removeButton, &QPushButton::clicked, this, &RemotesPage::onRemove);
    connect(renameButton, &QPushButton::clicked, this, &RemotesPage::onRename);
    connect(m_fetchButton, &QPushButton::clicked, this, &RemotesPage::onFetch);
    connect(m_pullButton, &QPushButton::clicked, this, &RemotesPage::onPull);
    connect(m_pushButton, &QPushButton::clicked, this, &RemotesPage::onPush);
    connect(m_deleteButton, &QPushButton::clicked, this, &RemotesPage::onDeleteRemoteBranch);
    connect(refreshButton, &QPushButton::clicked, this, &RemotesPage::refresh);
    connect(m_controller, &RemoteController::networkStarted, this, [this](const QString &) { setNetworkActive(true); });
    connect(m_controller, &RemoteController::networkFinished, this, [this](const OperationResult &) { setNetworkActive(false); });
}

void RemotesPage::setNetworkActive(bool active)
{
    // One network operation at a time: prevent overlapping fetch/pull/push.
    m_fetchButton->setEnabled(!active);
    m_pullButton->setEnabled(!active);
    m_pushButton->setEnabled(!active);
    m_deleteButton->setEnabled(!active);
}

void RemotesPage::refresh()
{
    m_controller->refresh();
}

QString RemotesPage::selectedRemote() const
{
    const QList<QListWidgetItem *> selected = m_remoteList->selectedItems();
    if (selected.isEmpty())
        return {};
    return selected.constFirst()->data(Qt::UserRole).toString();
}

QStringList RemotesPage::localBranchNames() const
{
    QStringList names;
    for (const BranchInfo &branch : m_branches->branches()) {
        if (branch.isLocal())
            names.append(branch.name);
    }
    return names;
}

void RemotesPage::onRemotesChanged(const QList<RemoteInfo> &remotes)
{
    m_remotes = remotes;
    const QString previous = selectedRemote();
    m_remoteList->clear();
    if (remotes.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No remotes configured. Add one to fetch, pull, or push."), m_remoteList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_detailLabel->clear();
        return;
    }
    for (const RemoteInfo &remote : remotes) {
        auto *item = new QListWidgetItem(remote.name, m_remoteList);
        item->setData(Qt::UserRole, remote.name);
        item->setToolTip(remote.fetchUrl);
        if (remote.name == previous)
            item->setSelected(true);
    }
    if (m_remoteList->selectedItems().isEmpty())
        m_remoteList->setCurrentRow(0);
    updateDetails();
}

void RemotesPage::updateDetails()
{
    const QString current = selectedRemote();
    for (const RemoteInfo &remote : m_remotes) {
        if (remote.name == current) {
            m_detailLabel->setText(tr("Fetch: %1\nPush: %2").arg(remote.fetchUrl, remote.pushUrl));
            return;
        }
    }
    m_detailLabel->clear();
}

void RemotesPage::onAdd()
{
    RemoteDialog dialog(RemoteDialog::Mode::Add, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_controller->add(dialog.remoteName(), dialog.remoteUrl());
}

void RemotesPage::onEdit()
{
    const QString name = selectedRemote();
    if (name.isEmpty())
        return;
    RemoteDialog dialog(RemoteDialog::Mode::Edit, this);
    dialog.setRemoteName(name);
    dialog.setNameEditable(false);
    for (const RemoteInfo &remote : m_remotes) {
        if (remote.name == name) {
            dialog.setRemoteUrl(remote.fetchUrl);
            break;
        }
    }
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_controller->setUrl(name, dialog.remoteUrl());
}

void RemotesPage::onRemove()
{
    const QString name = selectedRemote();
    if (name.isEmpty())
        return;
    QMessageBox confirm(QMessageBox::Question, tr("Remove remote"),
                        tr("Forget remote “%1”?").arg(name),
                        QMessageBox::No | QMessageBox::Yes, this);
    confirm.setInformativeText(tr("Local branches, tags, and history are kept — only the remote entry is removed."));
    confirm.button(QMessageBox::Yes)->setText(tr("Remove"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_controller->remove(name);
}

void RemotesPage::onRename()
{
    const QString name = selectedRemote();
    if (name.isEmpty())
        return;
    const QString newName = QInputDialog::getText(this, tr("Rename Remote"),
                                                  tr("New name for remote “%1”:").arg(name),
                                                  QLineEdit::Normal, name)
                                .trimmed();
    if (newName.isEmpty() || newName == name)
        return;
    m_controller->rename(name, newName);
}

void RemotesPage::onFetch()
{
    const QString name = selectedRemote();
    if (name.isEmpty())
        return;
    m_controller->fetch(name, m_pruneBox->isChecked());
    m_commandLabel->setText(tr("Git: git fetch %1%2")
                                .arg(m_pruneBox->isChecked() ? QStringLiteral("--prune ") : QString(), name));
}

void RemotesPage::onPull()
{
    const QString name = selectedRemote();
    if (name.isEmpty())
        return;
    m_controller->pull(name);
    m_commandLabel->setText(tr("Git: git pull %1").arg(name));
}

void RemotesPage::onPush()
{
    QStringList remoteNames;
    for (const RemoteInfo &remote : m_remotes)
        remoteNames.append(remote.name);
    if (remoteNames.isEmpty())
        return;
    PushDialog dialog(PushDialog::Mode::Push, remoteNames, localBranchNames(), this);
    dialog.setCurrentRemote(selectedRemote());
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (dialog.force()) {
        QMessageBox confirm(QMessageBox::Warning, tr("Force push"),
                            tr("Force-push %1 to “%2”?").arg(dialog.source(), dialog.remote()),
                            QMessageBox::Cancel | QMessageBox::Yes, this);
        confirm.setInformativeText(tr("Uses --force-with-lease: aborts if the remote has unseen work, but can still "
                                      "discard commits others pushed. Prefer a merge or rebase when collaborating."));
        confirm.button(QMessageBox::Yes)->setText(tr("Force Push"));
        if (confirm.exec() != QMessageBox::Yes)
            return;
    }
    m_controller->push(dialog.remote(), dialog.source(), dialog.setUpstream(), dialog.force());
    m_commandLabel->setText(tr("Git: pushing %1 to %2…").arg(dialog.source(), dialog.remote()));
}

void RemotesPage::onDeleteRemoteBranch()
{
    QStringList remoteNames;
    for (const RemoteInfo &remote : m_remotes)
        remoteNames.append(remote.name);
    if (remoteNames.isEmpty())
        return;
    PushDialog dialog(PushDialog::Mode::DeleteRemoteBranch, remoteNames, {}, this);
    dialog.setCurrentRemote(selectedRemote());
    if (dialog.exec() != QDialog::Accepted)
        return;
    QMessageBox confirm(QMessageBox::Warning, tr("Delete remote branch"),
                        tr("Delete “%1” from remote “%2”?").arg(dialog.source(), dialog.remote()),
                        QMessageBox::Cancel | QMessageBox::Yes, this);
    confirm.setInformativeText(tr("Collaborators tracking this branch will keep their local copies."));
    confirm.button(QMessageBox::Yes)->setText(tr("Delete"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_controller->deleteRemoteBranch(dialog.remote(), dialog.source());
}

} // namespace Guit
