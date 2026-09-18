#include "StashController.h"

namespace Guit
{

StashController::StashController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

void StashController::refresh()
{
    m_entries = m_repository->stashList();
    emit stashChanged(m_entries);
}

void StashController::save(const QString &message, bool includeUntracked)
{
    handleResult(m_repository->stashPush(message, includeUntracked));
}

void StashController::apply(const QString &stashRef)
{
    handleResult(m_repository->stashApply(stashRef));
}

void StashController::pop(const QString &stashRef)
{
    handleResult(m_repository->stashPop(stashRef));
}

void StashController::drop(const QString &stashRef)
{
    handleResult(m_repository->stashDrop(stashRef));
}

void StashController::clear()
{
    handleResult(m_repository->stashClear());
}

void StashController::inspect(const QString &stashRef)
{
    emit stashDetails(stashRef, m_repository->stashShow(stashRef));
}

void StashController::handleResult(const OperationResult &result)
{
    refresh();
    m_repository->refreshHead();
    emit headChanged();
    if (result.ok)
        emit stashOperationDone(result.message, result.command);
    else
        emit operationFailed(result.message, {}, result.command);
}

} // namespace Guit
