#pragma once

#include "DiffInfo.h"
#include "FileStatus.h"
#include "GitClient.h"
#include "GitModels.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace Guit
{

// Current status of the working tree and staging area.
struct StatusSnapshot
{
    bool valid = false;
    QString errorMessage;
    QList<FileStatusEntry> entries;

    [[nodiscard]] QList<FileStatusEntry> staged() const;
    [[nodiscard]] QList<FileStatusEntry> unstaged() const;
    [[nodiscard]] bool isClean() const { return valid && entries.isEmpty(); }
};

// Outcome of a mutating repository operation. `command` is the exact
// equivalent Git command shown to the user for transparency.
struct OperationResult
{
    bool ok = false;
    QString message;
    QString command;
};

struct CommitResult : OperationResult
{
    QString commitHash;
};

// Current HEAD state of an open repository.
struct HeadInfo
{
    bool known = false;      // false when the repository has no HEAD info (e.g. probe failed)
    QString branch;          // current branch name, empty when detached or unborn
    bool detached = false;
    bool unborn = false;     // repository with no commits yet
    QString commitHash;      // empty when unborn
};

// Repository-level abstraction: owns the working-tree root, the .git
// location, and cached HEAD state. All Git access goes through GitClient;
// widgets must talk to this class (via RepositoryController), never to
// GitClient or QProcess directly.
class GitRepository : public QObject
{
    Q_OBJECT

public:
    explicit GitRepository(QObject *parent = nullptr);
    GitClient *client() const { return m_client; }

    // Opens any directory inside a working tree (the root is resolved).
    // Returns false and emits openFailed() when the directory is invalid.
    bool open(const QString &path);
    void close();

    [[nodiscard]] bool isValid() const { return m_valid; }
    [[nodiscard]] QString rootPath() const { return m_rootPath; }
    [[nodiscard]] QString gitDir() const { return m_gitDir; }
    [[nodiscard]] bool isBare() const { return m_bare; }
    [[nodiscard]] HeadInfo head() const { return m_head; }

    bool refreshHead();

    // --- Milestone 2: working tree -------------------------------------
    // All of these require an open repository; otherwise the result
    // reports the failure instead of crashing or guessing.
    StatusSnapshot status() const;
    OperationResult stagePaths(const QStringList &paths);
    OperationResult unstagePaths(const QStringList &paths);
    OperationResult stageAll();
    OperationResult unstageAll();
    // Restores paths to HEAD (tracked files) or deletes them (untracked).
    // Refuses conflicted paths: conflict resolution arrives in Milestone 3.
    OperationResult discardEntries(const QList<FileStatusEntry> &entries);
    CommitResult commit(const QString &subject, const QString &body, bool amend = false);
    QList<FileDiff> diffUnstaged(const QString &path = {}) const;
    QList<FileDiff> diffStaged(const QString &path = {}) const;

    // --- Milestone 2: history ------------------------------------------
    QList<CommitInfo> log(int maxCount = 500) const;
    CommitDetails showCommit(const QString &hash) const;

    // --- Milestone 2: branches -----------------------------------------
    QList<BranchInfo> branches() const;
    bool validateBranchName(const QString &name) const;
    OperationResult createBranch(const QString &name, const QString &startPoint, bool checkout);
    OperationResult switchBranch(const QString &name);
    OperationResult renameBranch(const QString &oldName, const QString &newName);
    OperationResult deleteBranch(const QString &name, bool force);
    AheadBehind aheadBehind(const QString &from, const QString &to) const;
    QList<FileDiff> compareDiff(const QString &from, const QString &to) const;

    // --- Repository setup ----------------------------------------------
    OperationResult initRepository(const QString &path, const QString &initialBranch = {});

signals:
    void repositoryChanged();
    void repositoryClosed();
    void openFailed(const QString &reason, const QString &details);

private:
    GitClient *m_client = nullptr;
    bool m_valid = false;
    bool m_bare = false;
    QString m_rootPath;
    QString m_gitDir;
    HeadInfo m_head;
};

} // namespace Guit
