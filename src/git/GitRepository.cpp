#include "GitRepository.h"

#include "GitProcess.h"

#include <QDir>
#include <QLoggingCategory>
#include <QRegularExpression>

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

QList<FileStatusEntry> StatusSnapshot::staged() const
{
    QList<FileStatusEntry> result;
    for (const FileStatusEntry &entry : entries) {
        if (entry.isStaged() && !entry.isConflicted())
            result.append(entry);
    }
    return result;
}

QList<FileStatusEntry> StatusSnapshot::unstaged() const
{
    QList<FileStatusEntry> result;
    for (const FileStatusEntry &entry : entries) {
        if (entry.isUnstaged() || entry.isConflicted())
            result.append(entry);
    }
    return result;
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

// --- Milestone 2: working tree --------------------------------------------

namespace
{

OperationResult failureResult(const QString &message, const QString &command = {})
{
    OperationResult result;
    result.ok = false;
    result.message = message;
    result.command = command;
    return result;
}

OperationResult runResult(const GitProcessResult &process,
                          const QStringList &args,
                          const QString &successMessage,
                          GitClient *client)
{
    OperationResult result;
    result.command = client->equivalentCommand(args);
    if (!process.isSuccess()) {
        result.ok = false;
        result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
        return result;
    }
    result.ok = true;
    result.message = successMessage;
    return result;
}

} // namespace

StatusSnapshot GitRepository::status() const
{
    StatusSnapshot snapshot;
    if (!m_valid) {
        snapshot.errorMessage = tr("No repository is open.");
        return snapshot;
    }
    const GitProcessResult result = m_client->run(
        {QStringLiteral("status"), QStringLiteral("--porcelain=v1"), QStringLiteral("--untracked-files=normal")},
        m_rootPath);
    if (!result.isSuccess()) {
        snapshot.errorMessage = !result.standardError.isEmpty() ? result.standardError : result.errorMessage;
        return snapshot;
    }
    snapshot.valid = true;
    snapshot.entries = parsePorcelainV1Status(result.standardOutput);
    return snapshot;
}

OperationResult GitRepository::stagePaths(const QStringList &paths)
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (paths.isEmpty())
        return failureResult(tr("No files selected."));
    QStringList args{QStringLiteral("add"), QStringLiteral("--")};
    args.append(paths);
    return runResult(m_client->run(args, m_rootPath), args, tr("Staged %n file(s).", nullptr, paths.size()), m_client);
}

OperationResult GitRepository::unstagePaths(const QStringList &paths)
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (paths.isEmpty())
        return failureResult(tr("No files selected."));
    QStringList args{QStringLiteral("restore"), QStringLiteral("--staged"), QStringLiteral("--")};
    args.append(paths);
    return runResult(m_client->run(args, m_rootPath), args, tr("Unstaged %n file(s).", nullptr, paths.size()), m_client);
}

OperationResult GitRepository::stageAll()
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("add"), QStringLiteral("--all")};
    return runResult(m_client->run(args, m_rootPath), args, tr("Staged all changes."), m_client);
}

OperationResult GitRepository::unstageAll()
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("restore"), QStringLiteral("--staged"), QStringLiteral("--"), QStringLiteral(".")};
    return runResult(m_client->run(args, m_rootPath), args, tr("Unstaged all changes."), m_client);
}

OperationResult GitRepository::discardEntries(const QList<FileStatusEntry> &entries)
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (entries.isEmpty())
        return failureResult(tr("No files selected."));

    QStringList tracked;
    QStringList untracked;
    for (const FileStatusEntry &entry : entries) {
        if (entry.isConflicted()) {
            return failureResult(tr("“%1” has merge conflicts. Resolve the conflicts first (conflict resolution arrives in Milestone 3).").arg(entry.path));
        }
        const bool isUntracked = entry.stagedState == FileState::Untracked && entry.worktreeState == FileState::Untracked;
        if (isUntracked)
            untracked.append(entry.path);
        else
            tracked.append(entry.path);
    }

    // Discard is destructive: restore tracked files to HEAD, delete
    // untracked files. Both use explicit "--" argument lists, never a shell.
    if (!tracked.isEmpty()) {
        QStringList args{QStringLiteral("restore"), QStringLiteral("--source=HEAD"), QStringLiteral("--staged"),
                         QStringLiteral("--worktree"), QStringLiteral("--")};
        args.append(tracked);
        OperationResult result = runResult(m_client->run(args, m_rootPath), args, tr("Discarded changes."), m_client);
        if (!result.ok)
            return result;
    }
    if (!untracked.isEmpty()) {
        QStringList args{QStringLiteral("clean"), QStringLiteral("--force"), QStringLiteral("--")};
        args.append(untracked);
        OperationResult result = runResult(m_client->run(args, m_rootPath), args, tr("Deleted untracked file(s)."), m_client);
        if (!result.ok)
            return result;
    }
    QStringList all = tracked + untracked;
    OperationResult done;
    done.ok = true;
    done.command = m_client->equivalentCommand({QStringLiteral("restore"), QStringLiteral("--source=HEAD"), QStringLiteral("--staged"), QStringLiteral("--worktree"), QStringLiteral("--"), QStringLiteral("<files>")});
    done.message = tr("Discarded %n file(s).", nullptr, all.size());
    return done;
}

