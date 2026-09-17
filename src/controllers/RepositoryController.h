#pragma once

#include "../git/GitRepository.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace Guit
{

class AppSettings;

// Coordinates UI actions with repository logic (UI -> Controller ->
// GitRepository -> GitClient -> GitProcess). Widgets never execute Git
// directly; they call these slots and react to these signals.
class RepositoryController : public QObject
{
    Q_OBJECT

public:
    explicit RepositoryController(GitRepository *repository,
                                  AppSettings *settings,
                                  QObject *parent = nullptr);

    [[nodiscard]] GitRepository *repository() const { return m_repository; }
    [[nodiscard]] QStringList recentRepositories() const;

public slots:
    bool openRepository(const QString &path);
    void refresh();
    void closeRepository();
    void removeRecentRepository(const QString &path);
    void clearRecentRepositories();

signals:
    void repositoryOpened(const QString &rootPath);
    void repositoryClosed();
    void openFailed(const QString &reason, const QString &details);
    void recentRepositoriesChanged();

private:
    GitRepository *m_repository = nullptr;
    AppSettings *m_settings = nullptr;
};

} // namespace Guit
