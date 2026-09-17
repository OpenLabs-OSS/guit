#pragma once

#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(guitAppLog)
Q_DECLARE_LOGGING_CATEGORY(guitGitLog)
Q_DECLARE_LOGGING_CATEGORY(guitUiLog)

namespace Guit::Logging
{
// Installs Guit's message handler: timestamped console output, plus an
// optional log file under the platform app-data location. Call once from
// main() before creating the QApplication's windows.
void initialize(bool logToFile = false);
QString logFilePath();
} // namespace Guit::Logging
