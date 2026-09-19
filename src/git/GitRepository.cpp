#include "GitRepository.h"

#include "GitProcess.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(guitRepoLog, "guit.git.repository")

namespace Guit
{

QString resetModeLabel(ResetMode mode)
{
    switch (mode) {
    case ResetMode::Soft:  return QStringLiteral("Soft");
    case ResetMode::Mixed: return QStringLiteral("Mixed");
    case ResetMode::Hard:  return QStringLiteral("Hard");
    }
    return {};
}

QString resetModeExplanation(ResetMode mode)
{
    switch (mode) {
    case ResetMode::Soft:
        return QStringLiteral("Move the branch pointer only. Staging area and files stay as they are.");
    case ResetMode::Mixed:
        return QStringLiteral("Move the branch pointer and reset the staging area. Files stay as they are.");
    case ResetMode::Hard:
        return QStringLiteral("Move the branch pointer and throw away all staged and unstaged changes. This cannot be undone.");
    }
    return {};
}

GitRepository::GitRepository(QObject *parent)
    : QObject(parent)
    , m_client(new GitClient(this))
    , m_network(new AsyncGitProcess(this))
{
    connect(m_network, &AsyncGitProcess::progress, this, &GitRepository::networkProgress);
    connect(m_network, &AsyncGitProcess::finished, this, [this](const GitProcessResult &result) {
        OperationResult operation;
        {
            QMutexLocker locker(&m_mutex);
            operation.command = m_networkCommand;
            if (result.isSuccess()) {
                operation.ok = true;
                operation.message = m_networkSuccessMessage;
            } else if (result.error == GitError::Cancelled) {
                operation.ok = false;
                operation.message = tr("The operation was cancelled.");
            } else {
                operation.ok = false;
                operation.message = !result.standardError.isEmpty() ? result.standardError : result.errorMessage;
            }
        }
        refreshHead();
        emit networkFinished(operation);
    });
}

QThreadPool *GitRepository::backgroundPool()
{
    // One thread: Git operations serialize, so they never fight over the
    // repository's index.lock, while the GUI thread never blocks.
    static QThreadPool *pool = []() {
        auto *created = new QThreadPool();
        created->setMaxThreadCount(1);
        return created;
    }();
    return pool;
}

RepoLocation GitRepository::snapshotLocation() const
{
    QMutexLocker locker(&m_mutex);
    RepoLocation location;
    location.valid = m_valid;
    location.rootPath = m_rootPath;
    location.gitDir = m_gitDir;
    location.bare = m_bare;
    return location;
}

void GitRepository::storeHead(const HeadInfo &head)
{
    QMutexLocker locker(&m_mutex);
    m_head = head;
}

bool GitRepository::isValid() const
{
    QMutexLocker locker(&m_mutex);
    return m_valid;
}

QString GitRepository::rootPath() const
{
    QMutexLocker locker(&m_mutex);
    return m_rootPath;
}

QString GitRepository::gitDir() const
{
    QMutexLocker locker(&m_mutex);
    return m_gitDir;
}

bool GitRepository::isBare() const
{
    QMutexLocker locker(&m_mutex);
    return m_bare;
}

HeadInfo GitRepository::head() const
{
    QMutexLocker locker(&m_mutex);
    return m_head;
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

    // Opening a `.git` directory itself resolves to its worktree, so
    // "open a folder containing .git" does what the user meant.
    QString probePath = QDir::cleanPath(path);
    if (QDir(probePath).dirName() == QStringLiteral(".git"))
        probePath = QDir(probePath).filePath(QStringLiteral(".."));

    const GitClient::RepositoryProbe probe = m_client->probeRepository(probePath);
    if (!probe.isRepository) {
        const QString reason = tr("The selected directory is not inside a Git repository.");
        qCInfo(guitRepoLog) << "Not a repository:" << path;
        emit openFailed(reason, probe.raw.errorMessage);
        return false;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_rootPath = probe.rootPath;
        m_gitDir = probe.gitDir;
        m_bare = probe.isBare;
        m_valid = true;
    }
    refreshHead();
    qCInfo(guitRepoLog) << "Opened repository:" << probe.rootPath;
    emit repositoryChanged();
    return true;
}

void GitRepository::close()
{
    bool wasValid = false;
    {
        QMutexLocker locker(&m_mutex);
        wasValid = m_valid;
        m_valid = false;
        m_bare = false;
        m_rootPath.clear();
        m_gitDir.clear();
        m_head = {};
    }
    if (wasValid)
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
    const RepoLocation location = snapshotLocation();
    HeadInfo head;
    if (location.valid) {
        // Current branch; empty output means detached HEAD or unborn branch.
        const GitProcessResult branchResult = m_client->run(
            {QStringLiteral("branch"), QStringLiteral("--show-current")}, location.rootPath);
        const QString branch = branchResult.isSuccess() ? branchResult.standardOutput.trimmed() : QString();

        const GitProcessResult headResult = m_client->run(
            {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("HEAD")}, location.rootPath);
        const bool hasHeadCommit = headResult.isSuccess();

        head.known = true;
        head.branch = branch;
        head.unborn = !hasHeadCommit;
        head.detached = hasHeadCommit && branch.isEmpty();
        head.commitHash = hasHeadCommit ? headResult.standardOutput.trimmed() : QString();
    }
    storeHead(head);
    return location.valid;
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
    const RepoLocation location = snapshotLocation();
    if (!location.valid) {
        snapshot.errorMessage = tr("No repository is open.");
        return snapshot;
    }
    const GitProcessResult result = m_client->run(
        {QStringLiteral("status"), QStringLiteral("--porcelain=v1"), QStringLiteral("--untracked-files=normal")},
        location.rootPath);
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
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (paths.isEmpty())
        return failureResult(tr("No files selected."));
    QStringList args{QStringLiteral("add"), QStringLiteral("--")};
    args.append(paths);
    return runResult(m_client->run(args, location.rootPath), args, tr("Staged %n file(s).", nullptr, paths.size()), m_client);
}

OperationResult GitRepository::unstagePaths(const QStringList &paths)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (paths.isEmpty())
        return failureResult(tr("No files selected."));
    QStringList args{QStringLiteral("restore"), QStringLiteral("--staged"), QStringLiteral("--")};
    args.append(paths);
    return runResult(m_client->run(args, location.rootPath), args, tr("Unstaged %n file(s).", nullptr, paths.size()), m_client);
}

