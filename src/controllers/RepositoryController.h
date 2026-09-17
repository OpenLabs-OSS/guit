#pragma once

#include "../git/GitProcess.h"
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
    void initRepository(const QString &path, const QString &initialBranch);
    // Long-running: progress via cloneProgress, result via cloneFinished /
    // cloneFailed. Cancel with cancelClone().
    void cloneRepository(const QString &url, const QString &directory);
    void cancelClone();

signals:
    void repositoryOpened(const QString &rootPath);
    void repositoryClosed();
    void openFailed(const QString &reason, const QString &details);
    void recentRepositoriesChanged();
    void cloneProgress(const QString &text);
    void cloneFinished(const QString &directory, const QString &command);
    void cloneFailed(const QString &reason, const QString &details, const QString &command);
    void notice(const QString &message);

private:
    GitRepository *m_repository = nullptr;
    AppSettings *m_settings = nullptr;
    AsyncGitProcess *m_cloneProcess = nullptr;
    QString m_cloneDirectory;
    QString m_cloneCommand;
};

} // namespace Guit
