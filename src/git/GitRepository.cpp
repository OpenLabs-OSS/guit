#include "GitRepository.h"

#include <QDir>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(guitRepoLog, "guit.git.repository")

namespace Guit
{

GitRepository::GitRepository(QObject *parent)
    : QObject(parent)
    , m_client(new GitClient(this))
{
}

bool GitRepository::open(const QString &path)
{
    if (path.isEmpty()) {
        emit openFailed(tr("No directory was selected."), {});
        return false;
    }

    if (!m_client->hasGit()) {
        const QString reason = tr("No Git executable was found. Install Git and make sure it is on PATH.");
        qCWarning(guitRepoLog) << reason;
        emit openFailed(reason, gitErrorMessage(GitError::GitNotFound));
        return false;
    }

    const GitClient::RepositoryProbe probe = m_client->probeRepository(path);
    if (!probe.isRepository) {
        const QString reason = tr("The selected directory is not inside a Git repository.");
        qCInfo(guitRepoLog) << "Not a repository:" << path;
        emit openFailed(reason, probe.raw.errorMessage);
        return false;
    }

    m_rootPath = probe.rootPath;
    m_gitDir = probe.gitDir;
    m_bare = probe.isBare;
    m_valid = true;
    refreshHead();
    qCInfo(guitRepoLog) << "Opened repository:" << m_rootPath;
    emit repositoryChanged();
    return true;
}

void GitRepository::close()
{
    if (!m_valid)
        return;
    m_valid = false;
    m_bare = false;
    m_rootPath.clear();
    m_gitDir.clear();
    m_head = {};
    emit repositoryClosed();
}

bool GitRepository::refreshHead()
{
    m_head = {};
    if (!m_valid)
        return false;

    // Current branch; empty output means detached HEAD or unborn branch.
    const GitProcessResult branchResult = m_client->run(
        {QStringLiteral("branch"), QStringLiteral("--show-current")}, m_rootPath);
    const QString branch = branchResult.isSuccess() ? branchResult.standardOutput.trimmed() : QString();

    const GitProcessResult headResult = m_client->run(
        {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("HEAD")}, m_rootPath);
    const bool hasHeadCommit = headResult.isSuccess();

    m_head.known = true;
    m_head.branch = branch;
    m_head.unborn = !hasHeadCommit;
    m_head.detached = hasHeadCommit && branch.isEmpty();
    m_head.commitHash = hasHeadCommit ? headResult.standardOutput.trimmed() : QString();
    return true;
}

} // namespace Guit
