#pragma once

#include <QString>

namespace Guit
{

// Opens the repository in the user's terminal. Guit complements the
// terminal instead of replacing it (see PROJECT_SPEC section 30).
class TerminalLauncher
{
public:
    TerminalLauncher() = delete;

    // Returns true when a terminal was launched. Never blocks, never
    // shells out through cmd/PowerShell string parsing on Windows beyond
    // what the terminal itself requires.
    static bool openTerminal(const QString &workingDirectory);
    static QString defaultTerminalName();
};

} // namespace Guit
