#pragma once

#include "DiffViewer.h"
#include "../controllers/BranchController.h"

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QWidget>

namespace Guit
{

// Branch management page: local and remote-tracking branches with
// create/switch/rename/delete, plus pairwise comparison (ahead/behind
// counts and diff).
class BranchesPage : public QWidget
{
    Q_OBJECT

public:
    explicit BranchesPage(BranchController *controller, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onBranchesChanged(const QList<BranchInfo> &branches);
    void onComparisonReady(const QString &from, const QString &to,
                           const AheadBehind &aheadBehind,
                           const QList<FileDiff> &diffs);
    void onCreate();
    void onSwitch();
    void onRename();
    void onDelete();
    void onCompare();
    void onBranchFailed(const QString &reason, const QString &details, const QString &command);

private:
    QString selectedBranch() const;

    BranchController *m_controller = nullptr;
    QListWidget *m_branchList = nullptr;
    QComboBox *m_fromBox = nullptr;
    QComboBox *m_toBox = nullptr;
    QLabel *m_compareLabel = nullptr;
    DiffViewer *m_diff = nullptr;
    QLabel *m_commandLabel = nullptr;
    QList<BranchInfo> m_branches;
    QString m_pendingForceDelete;
};

} // namespace Guit
