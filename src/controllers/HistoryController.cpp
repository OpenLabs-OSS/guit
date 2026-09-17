#include "HistoryController.h"

namespace Guit
{

HistoryController::HistoryController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

void HistoryController::refresh()
{
    m_commits = m_repository->log();
    emit historyChanged(m_commits);
}

void HistoryController::selectCommit(const QString &hash)
{
    const CommitDetails details = m_repository->showCommit(hash);
    if (!details.valid)
        emit operationFailed(details.errorMessage, {});
    emit detailsChanged(details);
}

} // namespace Guit
