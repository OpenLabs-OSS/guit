#include "RepositoryController.h"

#include "../app/AppSettings.h"

#include <QDir>

namespace Guit
{

RepositoryController::RepositoryController(GitRepository *repository,
                                           AppSettings *settings,
                                           QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_settings(settings)
    , m_cloneProcess(new AsyncGitProcess(this))
{
    connect(m_cloneProcess, &AsyncGitProcess::progress, this, &RepositoryController::cloneProgress);
    connect(m_cloneProcess, &AsyncGitProcess::finished, this, [this](const GitProcessResult &result) {
        if (result.isSuccess()) {
            emit cloneFinished(m_cloneDirectory, m_cloneCommand);
            openRepository(m_cloneDirectory);
        } else if (result.error == GitError::Cancelled) {
            emit cloneFailed(tr("Clone was cancelled."), {}, m_cloneCommand);
        } else {
            const QString reason = !result.standardError.isEmpty() ? result.standardError : result.errorMessage;
            emit cloneFailed(reason, result.standardError, m_cloneCommand);
        }
    });
    connect(m_repository, &GitRepository::repositoryChanged, this, [this]() {
        if (m_settings != nullptr) {
            m_settings->addRecentRepository(m_repository->rootPath());
            emit recentRepositoriesChanged();
        }
        emit repositoryOpened(m_repository->rootPath());
    });
    connect(m_repository, &GitRepository::repositoryClosed, this, &RepositoryController::repositoryClosed);
    connect(m_repository, &GitRepository::openFailed, this, &RepositoryController::openFailed);
}

QStringList RepositoryController::recentRepositories() const
{
    return m_settings != nullptr ? m_settings->recentRepositories() : QStringList{};
}

bool RepositoryController::openRepository(const QString &path)
{
    return m_repository->open(path);
}

void RepositoryController::refresh()
{
    if (!m_repository->isValid())
        return;
    // Re-probe in case the .git directory moved or vanished, then refresh HEAD.
    const QString root = m_repository->rootPath();
    if (!m_repository->client()->isRepository(root)) {
        m_repository->close();
        return;
    }
    m_repository->refreshHead();
}

void RepositoryController::closeRepository()
{
    m_repository->close();
}

void RepositoryController::removeRecentRepository(const QString &path)
{
    if (m_settings == nullptr)
        return;
    m_settings->removeRecentRepository(path);
    emit recentRepositoriesChanged();
}

void RepositoryController::clearRecentRepositories()
{
    if (m_settings == nullptr)
        return;
    m_settings->clearRecentRepositories();
    emit recentRepositoriesChanged();
}

void RepositoryController::initRepository(const QString &path, const QString &initialBranch)
{
    const OperationResult result = m_repository->initRepository(path, initialBranch);
    if (!result.ok) {
        emit openFailed(result.message, result.command);
        return;
    }
    emit notice(result.message);
    openRepository(path);
}

void RepositoryController::cloneRepository(const QString &url, const QString &directory)
{
    if (m_cloneProcess->isRunning())
        return;
    if (!m_repository->client()->hasGit()) {
        emit cloneFailed(tr("No Git executable was found. Install Git and make sure it is on PATH."), {},
                         tr("git clone"));
        return;
    }
    const QStringList args{QStringLiteral("clone"), QStringLiteral("--progress"), url, directory};
    m_cloneDirectory = directory;
    m_cloneCommand = m_repository->client()->equivalentCommand(args);
    m_cloneProcess->start(m_repository->client()->gitExecutable(), args, QDir::homePath(), 30 * 60 * 1000);
}

void RepositoryController::cancelClone()
{
    m_cloneProcess->cancel();
}

} // namespace Guit