OperationResult GitRepository::stageAll()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("add"), QStringLiteral("--all")};
    return runResult(m_client->run(args, location.rootPath), args, tr("Staged all changes."), m_client);
}

OperationResult GitRepository::unstageAll()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("restore"), QStringLiteral("--staged"), QStringLiteral("--"), QStringLiteral(".")};
    return runResult(m_client->run(args, location.rootPath), args, tr("Unstaged all changes."), m_client);
}

OperationResult GitRepository::discardEntries(const QList<FileStatusEntry> &entries)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (entries.isEmpty())
        return failureResult(tr("No files selected."));

    QStringList tracked;
    QStringList untracked;
    for (const FileStatusEntry &entry : entries) {
        if (entry.isConflicted()) {
            return failureResult(tr("“%1” has merge conflicts. Resolve it (edit the file, or use Ours/Theirs) and stage the result instead.").arg(entry.path));
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
        OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Discarded changes."), m_client);
        if (!result.ok)
            return result;
    }
    if (!untracked.isEmpty()) {
        QStringList args{QStringLiteral("clean"), QStringLiteral("--force"), QStringLiteral("--")};
        args.append(untracked);
        OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Deleted untracked file(s)."), m_client);
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
    const RepoLocation location = snapshotLocation();
    if (!location.valid) {
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
    const GitProcessResult process = m_client->run(args, location.rootPath);
    if (!process.isSuccess()) {
        result.ok = false;
        result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
        return result;
    }
    refreshHead();
    result.ok = true;
    const GitProcessResult hash = m_client->run(
        {QStringLiteral("rev-parse"), QStringLiteral("HEAD")}, location.rootPath);
    if (hash.isSuccess())
        result.commitHash = hash.standardOutput.trimmed();
    result.message = amend ? tr("Amended commit %1.").arg(result.commitHash.left(7))
                           : tr("Committed %1.").arg(result.commitHash.left(7));
    return result;
}

QList<FileDiff> GitRepository::diffUnstaged(const QString &path) const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    QStringList args{QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-ext-diff"),
                     QStringLiteral("--src-prefix=a/"), QStringLiteral("--dst-prefix=b/"), QStringLiteral("--")};
    if (!path.isEmpty())
        args.append(path);
    else
        args.append(QStringLiteral("."));
    const GitProcessResult result = m_client->run(args, location.rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

QList<FileDiff> GitRepository::diffStaged(const QString &path) const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    QStringList args{QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("--no-color"),
                     QStringLiteral("--no-ext-diff"), QStringLiteral("--")};
    if (!path.isEmpty())
        args.append(path);
    else
        args.append(QStringLiteral("."));
    const GitProcessResult result = m_client->run(args, location.rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

// --- Milestone 2: history ---------------------------------------------------

QList<CommitInfo> GitRepository::log(int maxCount) const
{
    if (maxCount <= 0)
        return {};
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("log"), QStringLiteral("--format=") + CommitInfo::logFormat(),
         QStringLiteral("--max-count=") + QString::number(maxCount)},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return CommitInfo::parseLog(result.standardOutput);
}

CommitDetails GitRepository::showCommit(const QString &hash) const
{
    CommitDetails details;
    const RepoLocation location = snapshotLocation();
    if (!location.valid) {
        details.errorMessage = tr("No repository is open.");
        return details;
    }
    if (hash.isEmpty()) {
        details.errorMessage = tr("No commit selected.");
        return details;
    }
    const GitProcessResult info = m_client->run(
        {QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=") + CommitInfo::logFormat(), hash},
        location.rootPath);
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
        location.rootPath);
    if (files.isSuccess())
        details.files = CommitDetails::parseNameStatus(files.standardOutput);

    const GitProcessResult patch = m_client->run(
        {QStringLiteral("show"), QStringLiteral("--format="), QStringLiteral("--no-color"),
         QStringLiteral("--no-ext-diff"), QStringLiteral("--unified=3"), hash},
        location.rootPath);
    if (patch.isSuccess())
        details.diffs = parseUnifiedDiff(patch.standardOutput);

    details.valid = true;
    return details;
}

// --- Milestone 2: branches --------------------------------------------------

QList<BranchInfo> GitRepository::branches() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("for-each-ref"), QStringLiteral("--format=") + BranchInfo::forEachRefFormat(),
         QStringLiteral("refs/heads"), QStringLiteral("refs/remotes")},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return BranchInfo::parseForEachRef(result.standardOutput);
}

bool GitRepository::validateBranchName(const QString &name) const
{
    if (name.trimmed().isEmpty())
        return false;
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return false;
    const GitProcessResult result = m_client->run(
        {QStringLiteral("check-ref-format"), QStringLiteral("--branch"), name.trimmed()}, location.rootPath);
    return result.isSuccess();
}

OperationResult GitRepository::createBranch(const QString &name, const QString &startPoint, bool checkout)
{
    const QString trimmed = name.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
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
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       checkout ? tr("Created and switched to branch “%1”.").arg(trimmed)
                                                : tr("Created branch “%1”.").arg(trimmed),
                                       m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::switchBranch(const QString &name)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (name.trimmed().isEmpty())
        return failureResult(tr("No branch selected."));
    // `git switch` handles local branches (moves HEAD) and remote-tracking
    // branches (creates a local tracking branch) alike. Uncommitted changes
    // that would be overwritten abort with a clear Git message we surface.
    const QStringList args{QStringLiteral("switch"), name.trimmed()};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       tr("Switched to “%1”.").arg(name.trimmed()), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::renameBranch(const QString &oldName, const QString &newName)
{
    const QString trimmed = newName.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (!validateBranchName(trimmed))
        return failureResult(tr("“%1” is not a valid branch name.").arg(newName));
    const QStringList args{QStringLiteral("branch"), QStringLiteral("--move"), oldName, trimmed};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       tr("Renamed branch to “%1”.").arg(trimmed), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::deleteBranch(const QString &name, bool force)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (head().branch == name)
        return failureResult(tr("Cannot delete “%1” while it is checked out. Switch to another branch first.").arg(name));
    const QStringList args{QStringLiteral("branch"), force ? QStringLiteral("-D") : QStringLiteral("-d"), name};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       force ? tr("Force-deleted branch “%1”.").arg(name)
                                             : tr("Deleted branch “%1”.").arg(name),
                                       m_client);
    return result;
}

AheadBehind GitRepository::aheadBehind(const QString &from, const QString &to) const
{
    AheadBehind result;
    if (from.isEmpty() || to.isEmpty())
        return result;
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return result;
    const GitProcessResult process = m_client->run(
        {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"),
         from + QStringLiteral("...") + to},
        location.rootPath);
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
    if (from.isEmpty() || to.isEmpty())
        return {};
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-ext-diff"),
         from + QStringLiteral("...") + to},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

// --- Repository setup -------------------------------------------------------

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

// --- Milestone 3: remotes ---------------------------------------------------

QList<RemoteInfo> GitRepository::remotes() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run({QStringLiteral("remote"), QStringLiteral("-v")}, location.rootPath);
    if (!result.isSuccess())
        return {};
    return RemoteInfo::parseVerboseList(result.standardOutput);
}

