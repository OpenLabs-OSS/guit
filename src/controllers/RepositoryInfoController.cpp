#include "RepositoryInfoController.h"

namespace Guit
{

RepositoryInfoController::RepositoryInfoController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

void RepositoryInfoController::refresh()
{
    emit infoChanged(m_repository->repositoryInfo(), m_repository->lfsInfo(), m_repository->submodules(),
                     m_repository->worktrees(), m_repository->reflog(), m_repository->readGitignore());
}

void RepositoryInfoController::updateSubmodules(bool initialize)
{
    handleResult(m_repository->submoduleUpdate(initialize));
}

void RepositoryInfoController::syncSubmodules()
{
    handleResult(m_repository->submoduleSync());
}

void RepositoryInfoController::addWorktree(const QString &path, const QString &source, bool newBranch)
{
    handleResult(m_repository->worktreeAdd(path, source, newBranch));
}

void RepositoryInfoController::removeWorktree(const QString &path, bool force)
{
    handleResult(m_repository->worktreeRemove(path, force));
}

void RepositoryInfoController::pruneWorktrees()
{
    handleResult(m_repository->worktreePrune());
}

void RepositoryInfoController::trackLfs(const QString &pattern)
{
    handleResult(m_repository->lfsTrack(pattern));
}

void RepositoryInfoController::saveGitignore(const QString &content)
{
    handleResult(m_repository->writeGitignore(content));
}

void RepositoryInfoController::handleResult(const OperationResult &result)
{
    refresh();
    m_repository->refreshHead();
    emit headChanged();
    if (result.ok)
        emit operationDone(result.message, result.command);
    else
        emit operationFailed(result.message, {}, result.command);
}

} // namespace Guit
