#include "TagsPage.h"

#include "TagDialog.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

namespace Guit
{

TagsPage::TagsPage(TagController *controller, const QStringList &remoteNames, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_tagList(new QListWidget(this))
    , m_infoLabel(new QLabel(this))
    , m_diff(new DiffViewer(this))
    , m_commandLabel(new QLabel(this))
    , m_remoteBox(new QComboBox(this))
{
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setStyleSheet(QStringLiteral("font-family: Consolas, monospace;"));
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    m_remoteBox->addItems(remoteNames);

    auto *newButton = new QPushButton(tr("New…"), this);
    auto *inspectButton = new QPushButton(tr("Inspect"), this);
    auto *deleteButton = new QPushButton(tr("Delete…"), this);
    auto *pushButton = new QPushButton(tr("Push"), this);
    pushButton->setToolTip(tr("Send the selected tag to the chosen remote (git push <remote> <tag>). Tags are not pushed by default."));
    auto *refreshButton = new QPushButton(tr("Refresh"), this);

    auto *actions = new QHBoxLayout();
    actions->addWidget(newButton);
    actions->addWidget(inspectButton);
    actions->addWidget(deleteButton);
    actions->addWidget(refreshButton);
    actions->addStretch(1);

    auto *pushRow = new QHBoxLayout();
    pushRow->addWidget(new QLabel(tr("Remote:"), this));
    pushRow->addWidget(m_remoteBox, 1);
    pushRow->addWidget(pushButton);

    auto *leftPane = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(m_tagList, 1);
    leftLayout->addLayout(actions);

    auto *rightPane = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_infoLabel);
    rightLayout->addWidget(m_diff, 1);
    rightLayout->addLayout(pushRow);
    rightLayout->addWidget(m_commandLabel);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setSizes({300, 700});

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    connect(m_controller, &TagController::tagsChanged, this, &TagsPage::onTagsChanged);
    connect(m_controller, &TagController::tagDetails, this, &TagsPage::onTagDetails);
    connect(m_controller, &TagController::tagOperationDone, this,
            [this](const QString &, const QString &command) { m_commandLabel->setText(tr("Git: %1").arg(command)); });
    connect(newButton, &QPushButton::clicked, this, &TagsPage::onCreate);
    connect(inspectButton, &QPushButton::clicked, this, &TagsPage::onInspect);
    connect(deleteButton, &QPushButton::clicked, this, &TagsPage::onDelete);
    connect(pushButton, &QPushButton::clicked, this, &TagsPage::onPush);
    connect(refreshButton, &QPushButton::clicked, this, &TagsPage::refresh);
    connect(m_tagList, &QListWidget::itemSelectionChanged, this, &TagsPage::onInspect);
}

void TagsPage::setRemoteNames(const QStringList &remotes)
{
    const QString previous = m_remoteBox->currentText();
    m_remoteBox->clear();
    m_remoteBox->addItems(remotes);
    if (!previous.isEmpty())
        m_remoteBox->setCurrentText(previous);
}

void TagsPage::refresh()
{
    m_controller->refresh();
}

QString TagsPage::selectedTag() const
{
    const QList<QListWidgetItem *> selected = m_tagList->selectedItems();
    if (selected.isEmpty())
        return {};
    return selected.constFirst()->data(Qt::UserRole).toString();
}

void TagsPage::onTagsChanged(const QList<TagInfo> &tags)
{
    m_tags = tags;
    m_tagList->clear();
    if (tags.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No tags yet. Tags mark releases like v1.0."), m_tagList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_infoLabel->clear();
        m_diff->clear();
        return;
    }
    for (const TagInfo &tag : tags) {
        const QString label = tag.annotated ? QStringLiteral("%1 (annotated)").arg(tag.name) : tag.name;
        auto *item = new QListWidgetItem(label, m_tagList);
        item->setData(Qt::UserRole, tag.name);
        item->setToolTip(tag.targetHash);
    }
    m_tagList->setCurrentRow(0);
}

void TagsPage::onTagDetails(const TagInfo &tag, const CommitDetails &details)
{
    QString info = tag.annotated
        ? tr("Annotated tag %1 by %2 · %3\n%4")
              .arg(tag.name, tag.tagger,
                   tag.date.isValid() ? tag.date.toString(QStringLiteral("yyyy-MM-dd")) : QString(),
                   tag.message)
        : tr("Lightweight tag %1 (points directly at a commit)").arg(tag.name);
    if (details.valid)
        info += tr("\nTarget: %1 %2").arg(details.info.shortHash(), details.info.subject);
    m_infoLabel->setText(info);
    m_diff->setDiffs(details.valid ? details.diffs : QList<FileDiff>{});
}

void TagsPage::onCreate()
{
    TagDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_controller->create(dialog.tagName(), dialog.message(), dialog.target());
}

void TagsPage::onInspect()
{
    const QString tag = selectedTag();
    if (!tag.isEmpty())
        m_controller->inspect(tag);
}

void TagsPage::onDelete()
{
    const QString tag = selectedTag();
    if (tag.isEmpty())
        return;
    QMessageBox confirm(QMessageBox::Warning, tr("Delete tag"),
                        tr("Delete tag “%1”?").arg(tag),
                        QMessageBox::No | QMessageBox::Yes, this);
    confirm.setInformativeText(tr("Only the local tag is removed. Tags already pushed stay on the remote."));
    confirm.button(QMessageBox::Yes)->setText(tr("Delete"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_controller->remove(tag);
}

void TagsPage::onPush()
{
    const QString tag = selectedTag();
    if (tag.isEmpty() || m_remoteBox->currentText().isEmpty())
        return;
    m_controller->requestPush(m_remoteBox->currentText(), tag);
    m_commandLabel->setText(tr("Git: git push %1 %2…").arg(m_remoteBox->currentText(), tag));
}

} // namespace Guit
