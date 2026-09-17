#pragma once

#include <QList>
#include <QString>

namespace Guit
{

// One added/removed/context line inside a hunk. `origin` is ' ', '+', or
// '-'; lines like "\ No newline at end of file" use origin '\\'.
struct DiffLine
{
    QChar origin = QLatin1Char(' ');
    QString content;
};

// One "@@ -old,count +new,count @@" section of a file diff.
struct DiffHunk
{
    int oldStart = 0;
    int oldCount = 0;
    int newStart = 0;
    int newCount = 0;
    QString sectionHeading;
    QList<DiffLine> lines;
};

// The diff of a single file from unified `git diff` output.
struct FileDiff
{
    QString oldPath;
    QString newPath;
    bool isNewFile = false;
    bool isDeleted = false;
    bool isRename = false;
    bool isBinary = false;
    int added = 0;
    int removed = 0;
    QList<DiffHunk> hunks;

    [[nodiscard]] QString displayPath() const { return isDeleted ? oldPath : newPath; }
    [[nodiscard]] QString summary() const;
};

// Parses `git diff` unified output (working tree, staged, or commit diffs).
// Handles new/deleted/renamed/binary files and multiple hunks. Malformed
// sections are skipped, never fatal.
QList<FileDiff> parseUnifiedDiff(const QString &output);

} // namespace Guit
