#include "MergeController.h"

namespace Guit
{

MergeController::MergeController(GitRepository *repository, QObject *parent)
    : QObject(parent)
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
    handleResult(m_repository->mergeBranch(branch, noFastForward));
}

void MergeController::rebase(const QString &branch)
{
    handleResult(m_repository->rebaseOnto(branch));
}

void MergeController::reset(const QString &target, ResetMode mode)
{
    handleResult(m_repository->resetTo(target, mode));
}

void MergeController::revert(const QString &hash)
{
    handleResult(m_repository->revertCommit(hash));
}

void MergeController::cherryPick(const QString &hash)
{
    handleResult(m_repository->cherryPick(hash));
}

void MergeController::resolveOurs(const QString &path)
{
    handleResult(m_repository->resolveWithOurs(path));
}

void MergeController::resolveTheirs(const QString &path)
{
    handleResult(m_repository->resolveWithTheirs(path));
}

void MergeController::continueOperation()
{
    refreshState();
    OperationResult result;
    switch (m_state.operation) {
    case PendingOperation::Merging:
        result = m_repository->mergeContinue();
        break;
    case PendingOperation::Rebasing:
        result = m_repository->rebaseContinue();
        break;
    case PendingOperation::CherryPicking:
        result = m_repository->cherryPickContinue();
        break;
    case PendingOperation::Reverting:
        result = m_repository->revertContinue();
        break;
    case PendingOperation::None:
        return;
    }
    handleResult(result);
}

void MergeController::skipOperation()
{
    // Only rebase supports --skip; the UI enables Skip during rebases only.
    refreshState();
    if (m_state.operation != PendingOperation::Rebasing)
        return;
    handleResult(m_repository->rebaseSkip());
}

void MergeController::abortOperation()
{
    refreshState();
    OperationResult result;
    switch (m_state.operation) {
    case PendingOperation::Merging:
        result = m_repository->mergeAbort();
        break;
    case PendingOperation::Rebasing:
        result = m_repository->rebaseAbort();
        break;
    case PendingOperation::CherryPicking:
        result = m_repository->cherryPickAbort();
        break;
    case PendingOperation::Reverting:
        result = m_repository->revertAbort();
        break;
    case PendingOperation::None:
        return;
    }
    handleResult(result);
}

void MergeController::handleResult(const OperationResult &result)
{
    m_repository->refreshHead();
    emit headChanged();
    refreshState();
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
}

} // namespace Guit
