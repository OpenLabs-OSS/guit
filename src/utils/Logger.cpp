#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

Q_LOGGING_CATEGORY(guitAppLog, "guit.app")
Q_LOGGING_CATEGORY(guitGitLog, "guit.git")
Q_LOGGING_CATEGORY(guitUiLog, "guit.ui")

namespace Guit::Logging
{
namespace
{
QMutex g_logMutex;
QFile *g_logFile = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QMutexLocker locker(&g_logMutex);
    QString level;
    switch (type) {
    case QtDebugMsg:    level = QStringLiteral("DEBUG"); break;
    case QtInfoMsg:     level = QStringLiteral("INFO"); break;
    case QtWarningMsg:  level = QStringLiteral("WARN"); break;
    case QtCriticalMsg: level = QStringLiteral("ERROR"); break;
    case QtFatalMsg:    level = QStringLiteral("FATAL"); break;
    }
    const QString line = QStringLiteral("%1 [%2] %3: %4")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
                             .arg(level, context.category ? QString::fromLatin1(context.category)
                                                          : QStringLiteral("default"))
                             .arg(message);
    QTextStream err(stderr);
    err << line << '\n';
    if (g_logFile != nullptr && g_logFile->isOpen()) {
        QTextStream fileStream(g_logFile);
        fileStream << line << '\n';
    }
    if (type == QtFatalMsg)
        abort();
}
} // namespace

QString logFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dir).filePath(QStringLiteral("guit.log"));
}

void initialize(bool logToFile)
{
    if (logToFile) {
        const QString path = logFilePath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        g_logFile = new QFile(path);
        // Never store credentials: only diagnostic messages go here, and no
        // Git operation in Guit handles credentials directly.
        g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    qInstallMessageHandler(messageHandler);
}

} // namespace Guit::Logging
