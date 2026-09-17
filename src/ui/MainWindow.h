#pragma once

#include "OverviewPage.h"
#include "Sidebar.h"
#include "../app/ThemeManager.h"

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
                        AppSettings *settings,
                        ThemeManager *themes,
                        QWidget *parent = nullptr);

private slots:
    void onOpenRepository();
    void onOpenRecent(const QString &path);
    void onRepositoryOpened(const QString &rootPath);
    void onRepositoryClosed();
    void onOpenFailed(const QString &reason, const QString &details);
    void onRecentChanged();
    void onPageSelected(Sidebar::Page page);
    void onThemeAction(ThemeManager::Theme theme);
    void onAbout();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildMenus();
    void buildStatusBar();
    void refreshOverview();
    void updateStatusBar();
    void showError(const QString &reason, const QString &details);

    RepositoryController *m_controller = nullptr;
    AppSettings *m_settings = nullptr;
    ThemeManager *m_themes = nullptr;

    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    OverviewPage *m_overview = nullptr;

    QLabel *m_repoLabel = nullptr;
    QLabel *m_branchLabel = nullptr;
    QLabel *m_gitLabel = nullptr;

    QMenu *m_recentMenu = nullptr;
};

} // namespace Guit
