#pragma once

#include "AdvancedModels.h"
#include "DiffInfo.h"
#include "FileStatus.h"
#include "GitClient.h"
#include "GitModels.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Guit
{

class AsyncGitProcess;

enum class ResetMode
{
    Soft,   // move branch pointer only; index and files untouched
    Mixed,  // + reset the index; files untouched (default)
    Hard    // + discard all working-tree changes (destructive)
};

QString resetModeLabel(ResetMode mode);
QString resetModeExplanation(ResetMode mode);

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
// equivalent Git command shown to the user for transparency. `conflict`
// marks the specific case "the operation stopped at conflicts": the UI
// switches to conflict resolution instead of showing an error box.
struct OperationResult
{
    bool ok = false;
    bool conflict = false;
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
    // Refuses conflicted paths: resolve them with resolveWithOurs/Theirs
    // (or by editing) and stage the result instead.
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

    // --- Milestone 3: remotes ------------------------------------------
    QList<RemoteInfo> remotes() const;
    OperationResult addRemote(const QString &name, const QString &url);
    OperationResult removeRemote(const QString &name);
    OperationResult renameRemote(const QString &oldName, const QString &newName);
    OperationResult setRemoteUrl(const QString &name, const QString &url);

    // --- Milestone 3: network operations (asynchronous) ------------------
    // fetch/pull/push run on AsyncGitProcess so the UI stays responsive.
    // Progress lines go to networkProgress; the outcome (including user
    // cancellation and authentication failures surfaced by Git) goes to
    // networkFinished. Only one network operation runs at a time.
    void startFetch(const QString &remote, bool prune);
    void startPull(const QString &remote);
    void startPush(const QString &remote, const QString &source, bool setUpstream = false, bool force = false,
                   bool deleteRemote = false);
    void cancelNetworkOperation();
    [[nodiscard]] bool isNetworkRunning() const;

    // --- Milestone 3: tags -----------------------------------------------
    QList<TagInfo> tags() const;
    // Empty message creates a lightweight tag; otherwise annotated.
    OperationResult createTag(const QString &name, const QString &message, const QString &target = {});
    OperationResult deleteTag(const QString &name);
    CommitDetails showTag(const QString &name) const;

    // --- Milestone 3: stash -----------------------------------------------
    QList<StashInfo> stashList() const;
    OperationResult stashPush(const QString &message, bool includeUntracked);
    OperationResult stashApply(const QString &stashRef);
    OperationResult stashPop(const QString &stashRef);
    OperationResult stashDrop(const QString &stashRef);
    OperationResult stashClear();
    QList<FileDiff> stashShow(const QString &stashRef) const;

    // --- Milestone 3: merge -------------------------------------------------
    // A conflicting merge succeeds at the Git level but leaves MERGE_HEAD
    // behind: the result reports ok=false with a conflict message so the UI
    // can switch to conflict resolution instead of showing an error.
    OperationResult mergeBranch(const QString &name, bool noFastForward);
    OperationResult mergeAbort();
    OperationResult mergeContinue();

    // --- Milestone 3: rebase --------------------------------------------------
    OperationResult rebaseOnto(const QString &branch);
    OperationResult rebaseContinue();
    OperationResult rebaseSkip();
    OperationResult rebaseAbort();

    // --- Milestone 3: reset / revert / cherry-pick ------------------------------
    OperationResult resetTo(const QString &target, ResetMode mode);
    OperationResult revertCommit(const QString &hash);
    OperationResult revertContinue();
    OperationResult revertAbort();
    OperationResult cherryPick(const QString &hash);
    OperationResult cherryPickContinue();
    OperationResult cherryPickAbort();

    // --- Milestone 3: operation state + conflict resolution ----------------------
    // Reads .git state so operations started outside Guit are detected too.
    OperationState operationState() const;
    OperationResult resolveWithOurs(const QString &path);
    OperationResult resolveWithTheirs(const QString &path);

    // --- Milestone 4: graph + search -----------------------------------------
    // All refs, topo-ordered, for the graph view and commit search.
    QList<CommitInfo> logAll(int maxCount = 2000) const;
    // Short ref names (branches, tags) grouped by the commit they point at.
    QMap<QString, QStringList> refsByHash() const;
    // Case-insensitive match over subject/body/author/hash.
    QList<CommitInfo> searchCommits(const QString &query, int maxCount = 2000) const;

    // --- Milestone 4: reflog ----------------------------------------------------
    QList<ReflogEntry> reflog(int maxCount = 200) const;

    // --- Milestone 4: LFS / submodules / worktrees --------------------------------
    LfsInfo lfsInfo() const;
    OperationResult lfsTrack(const QString &pattern);
    QList<SubmoduleInfo> submodules() const;
    OperationResult submoduleUpdate(bool initialize);
    OperationResult submoduleSync();
    QList<WorktreeInfo> worktrees() const;
    OperationResult worktreeAdd(const QString &path, const QString &source, bool newBranch);
    OperationResult worktreeRemove(const QString &path, bool force);
    OperationResult worktreePrune();

    // --- Milestone 4: repository facts + .gitignore ---------------------------------
    RepoInfo repositoryInfo() const;
    QString readGitignore() const;
    OperationResult writeGitignore(const QString &content);
    static QMap<QString, QStringList> gitignorePresets();

signals:
    void repositoryChanged();
    void repositoryClosed();
    void openFailed(const QString &reason, const QString &details);
    void networkStarted(const QString &command);
    void networkProgress(const QString &text);
    void networkFinished(const Guit::OperationResult &result);

private:
    void startNetwork(const QStringList &args, const QString &successMessage);
    GitClient *m_client = nullptr;
    AsyncGitProcess *m_network = nullptr;
    QString m_networkCommand;
    QString m_networkSuccessMessage;
    bool m_valid = false;
    bool m_bare = false;
    QString m_rootPath;
    QString m_gitDir;
    HeadInfo m_head;
};

} // namespace Guit

// Registered so OperationResult can travel through signals (and QSignalSpy)
// across threads and test boundaries.
Q_DECLARE_METATYPE(Guit::OperationResult)
