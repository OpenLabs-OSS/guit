#pragma once

#include "BranchesPage.h"
#include "ChangesPage.h"
#include "HistoryPage.h"
#include "OverviewPage.h"
#include "Sidebar.h"
#include "../app/ThemeManager.h"
#include "../controllers/BranchController.h"
#include "../controllers/ChangesController.h"
#include "../controllers/HistoryController.h"

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
                        AppSettings *settings,
                        ThemeManager *themes,
                        QWidget *parent = nullptr);

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

    RepositoryController *m_controller = nullptr;
    ChangesController *m_changes = nullptr;
    HistoryController *m_history = nullptr;
    BranchController *m_branches = nullptr;
    AppSettings *m_settings = nullptr;
    ThemeManager *m_themes = nullptr;

    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    OverviewPage *m_overview = nullptr;
    ChangesPage *m_changesPage = nullptr;
    HistoryPage *m_historyPage = nullptr;
    BranchesPage *m_branchesPage = nullptr;

    QLabel *m_repoLabel = nullptr;
    QLabel *m_branchLabel = nullptr;
    QLabel *m_gitLabel = nullptr;

    QMenu *m_recentMenu = nullptr;
    QProgressDialog *m_cloneProgress = nullptr;
};

} // namespace Guit
