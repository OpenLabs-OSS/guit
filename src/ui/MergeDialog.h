#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace Guit
{

// Starts a merge: pick the branch to merge into the current one. Fast-
// forward merges just move the pointer; --no-ff records a merge commit.
class MergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MergeDialog(const QStringList &branches, const QString &current, QWidget *parent = nullptr);

    [[nodiscard]] QString branch() const;
    [[nodiscard]] bool noFastForward() const;

private slots:
    void validate();

private:
    QComboBox *m_branchBox = nullptr;
    QCheckBox *m_noFfBox = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit
