#pragma once

#include "GitResult.h"
#include "GitVersion.h"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>

#include <optional>

namespace Guit
{

// Higher-level Git API used by GitRepository. It exposes clean C++
// operations over the raw process layer and always uses machine-readable
// Git output (porcelain formats, explicit --format strings) so parsing
// never depends on human-readable text that may change between versions.
//
// Thread safety: all methods are safe to call from any thread. A mutex
// guards the executable path and the cached version; each run() spawns
// its own QProcess, so concurrent calls never share process state.
class GitClient : public QObject
{
    Q_OBJECT

public:
    // Outcome of probing a directory for repository membership.
    struct RepositoryProbe
    {
        bool isRepository = false;
        QString rootPath;      // absolute working-tree root (or git dir for bare repos)
        QString gitDir;        // absolute path of the .git directory
        bool isBare = false;
        GitProcessResult raw;
    };

    explicit GitClient(QObject *parent = nullptr);
    explicit GitClient(const QString &gitExecutableOverride, QObject *parent = nullptr);

    [[nodiscard]] QString gitExecutable() const;
    [[nodiscard]] bool hasGit() const;

    // Re-run executable detection (e.g. after the user changes the override).
    // Clears the cached version: a different executable may report differently.
    void setGitExecutableOverride(const QString &overridePath);
    void refreshExecutable();

    // The version is queried once and cached: the status bar asks on every
    // refresh, and spawning `git --version` each time is pure overhead.
    GitVersion version(int timeoutMs = 15000) const;

    RepositoryProbe probeRepository(const QString &path, int timeoutMs = 15000) const;
    bool isRepository(const QString &path, int timeoutMs = 15000) const;
    QString repositoryRoot(const QString &path, int timeoutMs = 15000) const;

    GitProcessResult run(const QStringList &arguments,
                         const QString &workingDirectory,
                         int timeoutMs = 30000) const;

    // The exact command shown to users for transparency.
    [[nodiscard]] QString equivalentCommand(const QStringList &arguments) const;

private:
    mutable QMutex m_mutex;
    QString m_gitExecutable;
    QString m_override;
    mutable std::optional<GitVersion> m_cachedVersion;
};

} // namespace Guit
