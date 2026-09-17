#pragma once

#include <QString>
#include <QStringList>

namespace Guit
{

// Formats the equivalent Git command for display so users can always see
// what Guit is doing on their behalf. Display-only: Guit executes Git via
// QProcess argument lists, never by parsing these strings back.
class GitCommandFormatter
{
public:
    GitCommandFormatter() = delete;

    static QString format(const QStringList &arguments,
                          const QString &executableDisplayName = QStringLiteral("git"));
    static QString quoteArgument(const QString &argument);
};

} // namespace Guit