CommitResult GitRepository::commit(const QString &subject, const QString &body, bool amend)
{
    CommitResult result;
    if (!m_valid) {
        result.message = tr("No repository is open.");
        return result;
    }
    if (subject.trimmed().isEmpty()) {
        result.message = tr("Enter a commit message first. A short summary of the change is required.");
        return result;
    }
    QStringList args{QStringLiteral("commit")};
    if (amend)
        args.append(QStringLiteral("--amend"));
    args.append(QStringLiteral("-m"));
    args.append(subject.trimmed());
    if (!body.trimmed().isEmpty()) {
        args.append(QStringLiteral("-m"));
        args.append(body.trimmed());
    }
    result.command = m_client->equivalentCommand(args);
    const GitProcessResult process = m_client->run(args, m_rootPath);
    if (!process.isSuccess()) {
        result.ok = false;
        result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
        return result;
    }
    refreshHead();
    result.ok = true;
    const GitProcessResult hash = m_client->run(
        {QStringLiteral("rev-parse"), QStringLiteral("HEAD")}, m_rootPath);
    if (hash.isSuccess())
        result.commitHash = hash.standardOutput.trimmed();
    result.message = amend ? tr("Amended commit %1.").arg(result.commitHash.left(7))
                           : tr("Committed %1.").arg(result.commitHash.left(7));
    return result;
}

