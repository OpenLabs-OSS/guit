#pragma once

#include "AsyncController.h"
#include "../git/AdvancedModels.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates remote configuration and fetch/pull/push. Local remote
// bookkeeping runs on the background queue; network transfers keep their
// dedicated async process with progress. Authentication is handled by
// Git's credential helpers — failures surface as messages.
class RemoteController : public AsyncController
{
    Q_OBJECT

public:
    explicit RemoteController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] QList<RemoteInfo> remotes() const { return m_remotes; }

public slots:
    void refresh();
    void add(const QString &name, const QString &url);
    void remove(const QString &name);
    void rename(const QString &oldName, const QString &newName);
    void setUrl(const QString &name, const QString &url);
    void fetch(const QString &remote, bool prune);
    void pull(const QString &remote);
    void push(const QString &remote, const QString &source, bool setUpstream, bool force);
    void pushTag(const QString &remote, const QString &tag);
    void deleteRemoteBranch(const QString &remote, const QString &branch);
    void cancelNetwork();

signals:
    void remotesChanged(const QList<Guit::RemoteInfo> &remotes);
    void remoteOperationDone(const QString &message, const QString &command);
    void networkStarted(const QString &command);
    void networkProgress(const QString &text);
    void networkFinished(const Guit::OperationResult &result);
    void operationFailed(const QString &reason, const QString &details, const QString &command);
    void headChanged();

private:
    void reloadAfter(const OperationResult &result);

    GitRepository *m_repository = nullptr;
    QList<RemoteInfo> m_remotes;
};

} // namespace Guit