OperationResult GitRepository::addRemote(const QString &name, const QString &url)
{
    const QString trimmedName = name.trimmed();
    const QString trimmedUrl = url.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (trimmedName.isEmpty() || trimmedUrl.isEmpty())
        return failureResult(tr("Remote name and URL are required."));
    const QStringList args{QStringLiteral("remote"), QStringLiteral("add"), trimmedName, trimmedUrl};
    return runResult(m_client->run(args, location.rootPath), args, tr("Added remote “%1”.").arg(trimmedName), m_client);
}

OperationResult GitRepository::removeRemote(const QString &name)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("remote"), QStringLiteral("remove"), name};
    return runResult(m_client->run(args, location.rootPath), args, tr("Removed remote “%1”.").arg(name), m_client);
}

OperationResult GitRepository::renameRemote(const QString &oldName, const QString &newName)
{
    const QString trimmed = newName.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (trimmed.isEmpty())
        return failureResult(tr("Enter a new remote name."));
    const QStringList args{QStringLiteral("remote"), QStringLiteral("rename"), oldName, trimmed};
    return runResult(m_client->run(args, location.rootPath), args, tr("Renamed remote to “%1”.").arg(trimmed), m_client);
}

OperationResult GitRepository::setRemoteUrl(const QString &name, const QString &url)
{
    const QString trimmedUrl = url.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (trimmedUrl.isEmpty())
        return failureResult(tr("Enter a URL."));
    const QStringList args{QStringLiteral("remote"), QStringLiteral("set-url"), name, trimmedUrl};
    return runResult(m_client->run(args, location.rootPath), args, tr("Updated the URL of remote “%1”.").arg(name), m_client);
}

// --- Milestone 3: network operations (asynchronous) --------------------------

void GitRepository::startNetwork(const QStringList &args, const QString &successMessage)
{
    // Network operations stay on the GUI thread (AsyncGitProcess needs its
    // event loop); only quick member reads happen here.
    const RepoLocation location = snapshotLocation();
    if (!location.valid || !m_client->hasGit() || m_network->isRunning())
        return;
    m_networkCommand = m_client->equivalentCommand(args);
    m_networkSuccessMessage = successMessage;
    // Git handles authentication itself via credential helpers; Guit never
    // sees passwords or tokens. Progress goes to stderr (--progress).
    m_network->start(m_client->gitExecutable(), args, location.rootPath, 30 * 60 * 1000);
    emit networkStarted(m_networkCommand);
}

