#pragma once

#include "AsyncController.h"
#include "../git/AdvancedModels.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates advanced mutating operations that can enter conflicted
// states: merge, rebase, reset, revert, cherry-pick, plus the shared
// conflict-resolution lifecycle (resolve files -> stage -> continue, or
// abort). Reset never conflicts but lives here because it is equally
// destructive and deserves the same confirmation discipline.
// Mutations run on the background queue; refreshState stays synchronous
// (four filesystem probes, microseconds).
class MergeController : public AsyncController
{
    Q_OBJECT

public:
    explicit MergeController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] OperationState currentState() const { return m_state; }

public slots:
    void refreshState();
    void merge(const QString &branch, bool noFastForward);
    void rebase(const QString &branch);
    void reset(const QString &target, Guit::ResetMode mode);
    void revert(const QString &hash);
    void cherryPick(const QString &hash);
    void resolveOurs(const QString &path);
    void resolveTheirs(const QString &path);
    void continueOperation();
    void skipOperation();
    void abortOperation();

signals:
    void stateChanged(const Guit::OperationState &state);
    void operationDone(const QString &message, const QString &command);
    // A conflicting start is not an error: the UI switches to resolution.
    void conflictStarted(const QString &message, const QString &command);
    void operationFailed(const QString &reason, const QString &details, const QString &command);
    void headChanged();

private:
    void handleResult(const OperationResult &result);

    GitRepository *m_repository = nullptr;
    OperationState m_state;
};

} // namespace Guit
