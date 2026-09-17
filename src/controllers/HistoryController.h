#pragma once

#include "../git/GitModels.h"
#include "../git/GitRepository.h"

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

signals:
    void historyChanged(const QList<Guit::CommitInfo> &commits);
    void detailsChanged(const Guit::CommitDetails &details);
    void operationFailed(const QString &reason, const QString &details);

private:
    GitRepository *m_repository = nullptr;
    QList<CommitInfo> m_commits;
};

} // namespace Guit
