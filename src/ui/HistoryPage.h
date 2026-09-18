#pragma once

#include "DiffViewer.h"
#include "../controllers/HistoryController.h"
#include "../controllers/MergeController.h"

#include <QLabel>
#include <QListWidget>
#include <QSplitter>
#include <QWidget>

namespace Guit
{

// Commit history page: newest-first list plus full details of the
// selected commit (message, author, parents, changed files, diff).
class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPage(HistoryController *controller, MergeController *merge, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onHistoryChanged(const QList<CommitInfo> &commits);
    void onDetailsChanged(const CommitDetails &details);
    void onSelection();
    void onCherryPick();
    void onRevert();
    void onReset();

private:
    static QString commitLabel(const CommitInfo &commit);
    QString selectedHash() const;

    HistoryController *m_controller = nullptr;
    MergeController *m_merge = nullptr;
    QListWidget *m_commitList = nullptr;
    QLabel *m_detailsLabel = nullptr;
    QListWidget *m_filesList = nullptr;
    DiffViewer *m_diff = nullptr;
    QList<CommitInfo> m_commits;
};

} // namespace Guit
