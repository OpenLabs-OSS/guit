#include "RemoteController.h"

namespace Guit
{

RemoteController::RemoteController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
    connect(m_repository, &GitRepository::networkProgress, this, &RemoteController::networkProgress);
    connect(m_repository, &GitRepository::networkStarted, this, &RemoteController::networkStarted);
    connect(m_repository, &GitRepository::networkFinished, this, [this](const OperationResult &result) {
        m_repository->refreshHead();
        emit headChanged();
        refresh();
        if (result.ok)
            emit remoteOperationDone(result.message, result.command);
        emit networkFinished(result);
        if (!result.ok)
            emit operationFailed(result.message, {}, result.command);
    });
}

void RemoteController::refresh()
{
    GitRepository *repository = m_repository;
    submit<QList<RemoteInfo>>([repository]() { return repository->remotes(); },
                              [this](const QList<RemoteInfo> &remotes) {
                                  m_remotes = remotes;
                                  emit remotesChanged(m_remotes);
                                  emit loadingChanged(false);
                              });
}

void RemoteController::add(const QString &name, const QString &url)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name, url]() { return repository->addRemote(name, url); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RemoteController::remove(const QString &name)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name]() { return repository->removeRemote(name); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RemoteController::rename(const QString &oldName, const QString &newName)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, oldName, newName]() { return repository->renameRemote(oldName, newName); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RemoteController::setUrl(const QString &name, const QString &url)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name, url]() { return repository->setRemoteUrl(name, url); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RemoteController::fetch(const QString &remote, bool prune)
{
    m_repository->startFetch(remote, prune);
}

void RemoteController::pull(const QString &remote)
{
    m_repository->startPull(remote);
}

void RemoteController::push(const QString &remote, const QString &source, bool setUpstream, bool force)
{
    m_repository->startPush(remote, source, setUpstream, force, false);
}

void RemoteController::pushTag(const QString &remote, const QString &tag)
{
    m_repository->startPush(remote, tag, false, false, false);
}

void RemoteController::deleteRemoteBranch(const QString &remote, const QString &branch)
{
    m_repository->startPush(remote, branch, false, false, true);
}

void RemoteController::cancelNetwork()
{
    m_repository->cancelNetworkOperation();
}

void RemoteController::reloadAfter(const OperationResult &result)
{
    if (!result.ok) {
        emit loadingChanged(false);
        emit operationFailed(result.message, {}, result.command);
        return;
    }
    GitRepository *repository = m_repository;
    submit<QList<RemoteInfo>>([repository]() { return repository->remotes(); },
                              [this, result](const QList<RemoteInfo> &remotes) {
                                  m_remotes = remotes;
                                  emit remotesChanged(m_remotes);
                                  emit remoteOperationDone(result.message, result.command);
                                  emit loadingChanged(false);
                              });
}

} // namespace Guit
