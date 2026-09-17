#pragma once

#include <QString>

namespace Guit
{

// Locates the user's installed Git executable dynamically.
//
// Search order:
//   1. An explicit override (from Guit settings).
//   2. The GUIT_GIT_EXECUTABLE environment variable (useful for tests).
//   3. The system PATH — but only when neither of the above was specified.
//
// An explicitly specified executable that is not usable disables the PATH
// fallback: silently using a different Git than the user configured would
// hide misconfiguration instead of reporting it.
//
// Hard-coded install paths (such as "C:\Program Files\Git\bin\git.exe")
// are deliberately NOT used: Git may be installed anywhere.
QString findGitExecutable(const QString &overridePath = {});

} // namespace Guit
