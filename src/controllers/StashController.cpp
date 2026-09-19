#include "StashController.h"

namespace Guit
{

StashController::StashController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

void StashController::refresh()
{
    GitRepository *repository = m_repository;
    submit<QList<StashInfo>>([repository]() { return repository->stashList(); },
                             [this](const QList<StashInfo> &entries) {
                                 m_entries = entries;
                                 emit stashChanged(m_entries);
                                 emit loadingChanged(false);
                             });
}

void StashController::save(const QString &message, bool includeUntracked)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, message, includeUntracked]() {
        return repository->stashPush(message, includeUntracked);
    },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void StashController::apply(const QString &stashRef)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, stashRef]() { return repository->stashApply(stashRef); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void StashController::pop(const QString &stashRef)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, stashRef]() { return repository->stashPop(stashRef); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void StashController::drop(const QString &stashRef)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, stashRef]() { return repository->stashDrop(stashRef); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void StashController::clear()
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository]() { return repository->stashClear(); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void StashController::inspect(const QString &stashRef)
{
    GitRepository *repository = m_repository;
    submit<QList<FileDiff>>([repository, stashRef]() { return repository->stashShow(stashRef); },
                             [this, stashRef](const QList<FileDiff> &diffs) {
                                 emit stashDetails(stashRef, diffs);
                                 emit loadingChanged(false);
                             });
}

void StashController::reloadAfter(const OperationResult &result)
{
    if (!result.ok) {
        emit loadingChanged(false);
        emit operationFailed(result.message, {}, result.command);
        return;
    }
    GitRepository *repository = m_repository;
    submit<QList<StashInfo>>(
        [repository]() {
            repository->refreshHead();
            return repository->stashList();
        },
        [this, result](const QList<StashInfo> &entries) {
            m_entries = entries;
            emit stashChanged(m_entries);
            emit headChanged();
            emit stashOperationDone(result.message, result.command);
            emit loadingChanged(false);
        });
}

} // namespace Guit
