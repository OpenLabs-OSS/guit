#pragma once

#include "DiffViewer.h"
#include "../controllers/StashController.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

namespace Guit
{

// Stash management page: shelve uncommitted changes, inspect what each
// entry holds, restore (apply/pop), and delete entries.
class StashPage : public QWidget
{
    Q_OBJECT

public:
    explicit StashPage(StashController *controller, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onStashChanged(const QList<StashInfo> &entries);
    void onStashDetails(const QString &stashRef, const QList<FileDiff> &diffs);
    void onSave();
    void onApply();
    void onPop();
    void onDrop();
    void onClear();
    void onSelection();

private:
    QString selectedRef() const;

    StashController *m_controller = nullptr;
    QListWidget *m_stashList = nullptr;
    QLabel *m_infoLabel = nullptr;
    DiffViewer *m_diff = nullptr;
    QLabel *m_commandLabel = nullptr;
};

} // namespace Guit
