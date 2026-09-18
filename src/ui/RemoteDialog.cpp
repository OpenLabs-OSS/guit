#include "RemoteDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Guit
{

RemoteDialog::RemoteDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_nameEdit(new QLineEdit(this))
    , m_urlEdit(new QLineEdit(this))
    , m_hintLabel(new QLabel(this))
{
    setWindowTitle(mode == Mode::Add ? tr("Add Remote") : tr("Edit Remote"));
    m_urlEdit->setPlaceholderText(tr("https://example.com/user/repo.git"));
    m_urlEdit->setToolTip(tr("Guit never stores credentials — authentication is handled by Git's credential helpers."));
    m_hintLabel->setWordWrap(true);

    auto *form = new QFormLayout();
    form->addRow(tr("Name:"), m_nameEdit);
    form->addRow(tr("URL:"), m_urlEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setProperty("primary", true);
    m_okButton->setText(mode == Mode::Add ? tr("Add") : tr("Save"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &RemoteDialog::validate);
    connect(m_urlEdit, &QLineEdit::textChanged, this, &RemoteDialog::validate);
    validate();
}

QString RemoteDialog::remoteName() const
{
    return m_nameEdit->text().trimmed();
}

QString RemoteDialog::remoteUrl() const
{
    return m_urlEdit->text().trimmed();
}

void RemoteDialog::setRemoteName(const QString &name)
{
    m_nameEdit->setText(name);
}

void RemoteDialog::setRemoteUrl(const QString &url)
{
    m_urlEdit->setText(url);
}

void RemoteDialog::setNameEditable(bool editable)
{
    m_nameEdit->setReadOnly(!editable);
}

void RemoteDialog::validate()
{
    const bool nameOk = !remoteName().isEmpty() && !remoteName().contains(QLatin1Char(' '));
    const bool urlOk = !remoteUrl().isEmpty();
    // In edit mode only the URL changes, so only it gates OK.
    const bool ok = (m_mode == Mode::Edit) ? urlOk : (nameOk && urlOk);
    m_okButton->setEnabled(ok);
    if (!ok) {
        m_hintLabel->setText(tr("Enter a name without spaces and the remote URL."));
        return;
    }
    if (m_mode == Mode::Add)
        m_hintLabel->setText(tr("Git: git remote add %1 %2").arg(remoteName(), remoteUrl()));
    else
        m_hintLabel->setText(tr("Git: git remote set-url %1 %2").arg(remoteName(), remoteUrl()));
}

} // namespace Guit
