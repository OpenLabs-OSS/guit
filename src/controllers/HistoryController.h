#pragma once

#include "../git/GitModels.h"
#include "../git/GraphLanes.h"
#include "../git/GitRepository.h"

#include <QMap>
#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates the History page: loading the commit list and resolving the
// selected commit into full details (message, files, diff).
class HistoryController : public QObject
{
    Q_OBJECT

public:
    explicit HistoryController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] QList<CommitInfo> commits() const { return m_commits; }

public slots:
    void refresh();
    void selectCommit(const QString &hash);
    void search(const QString &query);
    void clearSearch();

signals:
    void historyChanged(const QList<Guit::CommitInfo> &commits,
                        const QMap<QString, QStringList> &refs,
                        const QList<Guit::GraphRow> &graph,
                        const QString &headHash);
    void searchChanged(const QList<Guit::CommitInfo> &commits, const QString &query);
    void detailsChanged(const Guit::CommitDetails &details);
    void operationFailed(const QString &reason, const QString &details);

private:
    GitRepository *m_repository = nullptr;
    QList<CommitInfo> m_commits;
};

} // namespace Guit
