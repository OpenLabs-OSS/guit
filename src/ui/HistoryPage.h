#pragma once

#include "DiffViewer.h"
#include "../controllers/HistoryController.h"

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
    explicit HistoryPage(HistoryController *controller, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onHistoryChanged(const QList<CommitInfo> &commits);
    void onDetailsChanged(const CommitDetails &details);
    void onSelection();

private:
    static QString commitLabel(const CommitInfo &commit);

    HistoryController *m_controller = nullptr;
    QListWidget *m_commitList = nullptr;
    QLabel *m_detailsLabel = nullptr;
    QListWidget *m_filesList = nullptr;
    DiffViewer *m_diff = nullptr;
    QList<CommitInfo> m_commits;
};

} // namespace Guit
