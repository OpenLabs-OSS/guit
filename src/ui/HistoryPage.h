#pragma once

#include "DiffViewer.h"
#include "GraphDelegate.h"
#include "../controllers/HistoryController.h"
#include "../controllers/MergeController.h"

#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QSplitter>
#include <QStandardItemModel>
#include <QWidget>

namespace Guit
{

// Commit history page: visual commit graph with search, plus full details
// of the selected commit (message, author, parents, changed files, diff).
class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPage(HistoryController *controller, MergeController *merge, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onHistoryChanged(const QList<CommitInfo> &commits,
                          const QMap<QString, QStringList> &refs,
                          const QList<GraphRow> &graph,
                          const QString &headHash);
    void onSearchChanged(const QList<CommitInfo> &commits, const QString &query);
    void onDetailsChanged(const CommitDetails &details);
    void onSelection();
    void onSearch();
    void onClearSearch();
    void onCherryPick();
    void onRevert();
    void onReset();

private:
    void showCommits(const QList<CommitInfo> &commits, bool isSearchResult, const QString &query = {});
    QString selectedHash() const;

    HistoryController *m_controller = nullptr;
    MergeController *m_merge = nullptr;
    QListView *m_commitList = nullptr;
    QStandardItemModel *m_model = nullptr;
    GraphDelegate *m_delegate = nullptr;
    QLineEdit *m_searchBox = nullptr;
    QLabel *m_searchLabel = nullptr;
    QLabel *m_detailsLabel = nullptr;
    QListWidget *m_filesList = nullptr;
    DiffViewer *m_diff = nullptr;
    QList<CommitInfo> m_commits;
};

} // namespace Guit
