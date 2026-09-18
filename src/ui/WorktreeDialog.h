#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

namespace Guit
{

// Adds a linked worktree: either check out an existing branch/commit or
// create a new branch at HEAD.
class WorktreeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorktreeDialog(QWidget *parent = nullptr);

    [[nodiscard]] QString path() const;
    [[nodiscard]] QString source() const;
    [[nodiscard]] bool newBranch() const;

private slots:
    void browse();
    void validate();

private:
    QLineEdit *m_pathEdit = nullptr;
    QLineEdit *m_sourceEdit = nullptr;
    QRadioButton *m_newBranchButton = nullptr;
    QRadioButton *m_existingButton = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit
