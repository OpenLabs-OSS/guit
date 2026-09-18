#include "TerminalLauncher.h"

#include <QDir>
#include <QProcess>
#include <QStandardPaths>

namespace Guit
{

QString TerminalLauncher::defaultTerminalName()
{
#ifdef Q_OS_WIN
    if (!QStandardPaths::findExecutable(QStringLiteral("wt")).isEmpty())
        return QStringLiteral("Windows Terminal");
    return QStringLiteral("Command Prompt");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("Terminal");
#else
    return QStringLiteral("terminal");
#endif
}

bool TerminalLauncher::openTerminal(const QString &workingDirectory)
{
    if (workingDirectory.isEmpty() || !QDir(workingDirectory).exists())
        return false;
#ifdef Q_OS_WIN
    // Prefer Windows Terminal; fall back to Command Prompt. Arguments go
    // to the process directly, never through a shell string.
    if (!QStandardPaths::findExecutable(QStringLiteral("wt")).isEmpty())
        return QProcess::startDetached(QStringLiteral("wt"), {QStringLiteral("-d"), workingDirectory});
    return QProcess::startDetached(QStringLiteral("cmd.exe"),
                                   {QStringLiteral("/K"), QStringLiteral("cd"), QStringLiteral("/d"), workingDirectory},
                                   workingDirectory);
#elif defined(Q_OS_MACOS)
    return QProcess::startDetached(QStringLiteral("open"),
                                   {QStringLiteral("-a"), QStringLiteral("Terminal"), workingDirectory});
#else
    const QString terminal = QStandardPaths::findExecutable(QStringLiteral("x-terminal-emulator"));
    if (!terminal.isEmpty())
        return QProcess::startDetached(terminal, {}, workingDirectory);
    const QString gnome = QStandardPaths::findExecutable(QStringLiteral("gnome-terminal"));
    if (!gnome.isEmpty())
        return QProcess::startDetached(gnome, {QStringLiteral("--working-directory"), workingDirectory});
    return false;
#endif
}

} // namespace Guit
