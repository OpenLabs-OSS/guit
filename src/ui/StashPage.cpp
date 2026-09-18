#include "Theme.h"
#include "StashPage.h"

#include "StashDialog.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

namespace Guit
{

StashPage::StashPage(StashController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_stashList(new QListWidget(this))
    , m_infoLabel(new QLabel(this))
    , m_diff(new DiffViewer(this))
    , m_commandLabel(new QLabel(this))
{
    m_infoLabel->setWordWrap(true);
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    Theme::applyMono(m_commandLabel);

    auto *saveButton = new QPushButton(tr("Stash…"), this);
    saveButton->setToolTip(tr("Shelve uncommitted changes to restore later (git stash push)."));
    auto *applyButton = new QPushButton(tr("Apply"), this);
    applyButton->setToolTip(tr("Restore the entry but keep it in the stash (git stash apply)."));
    auto *popButton = new QPushButton(tr("Pop"), this);
    popButton->setToolTip(tr("Restore the entry and remove it from the stash (git stash pop)."));
    auto *dropButton = new QPushButton(tr("Drop…"), this);
    dropButton->setProperty("destructive", true);
    auto *clearButton = new QPushButton(tr("Clear All…"), this);
    clearButton->setProperty("destructive", true);
    auto *refreshButton = new QPushButton(tr("Refresh"), this);

    auto *actions = new QHBoxLayout();
    actions->addWidget(saveButton);
    actions->addWidget(applyButton);
    actions->addWidget(popButton);
    actions->addWidget(dropButton);
    actions->addWidget(clearButton);
    actions->addWidget(refreshButton);
    actions->addStretch(1);

    auto *leftPane = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(m_stashList, 1);
    leftLayout->addLayout(actions);

    auto *rightPane = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_infoLabel);
    rightLayout->addWidget(m_diff, 1);
    rightLayout->addWidget(m_commandLabel);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setSizes({300, 700});

    m_stashList->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::pageMargin(), Theme::sectionSpacing(), Theme::pageMargin(), Theme::pageMargin());
    layout->setSpacing(Theme::controlSpacing());
    layout->addWidget(splitter);

    connect(m_controller, &StashController::stashChanged, this, &StashPage::onStashChanged);
    connect(m_controller, &StashController::stashDetails, this, &StashPage::onStashDetails);
    connect(m_controller, &StashController::stashOperationDone, this,
            [this](const QString &, const QString &command) { m_commandLabel->setText(tr("Git: %1").arg(command)); });
    connect(saveButton, &QPushButton::clicked, this, &StashPage::onSave);
    connect(applyButton, &QPushButton::clicked, this, &StashPage::onApply);
    connect(popButton, &QPushButton::clicked, this, &StashPage::onPop);
    connect(dropButton, &QPushButton::clicked, this, &StashPage::onDrop);
    connect(clearButton, &QPushButton::clicked, this, &StashPage::onClear);
    connect(refreshButton, &QPushButton::clicked, this, &StashPage::refresh);
    connect(m_stashList, &QListWidget::itemSelectionChanged, this, &StashPage::onSelection);
}

void StashPage::refresh()
{
    m_controller->refresh();
}

QString StashPage::selectedRef() const
{
    const QList<QListWidgetItem *> selected = m_stashList->selectedItems();
    if (selected.isEmpty())
        return {};
    return selected.constFirst()->data(Qt::UserRole).toString();
}

void StashPage::onStashChanged(const QList<StashInfo> &entries)
{
    m_stashList->clear();
    if (entries.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No stashes. Stashing shelves uncommitted changes for later."), m_stashList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_infoLabel->clear();
        m_diff->clear();
        return;
    }
    for (const StashInfo &entry : entries) {
        auto *item = new QListWidgetItem(QStringLiteral("%1: %2").arg(entry.ref, entry.message), m_stashList);
        item->setData(Qt::UserRole, entry.ref);
    }
    m_stashList->setCurrentRow(0);
}

void StashPage::onStashDetails(const QString &stashRef, const QList<FileDiff> &diffs)
{
    m_infoLabel->setText(tr("Contents of %1:").arg(stashRef));
    m_diff->setDiffs(diffs);
}

void StashPage::onSave()
{
    StashDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_controller->save(dialog.message(), dialog.includeUntracked());
}

void StashPage::onApply()
{
    const QString ref = selectedRef();
    if (!ref.isEmpty())
        m_controller->apply(ref);
}

void StashPage::onPop()
{
    const QString ref = selectedRef();
    if (!ref.isEmpty())
        m_controller->pop(ref);
}

void StashPage::onDrop()
{
    const QString ref = selectedRef();
    if (ref.isEmpty())
        return;
    QMessageBox confirm(QMessageBox::Warning, tr("Drop stash"),
                        tr("Drop %1? The shelved changes are lost.").arg(ref),
                        QMessageBox::No | QMessageBox::Yes, this);
    confirm.button(QMessageBox::Yes)->setText(tr("Drop"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_controller->drop(ref);
}

void StashPage::onClear()
{
    QMessageBox confirm(QMessageBox::Warning, tr("Clear all stashes"),
                        tr("Delete every stash entry? This cannot be undone."),
                        QMessageBox::Cancel | QMessageBox::Yes, this);
    confirm.button(QMessageBox::Yes)->setText(tr("Clear All"));
    if (confirm.exec() != QMessageBox::Yes)
        return;
    m_controller->clear();
}

void StashPage::onSelection()
{
    const QString ref = selectedRef();
    if (!ref.isEmpty())
        m_controller->inspect(ref);
}

} // namespace Guit
