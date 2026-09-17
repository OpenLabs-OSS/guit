#pragma once

#include "DiffInfo.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

namespace Guit
{

// One commit parsed from an explicit, machine-readable log format:
//
//   git log --format=%H%x1f%P%x1f%an%x1f%ae%x1f%aI%x1f%s%x1f%b%x1e ...
//
// Unit separator (0x1F) delimits fields, record separator (0x1E) delimits
// commits, so subjects and bodies may contain any other characters safely.
struct CommitInfo
{
    QString hash;
    QStringList parents;
    QString authorName;
    QString authorEmail;
    QDateTime authorDate;
    QString subject;
    QString body;

    [[nodiscard]] QString shortHash(int length = 7) const { return hash.left(length); }
    [[nodiscard]] bool isMerge() const { return parents.size() > 1; }

    static QString logFormat();
    static QList<CommitInfo> parseLog(const QString &output);
};

// One branch parsed from an explicit for-each-ref format:
//
//   git for-each-ref --format=%(refname)%1f%(refname:short)%1f%(HEAD)%1f%(objectname)%1f%(upstream:short) refs/heads refs/remotes
//
// (%1f, not %x1f: for-each-ref decodes %NN percent-escapes.)
struct BranchInfo
{
    QString refName;       // e.g. "refs/heads/main" or "refs/remotes/origin/main"
    QString name;          // short name, e.g. "main" or "origin/main"
    bool isCurrent = false;
    bool isRemote = false;
    QString commitHash;
    QString upstream;      // e.g. "origin/main", empty when unset

    [[nodiscard]] bool isLocal() const { return !isRemote; }

    static QString forEachRefFormat();
    static QList<BranchInfo> parseForEachRef(const QString &output);
};

// One file touched by a commit, from `git diff-tree --name-status`.
struct ChangedFile
{
    QString path;
    QString oldPath;  // set for renames
    QString status;   // M, A, D, R, C, T, ...
};

// Full details for the commit-details view: the commit itself, the files
// it touched, and its diff.
struct CommitDetails
{
    CommitInfo info;
    bool valid = false;
    QString errorMessage;
    QList<ChangedFile> files;
    QList<FileDiff> diffs;

    static QList<ChangedFile> parseNameStatus(const QString &output);
};

// How many commits `to` is ahead of / behind `from`.
struct AheadBehind
{
    int ahead = 0;
    int behind = 0;
    bool valid = false;
};

} // namespace Guit
