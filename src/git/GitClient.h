#pragma once

#include "GitResult.h"
#include "GitVersion.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace Guit
{

// Higher-level Git API used by GitRepository. It exposes clean C++
// operations over the raw process layer and always uses machine-readable
// Git output (porcelain formats, explicit --format strings) so parsing
// never depends on human-readable text that may change between versions.
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

    [[nodiscard]] QString gitExecutable() const { return m_gitExecutable; }
    [[nodiscard]] bool hasGit() const { return !m_gitExecutable.isEmpty(); }

    // Re-run executable detection (e.g. after the user changes the override).
    void setGitExecutableOverride(const QString &overridePath);
    void refreshExecutable();

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
    QString m_gitExecutable;
    QString m_override;
};

} // namespace Guit
