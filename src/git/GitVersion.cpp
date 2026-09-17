#include "GitVersion.h"

#include <QRegularExpression>

namespace Guit
{

QString GitVersion::toString() const
{
    if (!isValid())
        return raw.isEmpty() ? QStringLiteral("unknown") : raw;
    if (patch >= 0)
        return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
    return QStringLiteral("%1.%2").arg(major).arg(minor);
}

GitVersion GitVersion::parse(const QString &versionOutput)
{
    GitVersion version;
    version.raw = versionOutput.trimmed();
    // Handles "git version 2.43.0", "git version 2.43.0.windows.1", "git version 2.39".
    static const QRegularExpression pattern(QStringLiteral(R"(git version (\d+)\.(\d+)(?:\.(\d+))?)"));
    const QRegularExpressionMatch match = pattern.match(version.raw);
    if (!match.hasMatch())
        return version;
    version.major = match.captured(1).toInt();
    version.minor = match.captured(2).toInt();
    if (!match.captured(3).isEmpty())
        version.patch = match.captured(3).toInt();
    return version;
}

} // namespace Guit
