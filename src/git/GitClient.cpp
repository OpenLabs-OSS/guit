#include "GitClient.h"

#include "GitLocator.h"
#include "GitProcess.h"
#include "../utils/GitCommandFormatter.h"

#include <QDir>
#include <QLoggingCategory>
#include <QMutexLocker>

Q_LOGGING_CATEGORY(guitClientLog, "guit.git.client")

namespace Guit
{

GitClient::GitClient(QObject *parent)
    : QObject(parent)
    , m_gitExecutable(findGitExecutable())
{
}

GitClient::GitClient(const QString &gitExecutableOverride, QObject *parent)
    : QObject(parent)
    , m_gitExecutable(findGitExecutable(gitExecutableOverride))
    , m_override(gitExecutableOverride)
{
}

QString GitClient::gitExecutable() const
{
    QMutexLocker locker(&m_mutex);
    return m_gitExecutable;
}

bool GitClient::hasGit() const
{
    QMutexLocker locker(&m_mutex);
    return !m_gitExecutable.isEmpty();
}

void GitClient::setGitExecutableOverride(const QString &overridePath)
{
    QMutexLocker locker(&m_mutex);
    m_override = overridePath;
    m_gitExecutable = findGitExecutable(m_override);
    m_cachedVersion.reset();
}

void GitClient::refreshExecutable()
{
    QMutexLocker locker(&m_mutex);
    m_gitExecutable = findGitExecutable(m_override);
    m_cachedVersion.reset();
}

GitVersion GitClient::version(int timeoutMs) const
{
    QString executable;
    {
        QMutexLocker locker(&m_mutex);
        if (m_cachedVersion.has_value())
            return m_cachedVersion.value();
        executable = m_gitExecutable;
    }
    if (executable.isEmpty())
        return {};
    // The process runs unlocked: concurrent version() calls may query
    // twice, but never corrupt each other; the first result wins the cache.
    const GitProcessResult result = GitProcess::run(executable, {QStringLiteral("--version")}, {}, timeoutMs);
    if (!result.isSuccess()) {
        qCWarning(guitClientLog) << "git --version failed:" << result.errorMessage;
        return {};
    }
    const GitVersion parsed = GitVersion::parse(result.standardOutput);
    QMutexLocker locker(&m_mutex);
    if (!m_cachedVersion.has_value())
        m_cachedVersion = parsed;
    return m_cachedVersion.value();
}

GitClient::RepositoryProbe GitClient::probeRepository(const QString &path, int timeoutMs) const
{
    RepositoryProbe probe;
    const QString executable = gitExecutable();
    if (executable.isEmpty() || path.isEmpty())
        return probe;

    // `rev-parse --show-toplevel` prints the working-tree root and fails
    // outside a repository. `--absolute-git-dir` resolves the .git location.
    GitProcessResult topLevel = GitProcess::run(
        executable, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")}, path, timeoutMs);
    if (!topLevel.isSuccess()) {
        // Fall back to bare-repository detection before giving up.
        GitProcessResult gitDir = GitProcess::run(
            executable, {QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")}, path, timeoutMs);
        probe.raw = gitDir;
        if (!gitDir.isSuccess())
            return probe;
        const QString dir = QDir(gitDir.standardOutput.trimmed()).absolutePath();
        probe.isRepository = true;
        probe.isBare = true;
        probe.gitDir = dir;
        probe.rootPath = dir;
        return probe;
    }

    probe.isRepository = true;
    probe.rootPath = QDir(topLevel.standardOutput.trimmed()).absolutePath();

    GitProcessResult gitDir = GitProcess::run(
        executable, {QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")}, path, timeoutMs);
    if (gitDir.isSuccess())
        probe.gitDir = QDir(gitDir.standardOutput.trimmed()).absolutePath();
    probe.raw = topLevel;
    return probe;
}

bool GitClient::isRepository(const QString &path, int timeoutMs) const
{
    return probeRepository(path, timeoutMs).isRepository;
}

QString GitClient::repositoryRoot(const QString &path, int timeoutMs) const
{
    return probeRepository(path, timeoutMs).rootPath;
}

GitProcessResult GitClient::run(const QStringList &arguments,
                                const QString &workingDirectory,
                                int timeoutMs) const
{
    const QString executable = gitExecutable();
    if (executable.isEmpty()) {
        GitProcessResult result;
        result.error = GitError::GitNotFound;
        result.errorMessage = gitErrorMessage(GitError::GitNotFound);
        return result;
    }
    return GitProcess::run(executable, arguments, workingDirectory, timeoutMs);
}

QString GitClient::equivalentCommand(const QStringList &arguments) const
{
    return GitCommandFormatter::format(arguments);
}

} // namespace Guit
