#pragma once

#include "../app/AppSettings.h"
#include "../controllers/RepositoryInfoController.h"

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace Guit
{

// Repository dashboard: Git availability, branch/HEAD facts, repository
// information, LFS, submodules, worktrees, reflog (advanced), .gitignore
// and terminal shortcuts, plus recent repositories.
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    struct Data
    {
        bool gitAvailable = false;
        QString gitVersion;
        QString gitExecutable;
        bool hasRepository = false;
        QString rootPath;
        QString branchDisplay;
        QString headDisplay;
        QString commitHash;
        QStringList recentRepositories;
    };

    explicit OverviewPage(RepositoryInfoController *info, AppSettings *settings, QWidget *parent = nullptr);

    void setData(const Data &data);

public slots:
    void refresh();
    void applyMode();
    void setLoading(bool loading);

signals:
    void openRequested();
    void recentOpened(const QString &path);
    void terminalRequested();

private slots:
    void onInfoChanged(const RepoInfo &info, const LfsInfo &lfs,
                       const QList<SubmoduleInfo> &submodules,
                       const QList<WorktreeInfo> &worktrees,
                       const QList<ReflogEntry> &reflog,
                       const QString &gitignore);
    void onOpenGitignore();
    void onAddWorktree();
    void onRemoveWorktree();
    void onTrackLfs();
    void onUpdateSubmodules();
    void onSyncSubmodules();

private:
    static QString humanSize(qlonglong bytes);

    RepositoryInfoController *m_info = nullptr;
    AppSettings *m_settings = nullptr;

    QLabel *m_gitBanner = nullptr;
    QLabel *m_repoTitle = nullptr;
    QLabel *m_repoDetails = nullptr;
    QLabel *m_commandLabel = nullptr;
    QPushButton *m_openButton = nullptr;
    QPushButton *m_terminalButton = nullptr;
    QPushButton *m_gitignoreButton = nullptr;
    QListWidget *m_recentList = nullptr;
    QLabel *m_recentLabel = nullptr;

    QGroupBox *m_factsBox = nullptr;
    QLabel *m_factsLabel = nullptr;
    QGroupBox *m_lfsBox = nullptr;
    QLabel *m_lfsLabel = nullptr;
    QLineEdit *m_lfsPattern = nullptr;
    QGroupBox *m_submoduleBox = nullptr;
    QListWidget *m_submoduleList = nullptr;
    QGroupBox *m_worktreeBox = nullptr;
    QListWidget *m_worktreeList = nullptr;
    QGroupBox *m_reflogBox = nullptr;
    QListWidget *m_reflogList = nullptr;
    QProgressBar *m_loadingBar = nullptr;
    QList<QPushButton *> m_actionButtons;
    QString m_gitignoreContent;
};

} // namespace Guit
