#pragma once

#include "AsyncController.h"
#include "../git/AdvancedModels.h"
#include "../git/DiffInfo.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates the stash: saving, listing, inspecting, restoring, and
// deleting stashed changes.
class StashController : public AsyncController
{
    Q_OBJECT

public:
    explicit StashController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] QList<StashInfo> entries() const { return m_entries; }

public slots:
    void refresh();
    void save(const QString &message, bool includeUntracked);
    void apply(const QString &stashRef);
    void pop(const QString &stashRef);
    void drop(const QString &stashRef);
    void clear();
    void inspect(const QString &stashRef);

signals:
    void stashChanged(const QList<Guit::StashInfo> &entries);
    void stashOperationDone(const QString &message, const QString &command);
    void stashDetails(const QString &stashRef, const QList<Guit::FileDiff> &diffs);
    void operationFailed(const QString &reason, const QString &details, const QString &command);
    void headChanged();

private:
    void reloadAfter(const OperationResult &result);

    GitRepository *m_repository = nullptr;
    QList<StashInfo> m_entries;
};

} // namespace Guit
