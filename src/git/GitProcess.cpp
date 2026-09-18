#include "GitProcess.h"

#include <QProcess>
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(guitProcessLog, "guit.git.process")

namespace Guit
{

GitProcessResult GitProcess::run(const QString &executable,
                                 const QStringList &arguments,
                                 const QString &workingDirectory,
                                 int timeoutMs)
{
    GitProcessResult result;

    if (executable.isEmpty()) {
        result.error = GitError::GitNotFound;
        result.errorMessage = gitErrorMessage(GitError::GitNotFound);
        return result;
    }
    if (timeoutMs <= 0) {
        result.error = GitError::InvalidInput;
        result.errorMessage = QStringLiteral("Timeout must be positive.");
        return result;
    }

    QProcess process;
    process.setProgram(executable);
    process.setArguments(arguments);
    if (!workingDirectory.isEmpty())
        process.setWorkingDirectory(workingDirectory);

    // Never route Git through a shell: arguments go directly to the process.
    process.start();
    if (!process.waitForStarted()) {
        result.error = GitError::ProcessStartFailed;
        result.errorMessage = process.errorString();
        qCWarning(guitProcessLog) << "Failed to start" << executable << arguments << ":" << result.errorMessage;
        return result;
    }

    result.started = true;
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(5000);
        result.error = GitError::TimedOut;
        result.errorMessage = QStringLiteral("git %1 timed out after %2 ms.")
                                  .arg(arguments.join(QLatin1Char(' ')))
                                  .arg(timeoutMs);
        result.standardOutput = QString::fromUtf8(process.readAllStandardOutput());
        result.standardError = QString::fromUtf8(process.readAllStandardError());
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput());
    result.standardError = QString::fromUtf8(process.readAllStandardError()).trimmed();

    if (process.exitStatus() != QProcess::NormalExit) {
        result.error = GitError::Cancelled;
        result.errorMessage = QStringLiteral("The Git process crashed.");
    } else if (result.exitCode != 0) {
        result.error = GitError::CommandFailed;
        result.errorMessage = !result.standardError.isEmpty()
            ? result.standardError
            : QStringLiteral("git %1 failed with exit code %2.")
                  .arg(arguments.join(QLatin1Char(' ')))
                  .arg(result.exitCode);
    }
    return result;
}

// --- AsyncGitProcess --------------------------------------------------------

class AsyncGitProcess::Private
{
public:
    QProcess *process = nullptr;
    bool running = false;
    // Progress handlers consume the buffers as data arrives, so accumulate
    // everything: failure messages must survive in the final result.
    QString standardOutput;
    QString standardError;
};

AsyncGitProcess::AsyncGitProcess(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

AsyncGitProcess::~AsyncGitProcess()
{
    cancel();
    delete d;
}

void AsyncGitProcess::start(const QString &executable,
                            const QStringList &arguments,
                            const QString &workingDirectory,
                            int timeoutMs)
{
    cancel();
    d->process = new QProcess(this);
    d->process->setProgram(executable);
    d->process->setArguments(arguments);
    if (!workingDirectory.isEmpty())
        d->process->setWorkingDirectory(workingDirectory);

    connect(d->process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString chunk = QString::fromUtf8(d->process->readAllStandardOutput());
        d->standardOutput.append(chunk);
        emit progress(chunk);
    });
    connect(d->process, &QProcess::readyReadStandardError, this, [this]() {
        const QString chunk = QString::fromUtf8(d->process->readAllStandardError());
        d->standardError.append(chunk);
        emit progress(chunk);
    });
    connect(d->process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (!d->running || d->process == nullptr)
                    return;
                GitProcessResult result;
                result.started = true;
                result.exitCode = exitCode;
                result.standardOutput = d->standardOutput + QString::fromUtf8(d->process->readAllStandardOutput());
                result.standardError =
                    (d->standardError + QString::fromUtf8(d->process->readAllStandardError())).trimmed();
                if (exitStatus != QProcess::NormalExit) {
                    result.error = GitError::Cancelled;
                    result.errorMessage = QStringLiteral("The Git process did not exit normally.");
                } else if (exitCode != 0) {
                    result.error = GitError::CommandFailed;
                    result.errorMessage = !result.standardError.isEmpty()
                        ? result.standardError
                        : QStringLiteral("Git failed with exit code %1.").arg(exitCode);
                }
                d->running = false;
                d->process->deleteLater();
                d->process = nullptr;
                emit finished(result);
            });

    if (timeoutMs > 0) {
        QTimer *timer = new QTimer(d->process);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this]() {
            if (!d->running || d->process == nullptr)
                return;
            GitProcessResult result;
            result.started = true;
            result.error = GitError::TimedOut;
            result.errorMessage = QStringLiteral("The Git operation timed out.");
            result.standardOutput = QString::fromUtf8(d->process->readAllStandardOutput());
            result.standardError = QString::fromUtf8(d->process->readAllStandardError());
            d->running = false;
            d->process->disconnect(this);
            d->process->kill();
            d->process->waitForFinished(3000);
            d->process->deleteLater();
            d->process = nullptr;
            emit finished(result);
        });
        timer->start(timeoutMs);
    }

    d->running = true;
    d->process->start();
}

void AsyncGitProcess::cancel()
{
    if (d->process != nullptr) {
        d->process->disconnect(this);
        if (d->process->state() != QProcess::NotRunning) {
            d->process->kill();
            d->process->waitForFinished(3000);
        }
        d->process->deleteLater();
        d->process = nullptr;
    }
    d->running = false;
}

bool AsyncGitProcess::isRunning() const
{
    return d->running;
}

} // namespace Guit
