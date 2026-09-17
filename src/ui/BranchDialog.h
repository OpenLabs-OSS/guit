#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Guit
{

class BranchController;

// Dialog for creating (and, in rename mode, renaming) branches. The name
// is validated live with `git check-ref-format --branch` through the
// controller — the dialog itself never runs Git.
class BranchDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        Create,
        Rename
    };

    explicit BranchDialog(BranchController *controller, Mode mode, QWidget *parent = nullptr);

    [[nodiscard]] QString branchName() const;
    [[nodiscard]] QString startPoint() const;
    [[nodiscard]] bool checkout() const;

    void setOldName(const QString &oldName);

private slots:
    void validate();

private:
    BranchController *m_controller = nullptr;
    Mode m_mode = Mode::Create;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_startEdit = nullptr;
    QCheckBox *m_checkoutBox = nullptr;
    QLabel *m_hintLabel = nullptr;
    QLabel *m_oldLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit
