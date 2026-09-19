#include "MergeController.h"

namespace Guit
{

MergeController::MergeController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

void MergeController::refreshState()
{
    m_state = m_repository->operationState();
    emit stateChanged(m_state);
}

void MergeController::merge(const QString &branch, bool noFastForward)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, branch, noFastForward]() {
        return repository->mergeBranch(branch, noFastForward);
    },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::rebase(const QString &branch)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, branch]() { return repository->rebaseOnto(branch); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::reset(const QString &target, ResetMode mode)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, target, mode]() { return repository->resetTo(target, mode); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::revert(const QString &hash)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, hash]() { return repository->revertCommit(hash); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::cherryPick(const QString &hash)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, hash]() { return repository->cherryPick(hash); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::resolveOurs(const QString &path)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, path]() { return repository->resolveWithOurs(path); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::resolveTheirs(const QString &path)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, path]() { return repository->resolveWithTheirs(path); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::continueOperation()
{
    const OperationState state = m_repository->operationState();
    GitRepository *repository = m_repository;
    switch (state.operation) {
    case PendingOperation::Merging:
        submit<OperationResult>([repository]() { return repository->mergeContinue(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::Rebasing:
        submit<OperationResult>([repository]() { return repository->rebaseContinue(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::CherryPicking:
        submit<OperationResult>([repository]() { return repository->cherryPickContinue(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::Reverting:
        submit<OperationResult>([repository]() { return repository->revertContinue(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::None:
        break;
    }
}

void MergeController::skipOperation()
{
    // Only rebase supports --skip; the UI enables Skip during rebases only.
    if (m_repository->operationState().operation != PendingOperation::Rebasing)
        return;
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository]() { return repository->rebaseSkip(); },
                            [this](const OperationResult &result) { handleResult(result); });
}

void MergeController::abortOperation()
{
    const OperationState state = m_repository->operationState();
    GitRepository *repository = m_repository;
    switch (state.operation) {
    case PendingOperation::Merging:
        submit<OperationResult>([repository]() { return repository->mergeAbort(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::Rebasing:
        submit<OperationResult>([repository]() { return repository->rebaseAbort(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::CherryPicking:
        submit<OperationResult>([repository]() { return repository->cherryPickAbort(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::Reverting:
        submit<OperationResult>([repository]() { return repository->revertAbort(); },
                                [this](const OperationResult &result) { handleResult(result); });
        break;
    case PendingOperation::None:
        break;
    }
}

void MergeController::handleResult(const OperationResult &result)
{
    GitRepository *repository = m_repository;
    submit<HeadInfo>(
        [repository]() {
            repository->refreshHead();
            return repository->head();
        },
        [this, result](const HeadInfo &) {
            emit headChanged();
            refreshState();
            emit loadingChanged(false);
            if (result.ok) {
                emit operationDone(result.message, result.command);
                return;
            }
            // Conflict stops route to resolution instead of the error box.
            if (result.conflict) {
                emit conflictStarted(result.message, result.command);
                return;
            }
            emit operationFailed(result.message, {}, result.command);
        });
}

} // namespace Guit
