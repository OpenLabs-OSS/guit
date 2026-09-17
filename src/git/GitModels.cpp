#include "GitModels.h"

namespace Guit
{

QString CommitInfo::logFormat()
{
    return QStringLiteral("%H%x1f%P%x1f%an%x1f%ae%x1f%aI%x1f%s%x1f%b%x1e");
}

QList<CommitInfo> CommitInfo::parseLog(const QString &output)
{
    QList<CommitInfo> commits;
    const QStringList records = output.split(QChar(0x1E), Qt::SkipEmptyParts);
    commits.reserve(records.size());

    for (const QString &record : records) {
        const QStringList fields = record.split(QChar(0x1F), Qt::KeepEmptyParts);
        // Trailing %x1e leaves a leading newline on the next record; trim per field.
        if (fields.size() < 7)
            continue;
        CommitInfo commit;
        commit.hash = fields.at(0).trimmed();
        if (commit.hash.isEmpty())
            continue;
        commit.parents = fields.at(1).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        commit.authorName = fields.at(2).trimmed();
        commit.authorEmail = fields.at(3).trimmed();
        commit.authorDate = QDateTime::fromString(fields.at(4).trimmed(), Qt::ISODateWithMs);
        if (!commit.authorDate.isValid())
            commit.authorDate = QDateTime::fromString(fields.at(4).trimmed(), Qt::ISODate);
        commit.subject = fields.at(5).trimmed();
        commit.body = fields.size() > 7
            ? fields.mid(6).join(QChar(0x1F)).trimmed()
            : fields.at(6).trimmed();
        commits.append(commit);
    }
    return commits;
}

QString BranchInfo::forEachRefFormat()
{
    // NOTE: unlike `git log --format` (which uses %xNN escapes),
    // `git for-each-ref --format` decodes %NN percent-escapes, so the unit
    // separator is written as %1f here. Verified against real Git output.
    return QStringLiteral("%(refname)%1f%(refname:short)%1f%(HEAD)%1f%(objectname)%1f%(upstream:short)");
}

QList<BranchInfo> BranchInfo::parseForEachRef(const QString &output)
{
    QList<BranchInfo> branches;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    branches.reserve(lines.size());

    for (const QString &line : lines) {
        const QStringList fields = line.split(QChar(0x1F), Qt::KeepEmptyParts);
        if (fields.size() < 5)
            continue;
        BranchInfo branch;
        branch.refName = fields.at(0).trimmed();
        branch.name = fields.at(1).trimmed();
        if (branch.refName.isEmpty() || branch.name.isEmpty())
            continue;
        branch.isCurrent = fields.at(2).trimmed() == QStringLiteral("*");
        branch.commitHash = fields.at(3).trimmed();
        branch.upstream = fields.at(4).trimmed();
        branch.isRemote = branch.refName.startsWith(QStringLiteral("refs/remotes/"));
        branches.append(branch);
    }
    return branches;
}

} // namespace Guit
