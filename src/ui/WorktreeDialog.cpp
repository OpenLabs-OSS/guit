#include "WorktreeDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace Guit
{

WorktreeDialog::WorktreeDialog(QWidget *parent)
    : QDialog(parent)
    , m_pathEdit(new QLineEdit(this))
    , m_sourceEdit(new QLineEdit(this))
    , m_newBranchButton(new QRadioButton(tr("Create a new branch"), this))
    , m_existingButton(new QRadioButton(tr("Use an existing branch or commit"), this))
    , m_hintLabel(new QLabel(this))
{
    setWindowTitle(tr("Add Worktree"));
    m_newBranchButton->setChecked(true);
    m_sourceEdit->setPlaceholderText(tr("Branch name, or a commit / existing branch"));
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *browseButton = new QPushButton(tr("Browse…"), this);
    auto *pathRow = new QHBoxLayout();
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(browseButton);

    auto *info = new QLabel(tr("A worktree is a second working copy of this repository, e.g. to review another branch without switching."), this);
    info->setWordWrap(true);

    auto *form = new QFormLayout();
    form->addRow(tr("Directory:"), pathRow);
    form->addRow(m_newBranchButton);
    form->addRow(m_existingButton);
    form->addRow(tr("Name or commit:"), m_sourceEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("Add"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(info);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browseButton, &QPushButton::clicked, this, &WorktreeDialog::browse);
    connect(m_pathEdit, &QLineEdit::textChanged, this, &WorktreeDialog::validate);
    connect(m_sourceEdit, &QLineEdit::textChanged, this, &WorktreeDialog::validate);
    connect(m_newBranchButton, &QRadioButton::toggled, this, &WorktreeDialog::validate);
    validate();
}

QString WorktreeDialog::path() const
{
    return m_pathEdit->text().trimmed();
}

QString WorktreeDialog::source() const
{
    return m_sourceEdit->text().trimmed();
}

bool WorktreeDialog::newBranch() const
{
    return m_newBranchButton->isChecked();
}

void WorktreeDialog::browse()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Worktree Directory"));
    if (!dir.isEmpty())
        m_pathEdit->setText(dir);
}

void WorktreeDialog::validate()
{
    const bool ok = !path().isEmpty() && (!newBranch() || !source().isEmpty());
    m_okButton->setEnabled(ok);
    if (!ok) {
        m_hintLabel->setText(tr("Choose a directory; a new branch also needs a name."));
        return;
    }
    if (newBranch())
        m_hintLabel->setText(tr("Git: git worktree add -b %1 %2").arg(source(), path()));
    else if (source().isEmpty())
        m_hintLabel->setText(tr("Git: git worktree add %1").arg(path()));
    else
        m_hintLabel->setText(tr("Git: git worktree add %1 %2").arg(path(), source()));
}

} // namespace Guit
