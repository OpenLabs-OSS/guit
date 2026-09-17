#pragma once

#include "GitClient.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Current HEAD state of an open repository.
struct HeadInfo
{
    bool known = false;      // false when the repository has no HEAD info (e.g. probe failed)
    QString branch;          // current branch name, empty when detached or unborn
    bool detached = false;
    bool unborn = false;     // repository with no commits yet
    QString commitHash;      // empty when unborn
};

// Repository-level abstraction: owns the working-tree root, the .git
// location, and cached HEAD state. All Git access goes through GitClient;
// widgets must talk to this class (via RepositoryController), never to
// GitClient or QProcess directly.
class GitRepository : public QObject
{
    Q_OBJECT

public:
    explicit GitRepository(QObject *parent = nullptr);
    GitClient *client() const { return m_client; }

    // Opens any directory inside a working tree (the root is resolved).
    // Returns false and emits openFailed() when the directory is invalid.
    bool open(const QString &path);
    void close();

    [[nodiscard]] bool isValid() const { return m_valid; }
    [[nodiscard]] QString rootPath() const { return m_rootPath; }
    [[nodiscard]] QString gitDir() const { return m_gitDir; }
    [[nodiscard]] bool isBare() const { return m_bare; }
    [[nodiscard]] HeadInfo head() const { return m_head; }

    bool refreshHead();

signals:
    void repositoryChanged();
    void repositoryClosed();
    void openFailed(const QString &reason, const QString &details);

private:
    GitClient *m_client = nullptr;
    bool m_valid = false;
    bool m_bare = false;
    QString m_rootPath;
    QString m_gitDir;
    HeadInfo m_head;
};

} // namespace Guit
