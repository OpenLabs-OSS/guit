#pragma once

#include "DiffViewer.h"
#include "../controllers/ChangesController.h"
#include "../controllers/MergeController.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QWidget>

namespace Guit
{

// Working-tree page: staged vs. unstaged changes with stage/unstage,
// discard (confirmed), per-file diffs, and the commit box with amend.
class ChangesPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChangesPage(ChangesController *controller, MergeController *merge, QWidget *parent = nullptr);

public slots:
    void refresh();
    void refreshConflicts();

private slots:
    void onStatusChanged(const StatusSnapshot &snapshot);
    void onDiffLoaded(const QList<FileDiff> &diffs, ChangesController::DiffKind kind);
    void onUnstagedSelection();
    void onStagedSelection();
    void onStage();
    void onUnstage();
    void onStageAll();
    void onUnstageAll();
    void onDiscard();
    void onCommit();
    void onConflictState(const OperationState &state);
    void onUseOurs();
    void onUseTheirs();
    void onContinue();
    void onSkip();
    void onAbort();

private:
    static QString entryLabel(const FileStatusEntry &entry);
    QStringList selectedPaths(QListWidget *list) const;
    QList<FileStatusEntry> selectedEntries(QListWidget *list) const;

    ChangesController *m_controller = nullptr;
    MergeController *m_merge = nullptr;
    QWidget *m_conflictBar = nullptr;
    QLabel *m_conflictLabel = nullptr;
    QPushButton *m_skipButton = nullptr;
    QListWidget *m_unstagedList = nullptr;
    QListWidget *m_stagedList = nullptr;
    QLabel *m_unstagedLabel = nullptr;
    QLabel *m_stagedLabel = nullptr;
    DiffViewer *m_diff = nullptr;
    QLineEdit *m_subject = nullptr;
    QTextEdit *m_body = nullptr;
    QCheckBox *m_amend = nullptr;
    QPushButton *m_commitButton = nullptr;
    QLabel *m_commandLabel = nullptr;
    QList<FileStatusEntry> m_unstaged;
    QList<FileStatusEntry> m_staged;
};

} // namespace Guit
