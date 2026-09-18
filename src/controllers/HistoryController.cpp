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
    // All refs, topo-ordered: the graph needs every branch, not just HEAD.
    m_commits = m_repository->logAll(500);
    emit historyChanged(m_commits, m_repository->refsByHash(),
                        GraphLanes::compute(m_commits), m_repository->head().commitHash);
}

void HistoryController::search(const QString &query)
{
    emit searchChanged(m_repository->searchCommits(query), query.trimmed());
}

void HistoryController::clearSearch()
{
    refresh();
}

void HistoryController::selectCommit(const QString &hash)
{
    const CommitDetails details = m_repository->showCommit(hash);
    if (!details.valid)
        emit operationFailed(details.errorMessage, {});
    emit detailsChanged(details);
}

} // namespace Guit
