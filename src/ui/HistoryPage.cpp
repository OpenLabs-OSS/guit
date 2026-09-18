#include "HistoryPage.h"

#include "ResetDialog.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItem>
#include <QVBoxLayout>

namespace Guit
{

HistoryPage::HistoryPage(HistoryController *controller, MergeController *merge, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_merge(merge)
    , m_commitList(new QListView(this))
    , m_model(new QStandardItemModel(this))
    , m_delegate(new GraphDelegate(this))
    , m_searchBox(new QLineEdit(this))
    , m_searchLabel(new QLabel(this))
    , m_detailsLabel(new QLabel(this))
    , m_filesList(new QListWidget(this))
    , m_diff(new DiffViewer(this))
{
    m_commitList->setModel(m_model);
    m_commitList->setItemDelegate(m_delegate);
    m_commitList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_commitList->setUniformItemSizes(true);
    m_searchBox->setPlaceholderText(tr("Search message, author, or hash…"));
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setToolTip(tr("Filters the loaded history. Searches commit messages, authors, and hashes."));
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *searchButton = new QPushButton(tr("Search"), this);
    auto *clearButton = new QPushButton(tr("Clear"), this);

    auto *searchRow = new QHBoxLayout();
    searchRow->addWidget(m_searchBox, 1);
    searchRow->addWidget(searchButton);
    searchRow->addWidget(clearButton);

    auto *leftPane = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addLayout(searchRow);
    leftLayout->addWidget(m_searchLabel);
    leftLayout->addWidget(m_commitList, 1);

    auto *rightPane = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_detailsLabel);
    rightLayout->addWidget(m_filesList);
    rightLayout->addWidget(m_diff, 1);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({380, 720});

    auto *cherryButton = new QPushButton(tr("Cherry-pick"), this);
    cherryButton->setToolTip(tr("Apply the selected commit onto the current branch (git cherry-pick)."));
    auto *revertButton = new QPushButton(tr("Revert"), this);
    revertButton->setToolTip(tr("Record a new commit that undoes the selected one (git revert). History is kept."));
    auto *resetButton = new QPushButton(tr("Reset here…"), this);
    resetButton->setToolTip(tr("Move the current branch to the selected commit (git reset). Can discard work."));
    auto *actions = new QHBoxLayout();
    actions->addWidget(cherryButton);
    actions->addWidget(revertButton);
    actions->addWidget(resetButton);
    actions->addStretch(1);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter, 1);
    layout->addLayout(actions);

    connect(m_controller, &HistoryController::historyChanged, this, &HistoryPage::onHistoryChanged);
    connect(m_controller, &HistoryController::searchChanged, this, &HistoryPage::onSearchChanged);
    connect(m_controller, &HistoryController::detailsChanged, this, &HistoryPage::onDetailsChanged);
    connect(m_commitList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &HistoryPage::onSelection);
    connect(searchButton, &QPushButton::clicked, this, &HistoryPage::onSearch);
    connect(m_searchBox, &QLineEdit::returnPressed, this, &HistoryPage::onSearch);
    connect(clearButton, &QPushButton::clicked, this, &HistoryPage::onClearSearch);
    connect(cherryButton, &QPushButton::clicked, this, &HistoryPage::onCherryPick);
    connect(revertButton, &QPushButton::clicked, this, &HistoryPage::onRevert);
    connect(resetButton, &QPushButton::clicked, this, &HistoryPage::onReset);
}

void HistoryPage::refresh()
{
    if (!m_searchBox->text().trimmed().isEmpty())
        m_controller->search(m_searchBox->text());
    else
        m_controller->refresh();
}

void HistoryPage::onHistoryChanged(const QList<CommitInfo> &commits,
                                   const QMap<QString, QStringList> &refs,
                                   const QList<GraphRow> &graph,
                                   const QString &headHash)
{
    m_searchLabel->clear();
    m_delegate->setRefs(refs);
    m_delegate->setGraph(graph);
    m_delegate->setHeadHash(headHash);
    showCommits(commits, false);
}

void HistoryPage::onSearchChanged(const QList<CommitInfo> &commits, const QString &query)
{
    // Search results keep chronological relevance but lose graph context
    // (parents may be missing), so they render as a flat filtered list.
    m_delegate->setRefs({});
    m_delegate->setGraph(GraphLanes::compute(commits));
    showCommits(commits, true, query);
}

