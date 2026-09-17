#pragma once

#include "GitResult.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace Guit
{

// Low-level Git process runner.
//
// Guit must use the user's installed Git executable and must never run Git
// through cmd.exe, PowerShell, or a shell string. QProcess receives the
// executable path and the argument list separately, so no shell quoting or
// shell injection is possible.
class GitProcess
{
public:
    static constexpr int defaultTimeoutMs = 60000;

    GitProcess() = delete;

    static GitProcessResult run(const QString &executable,
                                const QStringList &arguments,
                                const QString &workingDirectory = {},
                                int timeoutMs = defaultTimeoutMs);
};

// Long-running Git operations (clone, fetch, pull, push, ...) must not
// freeze the UI, so they run through this cancellable QObject wrapper.
// Milestone 1 only needs the synchronous GitProcess::run; this class exists
// so the async architecture is in place before Milestone 2/3 need it.
class AsyncGitProcess : public QObject
{
    Q_OBJECT

public:
    explicit AsyncGitProcess(QObject *parent = nullptr);
    ~AsyncGitProcess() override;

    void start(const QString &executable,
               const QStringList &arguments,
               const QString &workingDirectory = {},
               int timeoutMs = GitProcess::defaultTimeoutMs);
    void cancel();

    [[nodiscard]] bool isRunning() const;

signals:
    void finished(const Guit::GitProcessResult &result);
    void progress(const QString &text);

private:
    class Private;
    Private *d = nullptr;
};

} // namespace Guit
