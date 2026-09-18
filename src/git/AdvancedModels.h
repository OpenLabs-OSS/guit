#pragma once

#include "DiffInfo.h"

#include <QDateTime>
#include <QList>
#include <QString>

namespace Guit
{

// One configured remote (fetch + push URLs may differ).
struct RemoteInfo
{
    QString name;
    QString fetchUrl;
    QString pushUrl;

    // Parses `git remote -v` output:
    //   origin\tgit@host:user/repo.git (fetch)
    static QList<RemoteInfo> parseVerboseList(const QString &output);
};

// One tag. Annotated tags carry their own message/tagger/date;
// lightweight tags point straight at a commit.
struct TagInfo
{
    QString name;
    QString targetHash;   // peeled commit for annotated tags, object otherwise
    bool annotated = false;
    QString message;
    QString tagger;
    QDateTime date;

    // Parses `git for-each-ref` with TagInfo::forEachRefFormat() (%1f
    // separators, like BranchInfo — for-each-ref decodes %NN escapes).
    static QString forEachRefFormat();
    static QList<TagInfo> parseForEachRef(const QString &output);
};

// One stash entry from `git stash list`:
//   stash@{0}: WIP on main: 34307fa Commit subject
struct StashInfo
{
    QString ref;      // e.g. "stash@{0}"
    int index = -1;
    QString message;  // everything after "stash@{N}: "

    static QList<StashInfo> parseList(const QString &output);
};

// An in-progress operation that may own conflicted files. Detected from
// the .git directory state (MERGE_HEAD, rebase-merge/, ...), so it is
// accurate even if Guit did not start the operation.
enum class PendingOperation
{
    None,
    Merging,
    Rebasing,
    CherryPicking,
    Reverting
};

QString pendingOperationLabel(PendingOperation operation);

struct OperationState
{
    PendingOperation operation = PendingOperation::None;
    bool merging = false;       // MERGE_HEAD present
    bool rebasing = false;      // rebase-merge/ or rebase-apply/ present
    bool cherryPicking = false; // CHERRY_PICK_HEAD present
    bool reverting = false;     // REVERT_HEAD present

    [[nodiscard]] bool isActive() const { return operation != PendingOperation::None; }
};

// One reflog entry: where a reference pointed, and why it moved.
// Parsed from `git reflog --format=%H%x1f%gd%x1f%gs%x1f%an%x1f%aI%x1e`.
struct ReflogEntry
{
    QString hash;
    QString ref;      // e.g. "HEAD@{0}"
    QString message;  // e.g. "commit: Initial commit"
    QString author;
    QDateTime date;

    static QString logFormat();
    static QList<ReflogEntry> parse(const QString &output);
};

// Git LFS state: whether the extension exists and whether this
// repository uses it. Guit never reimplements LFS.
struct LfsInfo
{
    bool available = false;
    QString version;
    bool enabledInRepo = false; // .gitattributes routes through the LFS filter
    int trackedFiles = 0;       // files currently handled by LFS
};

// One submodule from `git submodule status`:
// "<status><hash> <path> (<describe>)" where status is ' ' (in sync),
// '-' (not initialized), '+' (different commit checked out), or 'U'.
struct SubmoduleInfo
{
    QString name;    // path (submodule status prints the path)
    QString path;
    QString hash;
    QChar status = QLatin1Char(' ');
    QString describe;

    [[nodiscard]] bool initialized() const { return status != QLatin1Char('-'); }
    [[nodiscard]] bool inSync() const { return status == QLatin1Char(' '); }

    static QList<SubmoduleInfo> parseStatus(const QString &output);
};

// One worktree from `git worktree list --porcelain`.
struct WorktreeInfo
{
    QString path;
    QString hash;
    QString branch;   // short name, or "(detached)" / "(bare)"
    bool isMain = false;
    bool locked = false;
    QString lockReason;

    static QList<WorktreeInfo> parsePorcelain(const QString &output);
};

// Repository facts for the dashboard.
struct RepoInfo
{
    bool valid = false;
    QString rootPath;
    QString gitDir;
    bool isBare = false;
    QString branch;
    bool detached = false;
    bool unborn = false;
    QString headHash;
    int commitCount = -1;   // -1 when unknown (e.g. unborn)
    int remoteCount = 0;
    int branchCount = 0;
    int tagCount = 0;
    int stashCount = 0;
    qlonglong gitDirSizeBytes = -1;
};

} // namespace Guit
