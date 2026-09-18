#pragma once

#include "../controllers/RemoteController.h"

#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

namespace Guit
{

class BranchController;

// Remote management page: configured remotes with fetch/push URLs, plus
// fetch (with prune), pull, push, and remote-branch deletion. Network
// work runs asynchronously with progress and cancel.
class RemotesPage : public QWidget
{
    Q_OBJECT

public:
    explicit RemotesPage(RemoteController *controller, BranchController *branches, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onRemotesChanged(const QList<RemoteInfo> &remotes);
    void onAdd();
    void onEdit();
    void onRemove();
    void onRename();
    void onFetch();
    void onPull();
    void onPush();
    void onDeleteRemoteBranch();
    void updateDetails();

private:
    QString selectedRemote() const;
    QStringList localBranchNames() const;

    RemoteController *m_controller = nullptr;
    BranchController *m_branches = nullptr;
    QListWidget *m_remoteList = nullptr;
    QLabel *m_detailLabel = nullptr;
    QLabel *m_commandLabel = nullptr;
    QCheckBox *m_pruneBox = nullptr;
    QList<RemoteInfo> m_remotes;
};

} // namespace Guit
