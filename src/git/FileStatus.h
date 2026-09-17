#pragma once

#include <QList>
#include <QString>

namespace Guit
{

// Per-side state of a file in `git status --porcelain=v1` output.
// The staging area (index) and the working tree each have their own state,
// which is exactly what the UI must communicate for every file.
enum class FileState
{
    Unmodified,
    Modified,
    Added,
    Deleted,
    Renamed,
    Copied,
    Untracked,
    Ignored,
    Conflicted
};

QString fileStateLabel(FileState state);

struct FileStatusEntry
{
    QString path;          // new path (or the only path)
    QString originalPath;  // set for renames/copies: "old -> new"
    FileState stagedState = FileState::Unmodified;
    FileState worktreeState = FileState::Unmodified;

    [[nodiscard]] bool isStaged() const { return stagedState != FileState::Unmodified; }
    [[nodiscard]] bool isUnstaged() const { return worktreeState != FileState::Unmodified; }
    [[nodiscard]] bool isConflicted() const
    {
        return stagedState == FileState::Conflicted || worktreeState == FileState::Conflicted;
    }
};

// Parses `git status --porcelain=v1` output. Malformed lines are skipped
// (never fatal): the parser must survive unexpected Git output.
QList<FileStatusEntry> parsePorcelainV1Status(const QString &output);

} // namespace Guit
