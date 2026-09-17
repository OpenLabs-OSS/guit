#include "GitCommandFormatter.h"

namespace Guit
{

QString GitCommandFormatter::quoteArgument(const QString &argument)
{
    if (argument.isEmpty())
        return QStringLiteral("\"\"");

    bool needsQuotes = false;
    for (const QChar c : argument) {
        if (!c.isLetterOrNumber() && c != QLatin1Char('_') && c != QLatin1Char('-')
            && c != QLatin1Char('.') && c != QLatin1Char('/') && c != QLatin1Char('\\')
            && c != QLatin1Char(':') && c != QLatin1Char('+') && c != QLatin1Char('=')
            && c != QLatin1Char('@') && c != QLatin1Char(',') && c != QLatin1Char('%')) {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes)
        return argument;

    QString quoted;
    quoted.reserve(argument.size() + 2);
    quoted.append(QLatin1Char('"'));
    for (const QChar c : argument) {
        if (c == QLatin1Char('"') || c == QLatin1Char('\\'))
            quoted.append(QLatin1Char('\\'));
        quoted.append(c);
    }
    quoted.append(QLatin1Char('"'));
    return quoted;
}

QString GitCommandFormatter::format(const QStringList &arguments, const QString &executableDisplayName)
{
    QString command = executableDisplayName.isEmpty() ? QStringLiteral("git") : executableDisplayName;
    for (const QString &argument : arguments)
        command += QLatin1Char(' ') + quoteArgument(argument);
    return command;
}

} // namespace Guit
