#include "RemoteController.h"

namespace Guit
{

RemoteController::RemoteController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
    connect(m_repository, &GitRepository::networkProgress, this, &RemoteController::networkProgress);
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
    m_remotes = m_repository->remotes();
    emit remotesChanged(m_remotes);
}

void RemoteController::add(const QString &name, const QString &url)
{
    handleResult(m_repository->addRemote(name, url));
}

void RemoteController::remove(const QString &name)
{
    handleResult(m_repository->removeRemote(name));
}

void RemoteController::rename(const QString &oldName, const QString &newName)
{
    handleResult(m_repository->renameRemote(oldName, newName));
}

void RemoteController::setUrl(const QString &name, const QString &url)
{
    handleResult(m_repository->setRemoteUrl(name, url));
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

void RemoteController::handleResult(const OperationResult &result)
{
    refresh();
    if (result.ok)
        emit remoteOperationDone(result.message, result.command);
    else
        emit operationFailed(result.message, {}, result.command);
}

} // namespace Guit
