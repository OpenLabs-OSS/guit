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
            RemoteInfo fresh;
            fresh.name = name;
            remotes.append(fresh);
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

QString ReflogEntry::logFormat()
{
    // Log-family formats use %xNN escapes (%1f would print literally —
    // only for-each-ref decodes %NN). Verified against real Git output.
    return QStringLiteral("%H%x1f%gd%x1f%gs%x1f%an%x1f%aI%x1e");
}

QList<ReflogEntry> ReflogEntry::parse(const QString &output)
{
    QList<ReflogEntry> entries;
    const QStringList records = output.split(QChar(0x1E), Qt::SkipEmptyParts);
    for (const QString &record : records) {
        const QStringList fields = record.split(QChar(0x1F), Qt::KeepEmptyParts);
        if (fields.size() < 5)
            continue;
        ReflogEntry entry;
        entry.hash = fields.at(0).trimmed();
        if (entry.hash.isEmpty())
            continue;
        entry.ref = fields.at(1).trimmed();
        entry.message = fields.at(2).trimmed();
        entry.author = fields.at(3).trimmed();
        entry.date = QDateTime::fromString(fields.at(4).trimmed(), Qt::ISODate);
        entries.append(entry);
    }
    return entries;
}

QList<SubmoduleInfo> SubmoduleInfo::parseStatus(const QString &output)
{
    QList<SubmoduleInfo> modules;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (QString line : lines) {
        if (line.size() < 42)
            continue;
        SubmoduleInfo module;
        module.status = line.at(0);
        module.hash = line.mid(1, 40).trimmed();
        QString rest = line.mid(41).trimmed();
        // Optional "(describe)" suffix: "<path> (<describe>)".
        const int paren = rest.lastIndexOf(QStringLiteral(" ("));
        if (paren > 0 && rest.endsWith(QLatin1Char(')'))) {
            module.describe = rest.mid(paren + 2, rest.size() - paren - 3);
            rest = rest.left(paren);
        }
        module.path = rest;
        module.name = rest;
        if (module.path.isEmpty() || module.hash.isEmpty())
            continue;
        modules.append(module);
    }
    return modules;
}

QList<WorktreeInfo> WorktreeInfo::parsePorcelain(const QString &output)
{
    QList<WorktreeInfo> trees;
    WorktreeInfo current;
    bool hasEntry = false;
    const auto flush = [&]() {
        if (hasEntry && !current.path.isEmpty())
            trees.append(current);
    };
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (QString line : lines) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.isEmpty()) {
            flush();
            current = WorktreeInfo();
            hasEntry = false;
            continue;
        }
        hasEntry = true;
        if (line.startsWith(QStringLiteral("worktree ")))
            current.path = line.mid(QStringLiteral("worktree ").size());
        else if (line.startsWith(QStringLiteral("HEAD ")))
            current.hash = line.mid(QStringLiteral("HEAD ").size()).trimmed();
        else if (line.startsWith(QStringLiteral("branch "))) {
            QString ref = line.mid(QStringLiteral("branch ").size()).trimmed();
            current.branch = ref.startsWith(QStringLiteral("refs/heads/")) ? ref.mid(11) : ref;
        } else if (line == QStringLiteral("detached")) {
            current.branch = QStringLiteral("(detached)");
        } else if (line == QStringLiteral("bare")) {
            current.branch = QStringLiteral("(bare)");
        } else if (line == QStringLiteral("locked") || line.startsWith(QStringLiteral("locked "))) {
            current.locked = true;
            current.lockReason = line == QStringLiteral("locked") ? QString() : line.mid(7).trimmed();
        }
    }
    flush();
    // Porcelain lists the main worktree first.
    if (!trees.isEmpty())
        trees.first().isMain = true;
    return trees;
}

} // namespace Guit