void GitRepository::startFetch(const QString &remote, bool prune)
{
    QStringList args{QStringLiteral("fetch"), QStringLiteral("--progress")};
    if (prune)
        args.append(QStringLiteral("--prune"));
    if (!remote.trimmed().isEmpty())
        args.append(remote.trimmed());
    const QString what = remote.trimmed().isEmpty() ? tr("default remote") : QStringLiteral("“%1”").arg(remote.trimmed());
    startNetwork(args, tr("Fetched from %1.").arg(what));
}

void GitRepository::startPull(const QString &remote)
{
    QStringList args{QStringLiteral("pull"), QStringLiteral("--progress")};
    if (!remote.trimmed().isEmpty())
        args.append(remote.trimmed());
    const QString what = remote.trimmed().isEmpty() ? tr("default remote") : QStringLiteral("“%1”").arg(remote.trimmed());
    startNetwork(args, tr("Pulled from %1.").arg(what));
}

void GitRepository::startPush(const QString &remote, const QString &source, bool setUpstream, bool force, bool deleteRemote)
{
    QStringList args{QStringLiteral("push"), QStringLiteral("--progress")};
    if (deleteRemote) {
        if (remote.trimmed().isEmpty() || source.trimmed().isEmpty())
            return;
        args.append(QStringLiteral("--delete"));
        args.append(remote.trimmed());
        args.append(source.trimmed());
        startNetwork(args, tr("Deleted “%1” from “%2”.").arg(source.trimmed(), remote.trimmed()));
        return;
    }
    // --force-with-lease refuses to overwrite work it has not seen, unlike
    // a bare --force. The UI confirms before using it.
    if (force)
        args.append(QStringLiteral("--force-with-lease"));
    if (setUpstream)
        args.append(QStringLiteral("--set-upstream"));
    if (!remote.trimmed().isEmpty())
        args.append(remote.trimmed());
    if (!source.trimmed().isEmpty())
        args.append(source.trimmed());
    startNetwork(args, tr("Pushed successfully."));
}

void GitRepository::cancelNetworkOperation()
{
    m_network->cancel();
}

bool GitRepository::isNetworkRunning() const
{
    return m_network->isRunning();
}

// --- Milestone 3: tags ------------------------------------------------------

QList<TagInfo> GitRepository::tags() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("for-each-ref"), QStringLiteral("--format=") + TagInfo::forEachRefFormat(),
         QStringLiteral("--sort=-creatordate"), QStringLiteral("refs/tags")},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return TagInfo::parseForEachRef(result.standardOutput);
}

OperationResult GitRepository::createTag(const QString &name, const QString &message, const QString &target)
{
    const QString trimmed = name.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (trimmed.isEmpty())
        return failureResult(tr("Enter a tag name."));
    QStringList args{QStringLiteral("tag")};
    if (message.trimmed().isEmpty()) {
        args.append(trimmed);
    } else {
        args.append(QStringLiteral("--annotate"));
        args.append(QStringLiteral("--message"));
        args.append(message.trimmed());
        args.append(trimmed);
    }
    if (!target.trimmed().isEmpty())
        args.append(target.trimmed());
    return runResult(m_client->run(args, location.rootPath), args, tr("Created tag “%1”.").arg(trimmed), m_client);
}

OperationResult GitRepository::deleteTag(const QString &name)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("tag"), QStringLiteral("--delete"), name};
    return runResult(m_client->run(args, location.rootPath), args, tr("Deleted tag “%1”.").arg(name), m_client);
}

CommitDetails GitRepository::showTag(const QString &name) const
{
    CommitDetails details;
    const RepoLocation location = snapshotLocation();
    if (!location.valid) {
        details.errorMessage = tr("No repository is open.");
        return details;
    }
    // Peel the tag to its commit (annotated tags point at a tag object).
    const GitProcessResult peeled = m_client->run(
        {QStringLiteral("rev-parse"), name + QStringLiteral("^{commit}")}, location.rootPath);
    if (!peeled.isSuccess()) {
        details.errorMessage = tr("Tag “%1” was not found.").arg(name);
        return details;
    }
    return showCommit(peeled.standardOutput.trimmed());
}

// --- Milestone 3: stash -----------------------------------------------------

QList<StashInfo> GitRepository::stashList() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run({QStringLiteral("stash"), QStringLiteral("list")}, location.rootPath);
    if (!result.isSuccess())
        return {};
    return StashInfo::parseList(result.standardOutput);
}

OperationResult GitRepository::stashPush(const QString &message, bool includeUntracked)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    // A stash shelves uncommitted changes so you can switch context and
    // restore them later with apply/pop.
    QStringList args{QStringLiteral("stash"), QStringLiteral("push")};
    if (includeUntracked)
        args.append(QStringLiteral("--include-untracked"));
    if (!message.trimmed().isEmpty()) {
        args.append(QStringLiteral("--message"));
        args.append(message.trimmed());
    }
    return runResult(m_client->run(args, location.rootPath), args, tr("Stashed the working-tree changes."), m_client);
}

OperationResult GitRepository::stashApply(const QString &stashRef)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("stash"), QStringLiteral("apply"), stashRef};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       tr("Applied %1 (kept in the stash).").arg(stashRef), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::stashPop(const QString &stashRef)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("stash"), QStringLiteral("pop"), stashRef};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       tr("Restored %1 and removed it from the stash.").arg(stashRef), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::stashDrop(const QString &stashRef)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("stash"), QStringLiteral("drop"), stashRef};
    return runResult(m_client->run(args, location.rootPath), args, tr("Dropped %1.").arg(stashRef), m_client);
}

OperationResult GitRepository::stashClear()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("stash"), QStringLiteral("clear")};
    return runResult(m_client->run(args, location.rootPath), args, tr("Cleared all stashes."), m_client);
}

QList<FileDiff> GitRepository::stashShow(const QString &stashRef) const
{
    if (stashRef.isEmpty())
        return {};
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("stash"), QStringLiteral("show"), QStringLiteral("--no-color"),
         QStringLiteral("--no-ext-diff"), QStringLiteral("-p"), stashRef},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return parseUnifiedDiff(result.standardOutput);
}

// --- Milestone 3: merge -----------------------------------------------------

OperationResult GitRepository::mergeBranch(const QString &name, bool noFastForward)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (name.trimmed().isEmpty())
        return failureResult(tr("No branch selected."));
    // A merge joins two histories. Fast-forward just moves the pointer;
    // --no-ff always records an explicit merge commit.
    QStringList args{QStringLiteral("merge")};
    if (noFastForward)
        args.append(QStringLiteral("--no-ff"));
    args.append(name.trimmed());
    const GitProcessResult process = m_client->run(args, location.rootPath);
    OperationResult result;
    result.command = m_client->equivalentCommand(args);
    refreshHead();
    if (process.isSuccess()) {
        result.ok = true;
        result.message = tr("Merged “%1”.").arg(name.trimmed());
        return result;
    }
    // A conflicting merge is not an error to hide: MERGE_HEAD is left
    // behind and the UI switches to conflict resolution.
    if (operationState().merging) {
        result.ok = false;
        result.conflict = true;
        result.message = tr("“%1” conflicts with the current branch. Resolve the conflicted files, then continue the merge.").arg(name.trimmed());
        return result;
    }
    result.ok = false;
    result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
    return result;
}

OperationResult GitRepository::mergeAbort()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("merge"), QStringLiteral("--abort")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Aborted the merge."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::mergeContinue()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("-c"), QStringLiteral("core.editor=true"), QStringLiteral("merge"),
                           QStringLiteral("--continue")};
    // --continue reuses the in-progress message (MERGE_MSG). Pinning the
    // editor to `true` guarantees no interactive editor can ever hang the
    // UI or the tests when Git decides a message needs a look.
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Completed the merge."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

// --- Milestone 3: rebase ----------------------------------------------------

OperationResult GitRepository::rebaseOnto(const QString &branch)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (branch.trimmed().isEmpty())
        return failureResult(tr("No branch selected."));
    // A rebase replays local commits on top of another branch. Unlike a
    // merge it rewrites history, so it needs the explicit explanation and
    // confirmation the UI provides.
    const QStringList args{QStringLiteral("rebase"), branch.trimmed()};
    const GitProcessResult process = m_client->run(args, location.rootPath);
    OperationResult result;
    result.command = m_client->equivalentCommand(args);
    refreshHead();
    if (process.isSuccess()) {
        result.ok = true;
        result.message = tr("Rebased onto “%1”.").arg(branch.trimmed());
        return result;
    }
    if (operationState().rebasing) {
        result.ok = false;
        result.conflict = true;
        result.message = tr("Rebase stopped at a conflict. Resolve the conflicted files, then continue.");
        return result;
    }
    result.ok = false;
    result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
    return result;
}

OperationResult GitRepository::rebaseContinue()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("-c"), QStringLiteral("core.editor=true"), QStringLiteral("rebase"),
                           QStringLiteral("--continue")};
    const GitProcessResult process = m_client->run(args, location.rootPath);
    OperationResult result;
    result.command = m_client->equivalentCommand(args);
    refreshHead();
    if (process.isSuccess()) {
        result.ok = true;
        result.message = tr("Rebase continued.");
        return result;
    }
    if (operationState().rebasing) {
        result.ok = false;
        result.conflict = true;
        result.message = tr("Still conflicts to resolve before the rebase can continue.");
        return result;
    }
    result.ok = false;
    result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
    return result;
}

OperationResult GitRepository::rebaseSkip()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("rebase"), QStringLiteral("--skip")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Skipped the current commit."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::rebaseAbort()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("rebase"), QStringLiteral("--abort")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Aborted the rebase."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

// --- Milestone 3: reset / revert / cherry-pick --------------------------------

OperationResult GitRepository::resetTo(const QString &target, ResetMode mode)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (target.trimmed().isEmpty())
        return failureResult(tr("No target selected."));
    const QString flag = (mode == ResetMode::Soft) ? QStringLiteral("--soft")
        : (mode == ResetMode::Hard)                 ? QStringLiteral("--hard")
                                                    : QStringLiteral("--mixed");
    const QStringList args{QStringLiteral("reset"), flag, target.trimmed()};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       tr("Reset to %1 (%2).").arg(target.trimmed(), resetModeLabel(mode)), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::revertCommit(const QString &hash)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (hash.trimmed().isEmpty())
        return failureResult(tr("No commit selected."));
    // A revert records a NEW commit that undoes the selected one; history
    // is never rewritten.
    const QStringList args{QStringLiteral("revert"), QStringLiteral("--no-edit"), hash.trimmed()};
    const GitProcessResult process = m_client->run(args, location.rootPath);
    OperationResult result;
    result.command = m_client->equivalentCommand(args);
    refreshHead();
    if (process.isSuccess()) {
        result.ok = true;
        result.message = tr("Reverted %1.").arg(hash.left(7));
        return result;
    }
    if (operationState().reverting) {
        result.ok = false;
        result.conflict = true;
        result.message = tr("Revert stopped at a conflict. Resolve the conflicted files, then continue.");
        return result;
    }
    result.ok = false;
    result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
    return result;
}

OperationResult GitRepository::revertContinue()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("-c"), QStringLiteral("core.editor=true"), QStringLiteral("revert"),
                           QStringLiteral("--continue")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Completed the revert."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::revertAbort()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("revert"), QStringLiteral("--abort")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Aborted the revert."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::cherryPick(const QString &hash)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (hash.trimmed().isEmpty())
        return failureResult(tr("No commit selected."));
    const QStringList args{QStringLiteral("cherry-pick"), hash.trimmed()};
    const GitProcessResult process = m_client->run(args, location.rootPath);
    OperationResult result;
    result.command = m_client->equivalentCommand(args);
    refreshHead();
    if (process.isSuccess()) {
        result.ok = true;
        result.message = tr("Cherry-picked %1.").arg(hash.left(7));
        return result;
    }
    if (operationState().cherryPicking) {
        result.ok = false;
        result.conflict = true;
        result.message = tr("Cherry-pick stopped at a conflict. Resolve the conflicted files, then continue.");
        return result;
    }
    result.ok = false;
    result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
    return result;
}

OperationResult GitRepository::cherryPickContinue()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("-c"), QStringLiteral("core.editor=true"), QStringLiteral("cherry-pick"),
                           QStringLiteral("--continue")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Completed the cherry-pick."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

OperationResult GitRepository::cherryPickAbort()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("cherry-pick"), QStringLiteral("--abort")};
    OperationResult result = runResult(m_client->run(args, location.rootPath), args, tr("Aborted the cherry-pick."), m_client);
    if (result.ok)
        refreshHead();
    return result;
}

// --- Milestone 3: operation state + conflict resolution ------------------------

OperationState GitRepository::operationState() const
{
    OperationState state;
    const RepoLocation location = snapshotLocation();
    if (!location.valid || location.gitDir.isEmpty())
        return state;
    const QString gitDir = location.gitDir;
    state.merging = QFile::exists(QDir(gitDir).filePath(QStringLiteral("MERGE_HEAD")));
    state.rebasing = QDir(QDir(gitDir).filePath(QStringLiteral("rebase-merge"))).exists()
        || QDir(QDir(gitDir).filePath(QStringLiteral("rebase-apply"))).exists();
    state.cherryPicking = QFile::exists(QDir(gitDir).filePath(QStringLiteral("CHERRY_PICK_HEAD")));
    state.reverting = QFile::exists(QDir(gitDir).filePath(QStringLiteral("REVERT_HEAD")));
    // A merge and a cherry-pick can technically overlap; report the most
    // actionable one first.
    if (state.merging)
        state.operation = PendingOperation::Merging;
    else if (state.rebasing)
        state.operation = PendingOperation::Rebasing;
    else if (state.cherryPicking)
        state.operation = PendingOperation::CherryPicking;
    else if (state.reverting)
        state.operation = PendingOperation::Reverting;
    return state;
}

OperationResult GitRepository::resolveWithOurs(const QString &path)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (path.isEmpty())
        return failureResult(tr("No file selected."));
    // --ours/--theirs write the chosen side to the index and the file.
    // Staging explicitly afterwards guarantees the conflict markers clear.
    const QStringList takeArgs{QStringLiteral("checkout"), QStringLiteral("--ours"), QStringLiteral("--"), path};
    OperationResult result = runResult(m_client->run(takeArgs, location.rootPath), takeArgs, {}, m_client);
    if (!result.ok)
        return result;
    const QStringList addArgs{QStringLiteral("add"), QStringLiteral("--"), path};
    result = runResult(m_client->run(addArgs, location.rootPath), addArgs,
                       tr("Resolved “%1” with your changes.").arg(path), m_client);
    // Show the resolution command, not the bookkeeping `git add`.
    result.command = m_client->equivalentCommand(takeArgs);
    return result;
}

OperationResult GitRepository::resolveWithTheirs(const QString &path)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (path.isEmpty())
        return failureResult(tr("No file selected."));
    const QStringList takeArgs{QStringLiteral("checkout"), QStringLiteral("--theirs"), QStringLiteral("--"), path};
    OperationResult result = runResult(m_client->run(takeArgs, location.rootPath), takeArgs, {}, m_client);
    if (!result.ok)
        return result;
    const QStringList addArgs{QStringLiteral("add"), QStringLiteral("--"), path};
    result = runResult(m_client->run(addArgs, location.rootPath), addArgs,
                       tr("Resolved “%1” with their changes.").arg(path), m_client);
    // Show the resolution command, not the bookkeeping `git add`.
    result.command = m_client->equivalentCommand(takeArgs);
    return result;
}

// --- Milestone 4: graph + search --------------------------------------------------

QList<CommitInfo> GitRepository::logAll(int maxCount) const
{
    if (maxCount <= 0)
        return {};
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    // Explicit ref classes instead of --all: `git log --all` also walks
    // refs/stash, leaking stash commits into normal History. Branches,
    // remotes, and tags are exactly what the graph view wants; the
    // dedicated Stashes page reads the stash separately.
    const GitProcessResult result = m_client->run(
        {QStringLiteral("log"), QStringLiteral("--branches"), QStringLiteral("--remotes"), QStringLiteral("--tags"),
         QStringLiteral("--topo-order"), QStringLiteral("--format=") + CommitInfo::logFormat(),
         QStringLiteral("--max-count=") + QString::number(maxCount)},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return CommitInfo::parseLog(result.standardOutput);
}

QMap<QString, QStringList> GitRepository::refsByHash() const
{
    QMap<QString, QStringList> refs;
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return refs;
    // Peeled object first so annotated tags map to their commit.
    const GitProcessResult result = m_client->run(
        {QStringLiteral("for-each-ref"), QStringLiteral("--format=%(*objectname)%1f%(objectname)%1f%(refname:short)"),
         QStringLiteral("refs/heads"), QStringLiteral("refs/remotes"), QStringLiteral("refs/tags")},
        location.rootPath);
    if (!result.isSuccess())
        return refs;
    const QStringList lines = result.standardOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList fields = line.split(QChar(0x1F), Qt::KeepEmptyParts);
        if (fields.size() < 3)
            continue;
        const QString hash = !fields.at(0).trimmed().isEmpty() ? fields.at(0).trimmed() : fields.at(1).trimmed();
        const QString name = fields.at(2).trimmed();
        if (!hash.isEmpty() && !name.isEmpty())
            refs[hash].append(name);
    }
    return refs;
}

QList<CommitInfo> GitRepository::searchCommits(const QString &query, int maxCount) const
{
    const QString needle = query.trimmed();
    if (needle.isEmpty())
        return {};
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    QList<CommitInfo> matches;
    for (const CommitInfo &commit : logAll(maxCount)) {
        if (commit.subject.contains(needle, Qt::CaseInsensitive)
            || commit.body.contains(needle, Qt::CaseInsensitive)
            || commit.authorName.contains(needle, Qt::CaseInsensitive)
            || commit.hash.startsWith(needle, Qt::CaseInsensitive))
            matches.append(commit);
    }
    return matches;
}

// --- Milestone 4: reflog ----------------------------------------------------------

QList<ReflogEntry> GitRepository::reflog(int maxCount) const
{
    if (maxCount <= 0)
        return {};
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result = m_client->run(
        {QStringLiteral("reflog"), QStringLiteral("--format=") + ReflogEntry::logFormat(),
         QStringLiteral("-n"), QString::number(maxCount)},
        location.rootPath);
    if (!result.isSuccess())
        return {};
    return ReflogEntry::parse(result.standardOutput);
}

// --- Milestone 4: LFS / submodules / worktrees ---------------------------------------

namespace
{
QString findLfsExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("git-lfs"));
}
} // namespace

LfsInfo GitRepository::lfsInfo() const
{
    LfsInfo info;
    const QString lfs = findLfsExecutable();
    if (lfs.isEmpty())
        return info;
    const GitProcessResult version = GitProcess::run(lfs, {QStringLiteral("version")}, {}, 15000);
    info.available = version.isSuccess();
    if (info.available)
        info.version = version.standardOutput.trimmed().split(QLatin1Char('\n')).constFirst().trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return info;
    // The repo routes files through LFS when .gitattributes says so.
    QFile attributes(QDir(location.rootPath).filePath(QStringLiteral(".gitattributes")));
    if (attributes.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString content = QString::fromUtf8(attributes.readAll());
        info.enabledInRepo = content.contains(QStringLiteral("filter=lfs"));
    }
    if (info.enabledInRepo) {
        const GitProcessResult files = GitProcess::run(lfs, {QStringLiteral("ls-files")}, location.rootPath, 30000);
        if (files.isSuccess())
            info.trackedFiles = files.standardOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts).size();
    }
    return info;
}

OperationResult GitRepository::lfsTrack(const QString &pattern)
{
    const QString trimmed = pattern.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (trimmed.isEmpty())
        return failureResult(tr("Enter a file pattern, e.g. *.psd."));
    const QString lfs = findLfsExecutable();
    if (lfs.isEmpty())
        return failureResult(tr("Git LFS is not installed."));
    const QStringList args{QStringLiteral("track"), trimmed};
    const GitProcessResult process = GitProcess::run(lfs, args, location.rootPath);
    OperationResult result;
    result.command = QStringLiteral("git lfs track %1").arg(trimmed);
    if (!process.isSuccess()) {
        result.ok = false;
        result.message = !process.standardError.isEmpty() ? process.standardError : process.errorMessage;
        return result;
    }
    result.ok = true;
    result.message = tr("Tracking “%1” with Git LFS (.gitattributes updated — review and commit it).").arg(trimmed);
    return result;
}

QList<SubmoduleInfo> GitRepository::submodules() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result =
        m_client->run({QStringLiteral("submodule"), QStringLiteral("status")}, location.rootPath);
    if (!result.isSuccess())
        return {};
    return SubmoduleInfo::parseStatus(result.standardOutput);
}

OperationResult GitRepository::submoduleUpdate(bool initialize)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    QStringList args{QStringLiteral("submodule"), QStringLiteral("update")};
    if (initialize)
        args.append(QStringLiteral("--init"));
    return runResult(m_client->run(args, location.rootPath, 10 * 60 * 1000), args,
                     tr("Updated submodules."), m_client);
}

OperationResult GitRepository::submoduleSync()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("submodule"), QStringLiteral("sync")};
    return runResult(m_client->run(args, location.rootPath), args, tr("Synchronized submodule URLs."), m_client);
}

