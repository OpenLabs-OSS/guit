#include "DiffInfo.h"

#include <QRegularExpression>

namespace Guit
{

QString FileDiff::summary() const
{
    if (isBinary)
        return QStringLiteral("binary");
    if (isNewFile)
        return QStringLiteral("+%1").arg(added);
    if (isDeleted)
        return QStringLiteral("-%1").arg(removed);
    return QStringLiteral("+%1 -%2").arg(added).arg(removed);
}

namespace
{

// Splits `a/path` `"a/my file"` pairs, respecting double quotes the way
// Git emits them for paths with special characters.
QStringList splitQuotedPair(const QString &text)
{
    QStringList parts;
    QString current;
    bool inQuotes = false;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('"')) {
            inQuotes = !inQuotes;
            continue;
        }
        if (c == QLatin1Char(' ') && !inQuotes) {
            if (!current.isEmpty())
                parts.append(current);
            current.clear();
            continue;
        }
        current.append(c);
    }
    if (!current.isEmpty())
        parts.append(current);
    return parts;
}

// Strips the conventional a/ b/ prefixes Git prepends (unless disabled by
// --no-prefix, in which case there is nothing to strip).
QString stripABPrefix(const QString &path)
{
    if (path.size() > 2 && path.at(1) == QLatin1Char('/')
        && (path.at(0) == QLatin1Char('a') || path.at(0) == QLatin1Char('b')))
        return path.mid(2);
    return path;
}

} // namespace

QList<FileDiff> parseUnifiedDiff(const QString &output)
{
    QList<FileDiff> files;
    FileDiff *current = nullptr;
    DiffHunk *hunk = nullptr;
    bool inBinaryBody = false;

    static const QRegularExpression hunkHeader(
        QStringLiteral(R"(^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@(.*)$)"));

    const QStringList lines = output.split(QLatin1Char('\n'));
    for (QString rawLine : lines) {
        if (rawLine.endsWith(QLatin1Char('\r')))
            rawLine.chop(1);

        if (rawLine.startsWith(QStringLiteral("diff --git "))) {
            const QStringList paths = splitQuotedPair(rawLine.mid(QStringLiteral("diff --git ").size()));
            files.append(FileDiff());
            current = &files.last();
            hunk = nullptr;
            inBinaryBody = false;
            if (paths.size() >= 2) {
                current->oldPath = stripABPrefix(paths.at(0));
                current->newPath = stripABPrefix(paths.at(1));
            }
            continue;
        }
        if (current == nullptr)
            continue;

        if (rawLine.startsWith(QStringLiteral("Binary files ")) && rawLine.endsWith(QStringLiteral(" differ"))) {
            current->isBinary = true;
            inBinaryBody = false;
            continue;
        }
        if (rawLine.startsWith(QStringLiteral("GIT binary patch"))) {
            current->isBinary = true;
            inBinaryBody = true;
            continue;
        }
        if (inBinaryBody)
            continue;
        if (rawLine.startsWith(QStringLiteral("new file mode"))) {
            current->isNewFile = true;
            continue;
        }
        if (rawLine.startsWith(QStringLiteral("deleted file mode"))) {
            current->isDeleted = true;
            continue;
        }
        if (rawLine.startsWith(QStringLiteral("rename from "))) {
            current->isRename = true;
            current->oldPath = rawLine.mid(QStringLiteral("rename from ").size());
            continue;
        }
        if (rawLine.startsWith(QStringLiteral("rename to "))) {
            current->isRename = true;
            current->newPath = rawLine.mid(QStringLiteral("rename to ").size());
            continue;
        }
        // "---", "+++", "index", "similarity index", "old mode"/"new mode"
        // carry no additional information the UI needs.
        if (rawLine.startsWith(QStringLiteral("--- ")) || rawLine.startsWith(QStringLiteral("+++ "))
            || rawLine.startsWith(QStringLiteral("index ")) || rawLine.startsWith(QStringLiteral("similarity "))
            || rawLine.startsWith(QStringLiteral("dissimilarity ")) || rawLine.startsWith(QStringLiteral("old mode "))
            || rawLine.startsWith(QStringLiteral("new mode ")))
            continue;

        const QRegularExpressionMatch hunkMatch = hunkHeader.match(rawLine);
        if (hunkMatch.hasMatch()) {
            current->hunks.append(DiffHunk());
            hunk = &current->hunks.last();
            hunk->oldStart = hunkMatch.captured(1).toInt();
            hunk->oldCount = hunkMatch.captured(2).isEmpty() ? 1 : hunkMatch.captured(2).toInt();
            hunk->newStart = hunkMatch.captured(3).toInt();
            hunk->newCount = hunkMatch.captured(4).isEmpty() ? 1 : hunkMatch.captured(4).toInt();
            hunk->sectionHeading = hunkMatch.captured(5);
            continue;
        }

        if (hunk != nullptr && !rawLine.isEmpty()) {
            const QChar origin = rawLine.at(0);
            if (origin == QLatin1Char(' ') || origin == QLatin1Char('+') || origin == QLatin1Char('-')) {
                DiffLine line{origin, rawLine.mid(1)};
                hunk->lines.append(line);
                if (origin == QLatin1Char('+'))
                    current->added++;
                else if (origin == QLatin1Char('-'))
                    current->removed++;
                continue;
            }
            if (rawLine.startsWith(QStringLiteral("\\ "))) {
                hunk->lines.append(DiffLine{QLatin1Char('\\'), rawLine});
                continue;
            }
        }
    }

    // Drop empty entries produced by trailing garbage without content.
    QList<FileDiff> result;
    result.reserve(files.size());
    for (const FileDiff &file : files) {
        if (!file.oldPath.isEmpty() || !file.newPath.isEmpty())
            result.append(file);
    }
    return result;
}

} // namespace Guit
