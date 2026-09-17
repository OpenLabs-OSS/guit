#include "BranchDialog.h"

#include "../controllers/BranchController.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace Guit
{

BranchDialog::BranchDialog(BranchController *controller, Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_mode(mode)
    , m_nameEdit(new QLineEdit(this))
    , m_startEdit(new QLineEdit(this))
    , m_checkoutBox(new QCheckBox(tr("Check out the new branch"), this))
    , m_hintLabel(new QLabel(this))
    , m_oldLabel(new QLabel(this))
{
    setWindowTitle(mode == Mode::Create ? tr("Create Branch") : tr("Rename Branch"));
    m_checkoutBox->setChecked(true);
    m_checkoutBox->setToolTip(tr("Move HEAD to the new branch right away (git switch -c)."));
    m_hintLabel->setWordWrap(true);

    auto *form = new QFormLayout();
    if (mode == Mode::Rename) {
        form->addRow(tr("Current name:"), m_oldLabel);
        form->addRow(tr("New name:"), m_nameEdit);
    } else {
        form->addRow(tr("Branch name:"), m_nameEdit);
        m_startEdit->setPlaceholderText(tr("HEAD (default)"));
        m_startEdit->setToolTip(tr("Optional: commit or branch to start from."));
        form->addRow(tr("Start from:"), m_startEdit);
        form->addRow(QString(), m_checkoutBox);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(mode == Mode::Create ? tr("Create") : tr("Rename"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &BranchDialog::validate);
    validate();
}

QString BranchDialog::branchName() const
{
    return m_nameEdit->text().trimmed();
}

QString BranchDialog::startPoint() const
{
    return m_startEdit->text().trimmed();
}

bool BranchDialog::checkout() const
{
    return m_checkoutBox->isChecked();
}

void BranchDialog::setOldName(const QString &oldName)
{
    m_oldLabel->setText(oldName);
}

void BranchDialog::validate()
{
    const bool valid = m_controller->validateBranchName(m_nameEdit->text());
    if (valid) {
        m_hintLabel->setText(m_mode == Mode::Create ? tr("Git: git switch -c %1").arg(branchName())
                                                    : tr("Git: git branch --move %1").arg(branchName()));
        m_hintLabel->setStyleSheet(QString());
    } else {
        m_hintLabel->setText(tr("Not a valid branch name. Use letters, digits, /, -, _, and dots — "
                                "avoid spaces and ~ ^ : ? * [ \\."));
        m_hintLabel->setStyleSheet(QStringLiteral("color: #B00020;"));
    }
    if (m_okButton != nullptr)
        m_okButton->setEnabled(valid);
}

} // namespace Guit
