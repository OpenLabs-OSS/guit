#pragma once

#include "../git/DiffInfo.h"
#include "../git/FileStatus.h"
#include "../git/GitModels.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace Guit
{

// Coordinates the Changes page: status refresh, staging, discarding, diff
// loading, and committing. Emits results; widgets only render state.
class ChangesController : public QObject
{
    Q_OBJECT

public:
    enum class DiffKind
    {
        Unstaged,
        Staged
    };
    Q_ENUM(DiffKind)

    explicit ChangesController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] StatusSnapshot currentStatus() const { return m_status; }

public slots:
    void refresh();
    void stage(const QStringList &paths);
    void unstage(const QStringList &paths);
    void stageAll();
    void unstageAll();
    void discard(const QList<FileStatusEntry> &entries);
    void loadDiff(const QString &path, ChangesController::DiffKind kind);
    void commit(const QString &subject, const QString &body, bool amend);

signals:
    void statusChanged(const Guit::StatusSnapshot &snapshot);
    void diffLoaded(const QList<Guit::FileDiff> &diffs, Guit::ChangesController::DiffKind kind);
    void staged(const QString &message, const QString &command);
    void committed(const QString &message, const QString &command, const QString &hash);
    void operationFailed(const QString &reason, const QString &details, const QString &command);
    void headChanged();

private:
    void handleResult(const OperationResult &result, bool isCommit = false, const QString &hash = {});

    GitRepository *m_repository = nullptr;
    StatusSnapshot m_status;
};

} // namespace Guit
