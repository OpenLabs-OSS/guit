#pragma once

#include "AsyncController.h"
#include "../git/AdvancedModels.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates repository insight and maintenance: facts dashboard, LFS,
// submodules, worktrees, reflog, and the .gitignore helper.
class RepositoryInfoController : public AsyncController
{
    Q_OBJECT

public:
    explicit RepositoryInfoController(GitRepository *repository, QObject *parent = nullptr);

public slots:
    void refresh();
    void updateSubmodules(bool initialize);
    void syncSubmodules();
    void addWorktree(const QString &path, const QString &source, bool newBranch);
    void removeWorktree(const QString &path, bool force);
    void pruneWorktrees();
    void trackLfs(const QString &pattern);
    void saveGitignore(const QString &content);

signals:
    void infoChanged(const Guit::RepoInfo &info,
                     const Guit::LfsInfo &lfs,
                     const QList<Guit::SubmoduleInfo> &submodules,
                     const QList<Guit::WorktreeInfo> &worktrees,
                     const QList<Guit::ReflogEntry> &reflog,
                     const QString &gitignore);
    void operationDone(const QString &message, const QString &command);
    void operationFailed(const QString &reason, const QString &details, const QString &command);
    void headChanged();

private:
    struct InfoBundle
    {
        RepoInfo info;
        LfsInfo lfs;
        QList<SubmoduleInfo> submodules;
        QList<WorktreeInfo> worktrees;
        QList<ReflogEntry> reflog;
        QString gitignore;
    };
    void reloadAfter(const OperationResult &result);

    GitRepository *m_repository = nullptr;
};

} // namespace Guit
