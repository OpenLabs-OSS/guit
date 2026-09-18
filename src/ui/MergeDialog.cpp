#include "MergeDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Guit
{

MergeDialog::MergeDialog(const QStringList &branches, const QString &current, QWidget *parent)
    : QDialog(parent)
    , m_branchBox(new QComboBox(this))
    , m_noFfBox(new QCheckBox(tr("No fast-forward (always record a merge commit)"), this))
    , m_hintLabel(new QLabel(this))
{
    setWindowTitle(tr("Merge Branch"));
    for (const QString &branch : branches) {
        if (branch != current)
            m_branchBox->addItem(branch);
    }
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *info = new QLabel(tr("Merge joins two histories. Conflicting changes stop the merge so you can resolve them."), this);
    info->setWordWrap(true);

    auto *form = new QFormLayout();
    form->addRow(tr("Merge into %1:").arg(current.isEmpty() ? tr("the current branch") : current), m_branchBox);
    form->addRow(QString(), m_noFfBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setProperty("primary", true);
    m_okButton->setText(tr("Merge"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(info);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_branchBox, &QComboBox::currentTextChanged, this, &MergeDialog::validate);
    connect(m_noFfBox, &QCheckBox::toggled, this, &MergeDialog::validate);
    validate();
}

QString MergeDialog::branch() const
{
    return m_branchBox->currentText().trimmed();
}

bool MergeDialog::noFastForward() const
{
    return m_noFfBox->isChecked();
}

void MergeDialog::validate()
{
    const bool ok = !branch().isEmpty();
    m_okButton->setEnabled(ok);
    if (ok) {
        m_hintLabel->setText(tr("Git: git merge %1%2")
                                 .arg(noFastForward() ? QStringLiteral("--no-ff ") : QString(), branch()));
    } else {
        m_hintLabel->setText(tr("No other branch to merge."));
    }
}

} // namespace Guit
