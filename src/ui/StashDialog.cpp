#include "StashDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Guit
{

StashDialog::StashDialog(QWidget *parent)
    : QDialog(parent)
    , m_messageEdit(new QLineEdit(this))
    , m_untrackedBox(new QCheckBox(tr("Include untracked files"), this))
{
    setWindowTitle(tr("Stash Changes"));
    m_messageEdit->setPlaceholderText(tr("What are you shelving? (optional)"));
    m_untrackedBox->setToolTip(tr("Also stash new files Git does not track yet (git stash --include-untracked)."));

    auto *form = new QFormLayout();
    form->addRow(tr("Message:"), m_messageEdit);
    form->addRow(QString(), m_untrackedBox);

    auto *info = new QLabel(tr("Git: git stash push [--include-untracked] [--message \"…\"]"), this);
    info->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Stash"));
    buttons->button(QDialogButtonBox::Ok)->setProperty("primary", true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(info);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString StashDialog::message() const
{
    return m_messageEdit->text().trimmed();
}

bool StashDialog::includeUntracked() const
{
    return m_untrackedBox->isChecked();
}

} // namespace Guit
