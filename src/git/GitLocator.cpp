#include "GitLocator.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QProcessEnvironment>

namespace Guit
{

QString findGitExecutable(const QString &overridePath)
{
    const auto usable = [](const QString &candidate) {
        return !candidate.isEmpty() && QFileInfo(candidate).isExecutable();
    };

    const QString fromEnv = QProcessEnvironment::systemEnvironment().value(QStringLiteral("GUIT_GIT_EXECUTABLE"));

    // Explicit configuration is authoritative: when the user (or the test
    // environment) names an executable, a broken path must surface as
    // "Git not found", not silently fall back to a different Git.
    if (!overridePath.isEmpty() || !fromEnv.isEmpty()) {
        if (usable(overridePath))
            return overridePath;
        if (usable(fromEnv))
            return fromEnv;
        return {};
    }

    return QStandardPaths::findExecutable(QStringLiteral("git"));
}

} // namespace Guit
