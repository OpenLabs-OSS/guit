#include "RepositoryInfoController.h"

namespace Guit
{

RepositoryInfoController::RepositoryInfoController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

void RepositoryInfoController::refresh()
{
    GitRepository *repository = m_repository;
    submit<InfoBundle>(
        [repository]() {
            InfoBundle bundle;
            bundle.info = repository->repositoryInfo();
            bundle.lfs = repository->lfsInfo();
            bundle.submodules = repository->submodules();
            bundle.worktrees = repository->worktrees();
            bundle.reflog = repository->reflog();
            bundle.gitignore = repository->readGitignore();
            return bundle;
        },
        [this](const InfoBundle &bundle) {
            emit infoChanged(bundle.info, bundle.lfs, bundle.submodules, bundle.worktrees, bundle.reflog,
                             bundle.gitignore);
            emit loadingChanged(false);
        });
}

void RepositoryInfoController::updateSubmodules(bool initialize)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, initialize]() { return repository->submoduleUpdate(initialize); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::syncSubmodules()
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository]() { return repository->submoduleSync(); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::addWorktree(const QString &path, const QString &source, bool newBranch)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, path, source, newBranch]() {
        return repository->worktreeAdd(path, source, newBranch);
    },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::removeWorktree(const QString &path, bool force)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, path, force]() { return repository->worktreeRemove(path, force); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::pruneWorktrees()
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository]() { return repository->worktreePrune(); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::trackLfs(const QString &pattern)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, pattern]() { return repository->lfsTrack(pattern); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::saveGitignore(const QString &content)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, content]() { return repository->writeGitignore(content); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void RepositoryInfoController::reloadAfter(const OperationResult &result)
{
    if (!result.ok) {
        emit loadingChanged(false);
        emit operationFailed(result.message, {}, result.command);
        return;
    }
    refresh();
    emit headChanged();
    emit operationDone(result.message, result.command);
}

} // namespace Guit
