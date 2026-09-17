#include "ChangesController.h"

namespace Guit
{

ChangesController::ChangesController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

void ChangesController::refresh()
{
    m_status = m_repository->status();
    emit statusChanged(m_status);
    if (!m_status.valid && !m_status.errorMessage.isEmpty())
        emit operationFailed(m_status.errorMessage, {}, {});
}

void ChangesController::stage(const QStringList &paths)
{
    handleResult(m_repository->stagePaths(paths));
}

void ChangesController::unstage(const QStringList &paths)
{
    handleResult(m_repository->unstagePaths(paths));
}

void ChangesController::stageAll()
{
    handleResult(m_repository->stageAll());
}

void ChangesController::unstageAll()
{
    handleResult(m_repository->unstageAll());
}

void ChangesController::discard(const QList<FileStatusEntry> &entries)
{
    handleResult(m_repository->discardEntries(entries));
}

void ChangesController::loadDiff(const QString &path, ChangesController::DiffKind kind)
{
    const QList<FileDiff> diffs = (kind == DiffKind::Staged) ? m_repository->diffStaged(path)
                                                             : m_repository->diffUnstaged(path);
    emit diffLoaded(diffs, kind);
}

void ChangesController::commit(const QString &subject, const QString &body, bool amend)
{
    const CommitResult result = m_repository->commit(subject, body, amend);
    handleResult(result, true, result.commitHash);
}

void ChangesController::handleResult(const OperationResult &result, bool isCommit, const QString &hash)
{
    // Every mutation reloads status and HEAD so the UI never shows stale
    // branch or file state.
    refresh();
    m_repository->refreshHead();
    emit headChanged();
    if (result.ok) {
        if (isCommit)
            emit committed(result.message, result.command, hash);
        else
            emit staged(result.message, result.command);
    } else {
        emit operationFailed(result.message, {}, result.command);
    }
}

} // namespace Guit
