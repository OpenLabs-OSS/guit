#include "HistoryController.h"

namespace Guit
{

HistoryController::HistoryController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

void HistoryController::refresh()
{
    GitRepository *repository = m_repository;
    submit<HistoryData>(
        [repository]() {
            // All refs, topo-ordered: the graph needs every branch, not just HEAD.
            HistoryData data;
            data.commits = repository->logAll(500);
            data.refs = repository->refsByHash();
            data.graph = GraphLanes::compute(data.commits);
            data.headHash = repository->head().commitHash;
            return data;
        },
        [this](const HistoryData &data) {
            m_commits = data.commits;
            emit historyChanged(m_commits, data.refs, data.graph, data.headHash);
            emit loadingChanged(false);
        });
}

void HistoryController::selectCommit(const QString &hash)
{
    GitRepository *repository = m_repository;
    submit<CommitDetails>([repository, hash]() { return repository->showCommit(hash); },
                          [this](const CommitDetails &details) {
                              if (!details.valid)
                                  emit operationFailed(details.errorMessage, {});
                              emit detailsChanged(details);
                              emit loadingChanged(false);
                          });
}

void HistoryController::search(const QString &query)
{
    GitRepository *repository = m_repository;
    submit<QList<CommitInfo>>([repository, query]() { return repository->searchCommits(query); },
                              [this, query](const QList<CommitInfo> &commits) {
                                  emit searchChanged(commits, query.trimmed());
                                  emit loadingChanged(false);
                              });
}

void HistoryController::clearSearch()
{
    refresh();
}

} // namespace Guit
