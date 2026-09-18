#include "AdvancedModels.h"

#include <QRegularExpression>

namespace Guit
{

QList<RemoteInfo> RemoteInfo::parseVerboseList(const QString &output)
{
    QList<RemoteInfo> remotes;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        // "<name>\t<url> (fetch)" or "(push)".
        const int tab = line.indexOf(QLatin1Char('\t'));
        const int paren = line.lastIndexOf(QLatin1Char('('));
        if (tab <= 0 || paren <= tab)
            continue;
        const QString name = line.left(tab).trimmed();
        const QString url = line.mid(tab + 1, paren - tab - 1).trimmed();
        const QString kind = line.mid(paren);
        if (name.isEmpty() || url.isEmpty())
            continue;
        RemoteInfo *remote = nullptr;
        for (RemoteInfo &existing : remotes) {
            if (existing.name == name) {
                remote = &existing;
                break;
            }
        }
        if (remote == nullptr) {
            remotes.append(RemoteInfo{name});
            remote = &remotes.last();
        }
        if (kind.startsWith(QStringLiteral("(fetch")))
            remote->fetchUrl = url;
        else if (kind.startsWith(QStringLiteral("(push")))
            remote->pushUrl = url;
    }
    return remotes;
}

QString TagInfo::forEachRefFormat()
{
    return QStringLiteral("%(refname:short)%1f%(objecttype)%1f%(objectname)%1f%(*objectname)%1f%(taggername)%1f%(taggerdate:iso)%1f%(subject)");
}

QList<TagInfo> TagInfo::parseForEachRef(const QString &output)
{
    QList<TagInfo> tags;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList fields = line.split(QChar(0x1F), Qt::KeepEmptyParts);
        if (fields.size() < 7)
            continue;
        TagInfo tag;
        tag.name = fields.at(0).trimmed();
        if (tag.name.isEmpty())
            continue;
        const QString objectType = fields.at(1).trimmed();
        const QString objectHash = fields.at(2).trimmed();
        const QString peeledHash = fields.at(3).trimmed();
        tag.annotated = objectType == QStringLiteral("tag");
        tag.targetHash = !peeledHash.isEmpty() ? peeledHash : objectHash;
        tag.tagger = fields.at(4).trimmed();
        tag.date = QDateTime::fromString(fields.at(5).trimmed(), Qt::ISODate);
        tag.message = fields.at(6).trimmed();
        tags.append(tag);
    }
    return tags;
}

QList<StashInfo> StashInfo::parseList(const QString &output)
{
    QList<StashInfo> entries;
    static const QRegularExpression pattern(QStringLiteral(R"(^(stash@\{(\d+)\}):\s*(.*)$)"));
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = pattern.match(line.trimmed());
        if (!match.hasMatch())
            continue;
        StashInfo entry;
        entry.ref = match.captured(1);
        entry.index = match.captured(2).toInt();
        entry.message = match.captured(3).trimmed();
        entries.append(entry);
    }
    return entries;
}

QString pendingOperationLabel(PendingOperation operation)
{
    switch (operation) {
    case PendingOperation::None:          return QStringLiteral("No operation in progress");
    case PendingOperation::Merging:       return QStringLiteral("Merging");
    case PendingOperation::Rebasing:      return QStringLiteral("Rebasing");
    case PendingOperation::CherryPicking: return QStringLiteral("Cherry-picking");
    case PendingOperation::Reverting:     return QStringLiteral("Reverting");
    }
    return {};
}

} // namespace Guit
