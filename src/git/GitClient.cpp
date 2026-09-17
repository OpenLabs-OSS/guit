#include "GitClient.h"

#include "GitLocator.h"
#include "GitProcess.h"
#include "../utils/GitCommandFormatter.h"

#include <QDir>
#include <QLoggingCategory>

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

void GitClient::setGitExecutableOverride(const QString &overridePath)
{
    m_override = overridePath;
    refreshExecutable();
}

void GitClient::refreshExecutable()
{
    m_gitExecutable = findGitExecutable(m_override);
}

GitVersion GitClient::version(int timeoutMs) const
{
    if (!hasGit())
        return {};
    const GitProcessResult result = GitProcess::run(m_gitExecutable, {QStringLiteral("--version")}, {}, timeoutMs);
    if (!result.isSuccess()) {
        qCWarning(guitClientLog) << "git --version failed:" << result.errorMessage;
        return {};
    }
    return GitVersion::parse(result.standardOutput);
}

GitClient::RepositoryProbe GitClient::probeRepository(const QString &path, int timeoutMs) const
{
    RepositoryProbe probe;
    if (!hasGit() || path.isEmpty())
        return probe;

    // `rev-parse --show-toplevel` prints the working-tree root and fails
    // outside a repository. `--absolute-git-dir` resolves the .git location.
    GitProcessResult topLevel = GitProcess::run(
        m_gitExecutable, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")}, path, timeoutMs);
    if (!topLevel.isSuccess()) {
        // Fall back to bare-repository detection before giving up.
        GitProcessResult gitDir = GitProcess::run(
            m_gitExecutable, {QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")}, path, timeoutMs);
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
        m_gitExecutable, {QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")}, path, timeoutMs);
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
    if (!hasGit()) {
        GitProcessResult result;
        result.error = GitError::GitNotFound;
        result.errorMessage = gitErrorMessage(GitError::GitNotFound);
        return result;
    }
    return GitProcess::run(m_gitExecutable, arguments, workingDirectory, timeoutMs);
}

QString GitClient::equivalentCommand(const QStringList &arguments) const
{
    return GitCommandFormatter::format(arguments);
}

} // namespace Guit
