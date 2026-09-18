#include "RebaseDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Guit
{

RebaseDialog::RebaseDialog(const QStringList &branches, const QString &current, QWidget *parent)
    : QDialog(parent)
    , m_ontoBox(new QComboBox(this))
    , m_hintLabel(new QLabel(this))
{
    setWindowTitle(tr("Rebase"));
    for (const QString &branch : branches) {
        if (branch != current)
            m_ontoBox->addItem(branch);
    }
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *warning = new QLabel(tr("Rebase replays your commits on top of another branch and rewrites history. "
                                  "Only rebase commits you have not shared yet."),
                               this);
    warning->setWordWrap(true);
    warning->setStyleSheet(QStringLiteral("color: #B00020;"));

    auto *form = new QFormLayout();
    form->addRow(tr("Replay %1 onto:").arg(current.isEmpty() ? tr("the current branch") : current), m_ontoBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("Rebase"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(warning);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_ontoBox, &QComboBox::currentTextChanged, this, &RebaseDialog::validate);
    validate();
}

QString RebaseDialog::onto() const
{
    return m_ontoBox->currentText().trimmed();
}

void RebaseDialog::validate()
{
    const bool ok = !onto().isEmpty();
    m_okButton->setEnabled(ok);
    m_hintLabel->setText(ok ? tr("Git: git rebase %1").arg(onto()) : tr("No other branch to rebase onto."));
}

} // namespace Guit
