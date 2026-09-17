#include "HistoryPage.h"

#include <QDateTime>
#include <QVBoxLayout>

namespace Guit
{

HistoryPage::HistoryPage(HistoryController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_commitList(new QListWidget(this))
    , m_detailsLabel(new QLabel(this))
    , m_filesList(new QListWidget(this))
    , m_diff(new DiffViewer(this))
{
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *rightPane = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_detailsLabel);
    rightLayout->addWidget(m_filesList);
    rightLayout->addWidget(m_diff, 1);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_commitList);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({350, 750});

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    connect(m_controller, &HistoryController::historyChanged, this, &HistoryPage::onHistoryChanged);
    connect(m_controller, &HistoryController::detailsChanged, this, &HistoryPage::onDetailsChanged);
    connect(m_commitList, &QListWidget::itemSelectionChanged, this, &HistoryPage::onSelection);
}

void HistoryPage::refresh()
{
    m_controller->refresh();
}

QString HistoryPage::commitLabel(const CommitInfo &commit)
{
    const QString date = commit.authorDate.isValid()
        ? commit.authorDate.toString(QStringLiteral("yyyy-MM-dd hh:mm"))
        : QString();
    return QStringLiteral("%1\n%2 · %3 %4%5")
        .arg(commit.subject, commit.shortHash(), commit.authorName, date,
             commit.isMerge() ? QStringLiteral(" · merge") : QString());
}

void HistoryPage::onHistoryChanged(const QList<CommitInfo> &commits)
{
    m_commits = commits;
    m_commitList->clear();
    if (commits.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No commits yet. Stage changes and commit to create history."), m_commitList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_detailsLabel->clear();
        m_filesList->clear();
        m_diff->clear();
        return;
    }
    for (const CommitInfo &commit : commits) {
        auto *item = new QListWidgetItem(commitLabel(commit), m_commitList);
        item->setData(Qt::UserRole, commit.hash);
        item->setToolTip(commit.hash);
    }
    m_commitList->setCurrentRow(0);
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
    const QList<QListWidgetItem *> selected = m_commitList->selectedItems();
    if (selected.isEmpty())
        return;
    m_controller->selectCommit(selected.constFirst()->data(Qt::UserRole).toString());
}

} // namespace Guit