QList<FileDiff> GitRepository::diffUnstaged(const QString &path) const
{
    if (!m_valid)
        return {};
    QStringList args{QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-ext-diff"),
                     QStringLiteral("--src-prefix=a/"), QStringLiteral("--dst-prefix=b/"), QStringLiteral("--")};
    if (!path.isEmpty())
        args.append(path);
    else
        args.append(QStringLiteral("."));
    const GitProcessResult result = m_client->run(args, m_rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

QList<FileDiff> GitRepository::diffStaged(const QString &path) const
{
    if (!m_valid)
        return {};
    QStringList args{QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("--no-color"),
                     QStringLiteral("--no-ext-diff"), QStringLiteral("--")};
    if (!path.isEmpty())
        args.append(path);
    else
        args.append(QStringLiteral("."));
    const GitProcessResult result = m_client->run(args, m_rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

// --- Milestone 2: history ---------------------------------------------------

QList<CommitInfo> GitRepository::log(int maxCount) const
{
    if (!m_valid || maxCount <= 0)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("log"), QStringLiteral("--format=") + CommitInfo::logFormat(),
         QStringLiteral("--max-count=") + QString::number(maxCount)},
        m_rootPath);
    if (!result.isSuccess())
        return {};
    return CommitInfo::parseLog(result.standardOutput);
}

CommitDetails GitRepository::showCommit(const QString &hash) const
{
    CommitDetails details;
    if (!m_valid) {
        details.errorMessage = tr("No repository is open.");
        return details;
    }
    if (hash.isEmpty()) {
        details.errorMessage = tr("No commit selected.");
        return details;
    }
    const GitProcessResult info = m_client->run(
        {QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=") + CommitInfo::logFormat(), hash},
        m_rootPath);
    if (!info.isSuccess()) {
        details.errorMessage = !info.standardError.isEmpty() ? info.standardError : info.errorMessage;
        return details;
    }
    const QList<CommitInfo> commits = CommitInfo::parseLog(info.standardOutput);
    if (commits.isEmpty()) {
        details.errorMessage = tr("Commit %1 was not found.").arg(hash);
        return details;
    }
    details.info = commits.constFirst();

    // NB: diff-tree is plumbing and does not detect renames unless asked.
    const GitProcessResult files = m_client->run(
        {QStringLiteral("diff-tree"), QStringLiteral("--no-commit-id"), QStringLiteral("--name-status"),
         QStringLiteral("-r"), QStringLiteral("--root"), QStringLiteral("--find-renames"), hash},
        m_rootPath);
    if (files.isSuccess())
        details.files = CommitDetails::parseNameStatus(files.standardOutput);

    const GitProcessResult patch = m_client->run(
        {QStringLiteral("show"), QStringLiteral("--format="), QStringLiteral("--no-color"),
         QStringLiteral("--no-ext-diff"), QStringLiteral("--unified=3"), hash},
        m_rootPath);
    if (patch.isSuccess())
        details.diffs = parseUnifiedDiff(patch.standardOutput);

    details.valid = true;
    return details;
}

// --- Milestone 2: branches --------------------------------------------------

QList<BranchInfo> GitRepository::branches() const
{
    if (!m_valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("for-each-ref"), QStringLiteral("--format=") + BranchInfo::forEachRefFormat(),
         QStringLiteral("refs/heads"), QStringLiteral("refs/remotes")},
        m_rootPath);
    if (!result.isSuccess())
        return {};
    return BranchInfo::parseForEachRef(result.standardOutput);
}

bool GitRepository::validateBranchName(const QString &name) const
{
    if (!m_valid || name.trimmed().isEmpty())
        return false;
    const GitProcessResult result = m_client->run(
        {QStringLiteral("check-ref-format"), QStringLiteral("--branch"), name.trimmed()}, m_rootPath);
    return result.isSuccess();
}

OperationResult GitRepository::createBranch(const QString &name, const QString &startPoint, bool checkout)
{
    const QString trimmed = name.trimmed();
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (!validateBranchName(trimmed))
        return failureResult(tr("“%1” is not a valid branch name.").arg(name));
    QStringList args;
    if (checkout)
        args = {QStringLiteral("switch"), QStringLiteral("--create"), trimmed};
    else
        args = {QStringLiteral("branch"), trimmed};
    if (!startPoint.trimmed().isEmpty())
        args.append(startPoint.trimmed());
    OperationResult result = runResult(m_client->run(args, m_rootPath), args,
                                       checkout ? tr("Created and switched to branch “%1”.").arg(trimmed)
                                                : tr("Created branch “%1”.").arg(trimmed),
                                       m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::switchBranch(const QString &name)
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (name.trimmed().isEmpty())
        return failureResult(tr("No branch selected."));
    // `git switch` handles local branches (moves HEAD) and remote-tracking
    // branches (creates a local tracking branch) alike. Uncommitted changes
    // that would be overwritten abort with a clear Git message we surface.
    const QStringList args{QStringLiteral("switch"), name.trimmed()};
    OperationResult result = runResult(m_client->run(args, m_rootPath), args,
                                       tr("Switched to “%1”.").arg(name.trimmed()), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::renameBranch(const QString &oldName, const QString &newName)
{
    const QString trimmed = newName.trimmed();
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (!validateBranchName(trimmed))
        return failureResult(tr("“%1” is not a valid branch name.").arg(newName));
    const QStringList args{QStringLiteral("branch"), QStringLiteral("--move"), oldName, trimmed};
    OperationResult result = runResult(m_client->run(args, m_rootPath), args,
                                       tr("Renamed branch to “%1”.").arg(trimmed), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::deleteBranch(const QString &name, bool force)
{
    if (!m_valid)
        return failureResult(tr("No repository is open."));
    if (head().branch == name)
        return failureResult(tr("Cannot delete “%1” while it is checked out. Switch to another branch first.").arg(name));
    const QStringList args{QStringLiteral("branch"), force ? QStringLiteral("-D") : QStringLiteral("-d"), name};
    OperationResult result = runResult(m_client->run(args, m_rootPath), args,
                                       force ? tr("Force-deleted branch “%1”.").arg(name)
                                             : tr("Deleted branch “%1”.").arg(name),
                                       m_client);
    return result;
}

AheadBehind GitRepository::aheadBehind(const QString &from, const QString &to) const
{
    AheadBehind result;
    if (!m_valid || from.isEmpty() || to.isEmpty())
        return result;
    const GitProcessResult process = m_client->run(
        {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"),
         from + QStringLiteral("...") + to},
        m_rootPath);
    if (!process.isSuccess())
        return result;
    const QStringList parts = process.standardOutput.trimmed().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (parts.size() != 2)
        return result;
    result.behind = parts.at(0).toInt();
    result.ahead = parts.at(1).toInt();
    result.valid = true;
    return result;
}

QList<FileDiff> GitRepository::compareDiff(const QString &from, const QString &to) const
{
    if (!m_valid || from.isEmpty() || to.isEmpty())
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-ext-diff"),
         from + QStringLiteral("...") + to},
        m_rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

// --- Repository setup -------------------------------------------------------

OperationResult GitRepository::initRepository(const QString &path, const QString &initialBranch)
{
    if (path.trimmed().isEmpty())
        return failureResult(tr("No directory selected."));
    QStringList args{QStringLiteral("init")};
    if (!initialBranch.trimmed().isEmpty()) {
        args.append(QStringLiteral("--initial-branch=") + initialBranch.trimmed());
    }
    args.append(path.trimmed());
    // init runs without a working directory (the target may not be a repo yet).
    OperationResult result = runResult(GitProcess::run(m_client->gitExecutable(), args, {}), args,
                                       tr("Initialized a Git repository."), m_client);
    return result;
}

} // namespace Guit