QList<WorktreeInfo> GitRepository::worktrees() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    const GitProcessResult result =
        m_client->run({QStringLiteral("worktree"), QStringLiteral("list"), QStringLiteral("--porcelain")}, location.rootPath);
    if (!result.isSuccess())
        return {};
    return WorktreeInfo::parsePorcelain(result.standardOutput);
}

OperationResult GitRepository::worktreeAdd(const QString &path, const QString &source, bool newBranch)
{
    const QString trimmedPath = path.trimmed();
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (trimmedPath.isEmpty())
        return failureResult(tr("Enter a directory for the new worktree."));
    QStringList args{QStringLiteral("worktree"), QStringLiteral("add")};
    if (newBranch) {
        if (source.trimmed().isEmpty())
            return failureResult(tr("Enter a name for the new branch."));
        args.append(QStringLiteral("-b"));
        args.append(source.trimmed());
    }
    args.append(trimmedPath);
    if (!newBranch && !source.trimmed().isEmpty())
        args.append(source.trimmed());
    OperationResult result = runResult(m_client->run(args, location.rootPath), args,
                                       tr("Added worktree at %1.").arg(trimmedPath), m_client);
    return result;
}

OperationResult GitRepository::worktreeRemove(const QString &path, bool force)
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    if (path.isEmpty())
        return failureResult(tr("No worktree selected."));
    QStringList args{QStringLiteral("worktree"), QStringLiteral("remove")};
    if (force)
        args.append(QStringLiteral("--force"));
    args.append(path);
    return runResult(m_client->run(args, location.rootPath), args, tr("Removed worktree %1.").arg(path), m_client);
}

OperationResult GitRepository::worktreePrune()
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return failureResult(tr("No repository is open."));
    const QStringList args{QStringLiteral("worktree"), QStringLiteral("prune")};
    return runResult(m_client->run(args, location.rootPath), args, tr("Pruned worktree metadata."), m_client);
}

// --- Milestone 4: repository facts + .gitignore ---------------------------------------

namespace
{
qlonglong directorySize(const QString &path, int depth = 0)
{
    // Depth cap keeps pathological repositories (nested node_modules and
    // the like inside .git are impossible, but caution is cheap).
    if (depth > 12)
        return 0;
    qlonglong total = 0;
    QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    QDirIterator dirs(path, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
    while (dirs.hasNext()) {
        dirs.next();
        total += directorySize(dirs.filePath(), depth + 1);
    }
    return total;
}
} // namespace

RepoInfo GitRepository::repositoryInfo() const
{
    RepoInfo info;
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return info;
    info.valid = true;
    info.rootPath = location.rootPath;
    info.gitDir = location.gitDir;
    info.isBare = location.bare;
    const HeadInfo head = this->head();
    info.branch = head.branch;
    info.detached = head.detached;
    info.unborn = head.unborn;
    info.headHash = head.commitHash;
    if (!head.unborn) {
        const GitProcessResult count =
            m_client->run({QStringLiteral("rev-list"), QStringLiteral("--count"), QStringLiteral("HEAD")}, location.rootPath);
        if (count.isSuccess())
            info.commitCount = count.standardOutput.trimmed().toInt();
    } else {
        info.commitCount = 0;
    }
    info.remoteCount = remotes().size();
    info.branchCount = branches().size();
    info.tagCount = tags().size();
    info.stashCount = stashList().size();
    if (!location.gitDir.isEmpty())
        info.gitDirSizeBytes = directorySize(location.gitDir);
    return info;
}

QString GitRepository::readGitignore() const
{
    const RepoLocation location = snapshotLocation();
    if (!location.valid)
        return {};
    QFile file(QDir(location.rootPath).filePath(QStringLiteral(".gitignore")));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

OperationResult GitRepository::writeGitignore(const QString &content)
{
    OperationResult result;
    result.command = tr("# edited .gitignore (file operation, no Git command)");
    const RepoLocation location = snapshotLocation();
    if (!location.valid) {
        result.message = tr("No repository is open.");
        return result;
    }
    QFile file(QDir(location.rootPath).filePath(QStringLiteral(".gitignore")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        result.message = tr("Could not write .gitignore: %1").arg(file.errorString());
        return result;
    }
    file.write(content.toUtf8());
    result.ok = true;
    result.message = tr("Saved .gitignore. Review it with git diff, then stage and commit.");
    return result;
}

QMap<QString, QStringList> GitRepository::gitignorePresets()
{
    return {
        {QStringLiteral("Windows"), {QStringLiteral("Thumbs.db"), QStringLiteral("Desktop.ini"), QStringLiteral("$RECYCLE.BIN/")}},
        {QStringLiteral("Qt / C++ build"), {QStringLiteral("build*/"), QStringLiteral("*.o"), QStringLiteral("*.obj"), QStringLiteral("moc_*.cpp"), QStringLiteral("ui_*.h"), QStringLiteral("*.user")}},
        {QStringLiteral("Python"), {QStringLiteral("__pycache__/"), QStringLiteral("*.pyc"), QStringLiteral(".venv/"), QStringLiteral("*.egg-info/")}},
        {QStringLiteral("Node"), {QStringLiteral("node_modules/"), QStringLiteral("dist/"), QStringLiteral(".env")}},
        {QStringLiteral("macOS"), {QStringLiteral(".DS_Store"), QStringLiteral(".AppleDouble")}},
    };
}

} // namespace Guit
