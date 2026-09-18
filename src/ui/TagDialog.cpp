#include "TagDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Guit
{

TagDialog::TagDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(new QLineEdit(this))
    , m_annotatedBox(new QCheckBox(tr("Annotated tag (message, tagger, date)"), this))
    , m_messageEdit(new QTextEdit(this))
    , m_targetEdit(new QLineEdit(this))
    , m_hintLabel(new QLabel(this))
{
    setWindowTitle(tr("Create Tag"));
    m_annotatedBox->setChecked(true);
    m_annotatedBox->setToolTip(tr("Annotated tags are recommended for releases; lightweight tags are just pointers."));
    m_messageEdit->setPlaceholderText(tr("Release notes, e.g. what changed in v1.0"));
    m_messageEdit->setMaximumHeight(80);
    m_targetEdit->setPlaceholderText(tr("HEAD (default)"));
    m_targetEdit->setToolTip(tr("Optional: commit to tag instead of the current HEAD."));
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *form = new QFormLayout();
    form->addRow(tr("Tag name:"), m_nameEdit);
    form->addRow(QString(), m_annotatedBox);
    form->addRow(tr("Message:"), m_messageEdit);
    form->addRow(tr("Target:"), m_targetEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setProperty("primary", true);
    m_okButton->setText(tr("Create"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &TagDialog::validate);
    connect(m_annotatedBox, &QCheckBox::toggled, this, &TagDialog::validate);
    connect(m_messageEdit, &QTextEdit::textChanged, this, &TagDialog::validate);
    connect(m_targetEdit, &QLineEdit::textChanged, this, &TagDialog::validate);
    validate();
}

QString TagDialog::tagName() const
{
    return m_nameEdit->text().trimmed();
}

QString TagDialog::message() const
{
    return m_annotatedBox->isChecked() ? m_messageEdit->toPlainText().trimmed() : QString();
}

QString TagDialog::target() const
{
    return m_targetEdit->text().trimmed();
}

void TagDialog::validate()
{
    m_messageEdit->setEnabled(m_annotatedBox->isChecked());
    const bool nameOk = !tagName().isEmpty() && !tagName().contains(QLatin1Char(' '));
    const bool messageOk = !m_annotatedBox->isChecked() || !message().isEmpty();
    const bool ok = nameOk && messageOk;
    m_okButton->setEnabled(ok);
    if (!ok) {
        m_hintLabel->setText(tr("Enter a tag name (no spaces); annotated tags need a message."));
        return;
    }
    if (m_annotatedBox->isChecked())
        m_hintLabel->setText(tr("Git: git tag --annotate --message \"…\" %1").arg(tagName()));
    else
        m_hintLabel->setText(tr("Git: git tag %1").arg(tagName()));
}

} // namespace Guit
