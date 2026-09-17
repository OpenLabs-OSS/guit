#include "FileStatus.h"

namespace Guit
{

QString fileStateLabel(FileState state)
{
    switch (state) {
    case FileState::Unmodified: return QStringLiteral("Unmodified");
    case FileState::Modified:   return QStringLiteral("Modified");
    case FileState::Added:      return QStringLiteral("Added");
    case FileState::Deleted:    return QStringLiteral("Deleted");
    case FileState::Renamed:    return QStringLiteral("Renamed");
    case FileState::Copied:     return QStringLiteral("Copied");
    case FileState::Untracked:  return QStringLiteral("Untracked");
    case FileState::Ignored:    return QStringLiteral("Ignored");
    case FileState::Conflicted: return QStringLiteral("Conflicted");
    }
    return QStringLiteral("Unknown");
}

namespace
{

FileState mapStatusChar(QChar c, bool stagedSide)
{
    switch (c.toLatin1()) {
    case ' ': return FileState::Unmodified;
    case 'M': return FileState::Modified;
    case 'A': return FileState::Added;
    case 'D': return FileState::Deleted;
    case 'R': return FileState::Renamed;
    case 'C': return FileState::Copied;
    case 'U': return FileState::Conflicted;
    case '?': return stagedSide ? FileState::Untracked : FileState::Untracked;
    case '!': return FileState::Ignored;
    default:  return FileState::Unmodified;
    }
}

bool isConflictCombination(QChar x, QChar y)
{
    // Unmerged paths per git status documentation: any U, or AA / DD.
    if (x == QLatin1Char('U') || y == QLatin1Char('U'))
        return true;
    if (x == QLatin1Char('A') && y == QLatin1Char('A'))
        return true;
    if (x == QLatin1Char('D') && y == QLatin1Char('D'))
        return true;
    return false;
}

} // namespace

QList<FileStatusEntry> parsePorcelainV1Status(const QString &output)
{
    QList<FileStatusEntry> entries;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    entries.reserve(lines.size());

    for (const QString &rawLine : lines) {
        // Never trim the start of the line: the two status columns are
        // position-sensitive and a leading space is meaningful
        // (e.g. " M file.txt" is modified in the working tree only).
        QString line = rawLine;
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        // Minimum valid entry: "XY path".
        if (line.size() < 4 || line.at(2) != QLatin1Char(' '))
            continue;

        const QChar x = line.at(0);
        const QChar y = line.at(1);
        // Trailing whitespace is chopped; leading whitespace belongs to the
        // status columns and was already validated above.
        QString path = line.mid(3);
        while (path.endsWith(QLatin1Char(' ')) || path.endsWith(QLatin1Char('\t')))
            path.chop(1);
        if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"')) && path.size() >= 2)
            path = path.mid(1, path.size() - 2);
        if (path.isEmpty())
            continue;

        FileStatusEntry entry;
        if (isConflictCombination(x, y)) {
            entry.stagedState = FileState::Conflicted;
            entry.worktreeState = FileState::Conflicted;
        } else {
            entry.stagedState = mapStatusChar(x, true);
            entry.worktreeState = mapStatusChar(y, false);
        }

        if ((x == QLatin1Char('R') || x == QLatin1Char('C') || y == QLatin1Char('R') || y == QLatin1Char('C'))
            && path.contains(QStringLiteral(" -> "))) {
            const int arrow = path.indexOf(QStringLiteral(" -> "));
            entry.originalPath = path.left(arrow).trimmed();
            entry.path = path.mid(arrow + 4).trimmed();
        } else {
            entry.path = path;
        }
        entries.append(entry);
    }
    return entries;
}

} // namespace Guit
