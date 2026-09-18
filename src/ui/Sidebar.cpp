#include "Sidebar.h"

#include <QListWidgetItem>
#include <QVBoxLayout>

namespace Guit
{

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
    , m_list(new QListWidget(this))
{
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setFocusPolicy(Qt::StrongFocus);

    addItem(Page::Overview);
    addItem(Page::Changes);
    addItem(Page::History);
    addItem(Page::Branches);
    addItem(Page::Tags);
    addItem(Page::Stashes);
    addItem(Page::Remotes);

    m_list->setCurrentRow(0);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        const QListWidgetItem *item = m_list->item(row);
        if (item == nullptr)
            return;
        m_current = static_cast<Page>(item->data(Qt::UserRole).toInt());
        emit pageSelected(m_current);
    });
}

void Sidebar::setCurrentPage(Page page)
{
    for (int row = 0; row < m_list->count(); ++row) {
        if (static_cast<Page>(m_list->item(row)->data(Qt::UserRole).toInt()) == page) {
            m_list->setCurrentRow(row);
            return;
        }
    }
}

QString Sidebar::pageTitle(Page page)
{
    switch (page) {
    case Page::Overview: return tr("Overview");
    case Page::Changes:  return tr("Changes");
    case Page::History:  return tr("History");
    case Page::Branches: return tr("Branches");
    case Page::Tags:     return tr("Tags");
    case Page::Stashes:  return tr("Stashes");
    case Page::Remotes:  return tr("Remotes");
    }
    return {};
}

QString Sidebar::pageExplanation(Page page)
{
    // Concise, contextual beginner explanations (AGENTS.md section 11).
    switch (page) {
    case Page::Overview:
        return tr("Repository context: where it lives, which branch is checked out, and where HEAD points.");
    case Page::Changes:
        return tr("Working tree vs. staging area. Staged changes go into the next commit; unstaged changes stay in your files.");
    case Page::History:
        return tr("Commits are snapshots of your project. Each commit has a message, an author, and a parent commit.");
    case Page::Branches:
        return tr("A branch is a movable pointer to a commit. Switching branches moves HEAD and updates your files.");
    case Page::Tags:
        return tr("Tags are fixed pointers to commits, usually marking releases like v1.0.");
    case Page::Stashes:
        return tr("A stash temporarily shelves uncommitted changes so you can switch context and restore them later.");
    case Page::Remotes:
        return tr("Remotes are other copies of this repository, e.g. origin on a server. Fetch, pull, and push exchange commits with them.");
    }
    return {};
}

void Sidebar::addItem(Page page)
{
    auto *item = new QListWidgetItem(pageTitle(page), m_list);
    item->setData(Qt::UserRole, static_cast<int>(page));
    // Navigation shortcut documented in Help → Keyboard Shortcuts.
    item->setToolTip(QStringLiteral("%1 (Alt+%2)").arg(pageExplanation(page)).arg(static_cast<int>(page) + 1));
}

} // namespace Guit
