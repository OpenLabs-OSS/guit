#include "RepositoryController.h"

#include "../app/AppSettings.h"

namespace Guit
{

RepositoryController::RepositoryController(GitRepository *repository,
                                           AppSettings *settings,
                                           QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_settings(settings)
{
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

} // namespace Guit
