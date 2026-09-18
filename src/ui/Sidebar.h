#pragma once

#include <QLabel>
#include <QListWidget>
#include <QWidget>

namespace Guit
{

// Left-hand navigation matching the main application layout:
// Changes / History / Branches / Tags / Stashes / Remotes plus an
// Overview page for repository context. Emits pageSelected(); the
// MainWindow owns the stacked content.
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    enum class Page
    {
        Overview,
        Changes,
        History,
        Branches,
        Tags,
        Stashes,
        Remotes
    };
    Q_ENUM(Page)

    explicit Sidebar(QWidget *parent = nullptr);

    void setCurrentPage(Page page);
    [[nodiscard]] Page currentPage() const { return m_current; }

    // Repository context shown above the navigation: directory name and
    // current branch (or detached-HEAD state).
    void setContext(const QString &repository, const QString &branch);

    static QString pageTitle(Page page);
    static QString pageExplanation(Page page);

signals:
    void pageSelected(Guit::Sidebar::Page page);

private:
    void addItem(Page page);

    QListWidget *m_list = nullptr;
    QLabel *m_repoLabel = nullptr;
    QLabel *m_branchLabel = nullptr;
    Page m_current = Page::Overview;
};

} // namespace Guit
