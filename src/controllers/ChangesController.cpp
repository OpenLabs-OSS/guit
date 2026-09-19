#include "ChangesController.h"

namespace Guit
{

ChangesController::ChangesController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

void ChangesController::refresh()
{
    GitRepository *repository = m_repository;
    submit<StatusSnapshot>([repository]() { return repository->status(); },
                           [this](const StatusSnapshot &snapshot) {
                               m_status = snapshot;
                               emit statusChanged(m_status);
                               if (!m_status.valid && !m_status.errorMessage.isEmpty())
                                   emit operationFailed(m_status.errorMessage, {}, {});
                               emit loadingChanged(false);
                           });
}

void ChangesController::stage(const QStringList &paths)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, paths]() { return repository->stagePaths(paths); },
                           [this](const OperationResult &result) { reloadAfter(result, false, {}); });
}

void ChangesController::unstage(const QStringList &paths)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, paths]() { return repository->unstagePaths(paths); },
                           [this](const OperationResult &result) { reloadAfter(result, false, {}); });
}

void ChangesController::stageAll()
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository]() { return repository->stageAll(); },
                           [this](const OperationResult &result) { reloadAfter(result, false, {}); });
}

void ChangesController::unstageAll()
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository]() { return repository->unstageAll(); },
                           [this](const OperationResult &result) { reloadAfter(result, false, {}); });
}

void ChangesController::discard(const QList<FileStatusEntry> &entries)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, entries]() { return repository->discardEntries(entries); },
                           [this](const OperationResult &result) { reloadAfter(result, false, {}); });
}

void ChangesController::loadDiff(const QString &path, ChangesController::DiffKind kind)
{
    GitRepository *repository = m_repository;
    submit<QList<FileDiff>>(
        [repository, path, kind]() {
            return (kind == DiffKind::Staged) ? repository->diffStaged(path) : repository->diffUnstaged(path);
        },
        [this, kind](const QList<FileDiff> &diffs) {
            emit diffLoaded(diffs, kind);
            emit loadingChanged(false);
        });
}

void ChangesController::commit(const QString &subject, const QString &body, bool amend)
{
    GitRepository *repository = m_repository;
    submit<CommitResult>([repository, subject, body, amend]() { return repository->commit(subject, body, amend); },
                         [this](const CommitResult &result) { reloadAfter(result, true, result.commitHash); });
}

void ChangesController::reloadAfter(const OperationResult &result, bool isCommit, const QString &hash)
{
    if (!result.ok) {
        emit loadingChanged(false);
        emit operationFailed(result.message, {}, result.command);
        return;
    }
    // Mutation done: reload HEAD and status on the worker before announcing
    // completion so the UI never shows stale branch or file state.
    GitRepository *repository = m_repository;
    submit<StatusSnapshot>(
        [repository]() {
            repository->refreshHead();
            return repository->status();
        },
        [this, result, isCommit, hash](const StatusSnapshot &snapshot) {
            m_status = snapshot;
            emit statusChanged(m_status);
            emit headChanged();
            if (isCommit)
                emit committed(result.message, result.command, hash);
            else
                emit staged(result.message, result.command);
            emit loadingChanged(false);
        });
}

} // namespace Guit