void HistoryPage::showCommits(const QList<CommitInfo> &commits, bool isSearchResult, const QString &query)
{
    m_commits = commits;
    m_model->clear();
    if (commits.isEmpty()) {
        m_searchLabel->setText(isSearchResult ? tr("No commits match “%1”.").arg(query)
                                             : tr("No commits yet. Stage changes and commit to create history."));
        m_detailsLabel->clear();
        m_filesList->clear();
        m_diff->clear();
        return;
    }
    if (isSearchResult)
        m_searchLabel->setText(tr("%n result(s) for “%1”.", nullptr, commits.size()).arg(query));
    else
        m_searchLabel->clear();
    for (int row = 0; row < commits.size(); ++row) {
        const CommitInfo &commit = commits.at(row);
        auto *item = new QStandardItem(commit.subject);
        item->setEditable(false);
        item->setData(commit.hash, Qt::UserRole);
        const QString date = commit.authorDate.isValid()
            ? commit.authorDate.toString(QStringLiteral("yyyy-MM-dd hh:mm"))
            : QString();
        item->setData(QStringLiteral("%1 · %2 %3%4")
                          .arg(commit.shortHash(), commit.authorName, date,
                               commit.isMerge() ? QStringLiteral(" · merge") : QString()),
                      Qt::UserRole + 1);
        item->setToolTip(commit.hash);
        m_model->appendRow(item);
        m_delegate->setRowHash(row, commit.hash);
    }
    m_commitList->setCurrentIndex(m_model->index(0, 0));
}

void HistoryPage::onDetailsChanged(const CommitDetails &details)
{
    if (!details.valid)
        return;
    const CommitInfo &info = details.info;
    QString parents = info.parents.isEmpty() ? tr("(none — root commit)") : info.parents.join(QStringLiteral(", "));
    QString text = QStringLiteral("<b>%1</b><br>commit %2<br>%3 &lt;%4&gt; · %5<br>parents: %6")
                       .arg(info.subject.toHtmlEscaped(), info.hash, info.authorName.toHtmlEscaped(),
                            info.authorEmail.toHtmlEscaped(),
                            info.authorDate.isValid() ? info.authorDate.toString(Qt::ISODateWithMs) : tr("unknown date"),
                            parents.toHtmlEscaped());
    if (!info.body.isEmpty())
        text += QStringLiteral("<br><br>%1").arg(info.body.toHtmlEscaped());
    m_detailsLabel->setText(text);

    m_filesList->clear();
    for (const ChangedFile &file : details.files) {
        const QString label = file.oldPath.isEmpty()
            ? QStringLiteral("[%1] %2").arg(file.status, file.path)
            : QStringLiteral("[%1] %2 → %3").arg(file.status, file.oldPath, file.path);
        auto *item = new QListWidgetItem(label, m_filesList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
    if (details.files.isEmpty()) {
        auto *item = new QListWidgetItem(tr("(no file changes)"), m_filesList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
    m_diff->setDiffs(details.diffs);
}

void HistoryPage::onSelection()
{
    const QString hash = selectedHash();
    if (!hash.isEmpty())
        m_controller->selectCommit(hash);
}

QString HistoryPage::selectedHash() const
{
    const QModelIndex current = m_commitList->currentIndex();
    if (!current.isValid())
        return {};
    return m_model->data(current, Qt::UserRole).toString();
}

void HistoryPage::onSearch()
{
    m_controller->search(m_searchBox->text());
}

void HistoryPage::onClearSearch()
{
    m_searchBox->clear();
    m_controller->clearSearch();
}

void HistoryPage::onCherryPick()
{
    const QString hash = selectedHash();
    if (!hash.isEmpty())
        m_merge->cherryPick(hash);
}

void HistoryPage::onRevert()
{
    const QString hash = selectedHash();
    if (hash.isEmpty())
        return;
    QMessageBox confirm(QMessageBox::Question, tr("Revert commit"),
                        tr("Record a new commit that undoes %1?").arg(hash.left(7)),
                        QMessageBox::No | QMessageBox::Yes, this);
    confirm.setInformativeText(tr("A revert adds history instead of rewriting it — safe for shared branches."));
    confirm.button(QMessageBox::Yes)->setText(tr("Revert"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_merge->revert(hash);
}

void HistoryPage::onReset()
{
    const QString hash = selectedHash();
    if (hash.isEmpty())
        return;
    ResetDialog dialog(hash.left(7), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (dialog.mode() == ResetMode::Hard) {
        QMessageBox confirm(QMessageBox::Warning, tr("Hard reset"),
                            tr("Hard-reset the current branch to %1?").arg(hash.left(7)),
                            QMessageBox::Cancel | QMessageBox::Yes, this);
        confirm.setInformativeText(tr("All staged and unstaged changes are thrown away. This cannot be undone."));
        confirm.button(QMessageBox::Yes)->setText(tr("Hard Reset"));
        if (confirm.exec() != QMessageBox::Yes)
            return;
    }
    m_merge->reset(hash, dialog.mode());
}

} // namespace Guit
