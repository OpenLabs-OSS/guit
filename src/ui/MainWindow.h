#pragma once

#include "BranchesPage.h"
#include "ChangesPage.h"
#include "HistoryPage.h"
#include "OverviewPage.h"
#include "RemotesPage.h"
#include "Sidebar.h"
#include "StashPage.h"
#include "TagsPage.h"
#include "../app/ThemeManager.h"
#include "../controllers/BranchController.h"
#include "../controllers/ChangesController.h"
#include "../controllers/HistoryController.h"
#include "../controllers/MergeController.h"
#include "../controllers/RemoteController.h"
#include "../controllers/RepositoryInfoController.h"
#include "../controllers/StashController.h"
#include "../controllers/TagController.h"
#include "Toast.h"

#include <QProgressDialog>

#include <QLabel>
#include <QMainWindow>
#include <QStackedWidget>

namespace Guit
{

class AppSettings;
class RepositoryController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(RepositoryController *controller,
                        ChangesController *changes,
                        HistoryController *history,
                        BranchController *branches,
                        RemoteController *remotes,
                        TagController *tags,
                        StashController *stashes,
                        MergeController *merge,
                        RepositoryInfoController *repoInfo,
                        AppSettings *settings,
                        ThemeManager *themes,
                        QWidget *parent = nullptr);

    void showPage(Sidebar::Page page);
    void logCommand(const QString &command);

private slots:
    void onOpenRepository();
    void onCloneRepository();
    void onInitRepository();
    void onCloseRepository();
    void onOpenRecent(const QString &path);
    void onRepositoryOpened(const QString &rootPath);
    void onRepositoryClosed();
    void onOpenFailed(const QString &reason, const QString &details);
    void onRecentChanged();
    void onPageSelected(Sidebar::Page page);
    void onThemeAction(ThemeManager::Theme theme);
    void onAbout();
    void onCloneProgress(const QString &text);
    void onCloneFinished(const QString &directory, const QString &command);
    void onCloneFailed(const QString &reason, const QString &details, const QString &command);
    void onNetworkProgress(const QString &text);
    void onNetworkFinished(const OperationResult &result);
    void onMergeConflict(const QString &message, const QString &command);
    void onMergeHeadChanged();
    void onOpenTerminal();
    void onSettings();
    void onShortcuts();
    void onConcepts();
    void onAdvancedToggled(bool advanced);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildMenus();
    void buildStatusBar();
    void refreshOverview();
    void refreshCurrentPage();
    void updateStatusBar();
    void showError(const QString &reason, const QString &details);
    void showNotice(const QString &message);
    void closeNetworkProgress();

    RepositoryController *m_controller = nullptr;
    ChangesController *m_changes = nullptr;
    HistoryController *m_history = nullptr;
    BranchController *m_branches = nullptr;
    RemoteController *m_remotes = nullptr;
    TagController *m_tags = nullptr;
    StashController *m_stashes = nullptr;
    MergeController *m_merge = nullptr;
    RepositoryInfoController *m_repoInfo = nullptr;
    AppSettings *m_settings = nullptr;
    ThemeManager *m_themes = nullptr;

    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    OverviewPage *m_overview = nullptr;
    ChangesPage *m_changesPage = nullptr;
    HistoryPage *m_historyPage = nullptr;
    BranchesPage *m_branchesPage = nullptr;
    TagsPage *m_tagsPage = nullptr;
    StashPage *m_stashPage = nullptr;
    RemotesPage *m_remotesPage = nullptr;

    QLabel *m_repoLabel = nullptr;
    QLabel *m_branchLabel = nullptr;
    QLabel *m_gitLabel = nullptr;

    QMenu *m_recentMenu = nullptr;
    QProgressDialog *m_cloneProgress = nullptr;
    QProgressDialog *m_networkProgress = nullptr;
    QListWidget *m_commandList = nullptr;
    Toast *m_toast = nullptr;
};

} // namespace Guit
