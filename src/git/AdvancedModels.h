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

} // namespace Guit
