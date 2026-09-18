#include "PushDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace Guit
{

PushDialog::PushDialog(Mode mode, const QStringList &remotes, const QStringList &sources, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_remoteBox(new QComboBox(this))
    , m_sourceBox(new QComboBox(this))
    , m_upstreamBox(new QCheckBox(tr("Set upstream tracking"), this))
    , m_forceBox(new QCheckBox(tr("Force (only if the remote has no unseen work)"), this))
    , m_hintLabel(new QLabel(this))
{
    const bool isDelete = mode == Mode::DeleteRemoteBranch;
    setWindowTitle(isDelete ? tr("Delete Remote Branch") : tr("Push"));
    m_remoteBox->addItems(remotes);
    m_sourceBox->addItems(sources);
    m_sourceBox->setEditable(true);
    m_upstreamBox->setToolTip(tr("Link the local branch to the remote one so future pull/push need no arguments."));
    // --force-with-lease, not --force: refuses when the remote moved
    // unexpectedly. Still destructive — the page confirms before pushing.
    m_forceBox->setToolTip(tr("Uses --force-with-lease, which aborts if the remote contains work you have not seen."));
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *form = new QFormLayout();
    form->addRow(tr("Remote:"), m_remoteBox);
    form->addRow(isDelete ? tr("Branch:") : tr("Branch or tag:"), m_sourceBox);
    if (!isDelete) {
        form->addRow(QString(), m_upstreamBox);
        form->addRow(QString(), m_forceBox);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(isDelete ? tr("Delete") : tr("Push"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_remoteBox, &QComboBox::currentTextChanged, this, &PushDialog::validate);
    connect(m_sourceBox, &QComboBox::currentTextChanged, this, &PushDialog::validate);
    connect(m_upstreamBox, &QCheckBox::toggled, this, &PushDialog::validate);
    connect(m_forceBox, &QCheckBox::toggled, this, &PushDialog::validate);
    validate();
}

QString PushDialog::remote() const
{
    return m_remoteBox->currentText().trimmed();
}

QString PushDialog::source() const
{
    return m_sourceBox->currentText().trimmed();
}

bool PushDialog::setUpstream() const
{
    return m_upstreamBox->isChecked();
}

bool PushDialog::force() const
{
    return m_forceBox->isChecked();
}

void PushDialog::setCurrentRemote(const QString &remote)
{
    m_remoteBox->setCurrentText(remote);
}

void PushDialog::setCurrentSource(const QString &source)
{
    m_sourceBox->setCurrentText(source);
}

void PushDialog::validate()
{
    const bool ok = !remote().isEmpty() && !source().isEmpty();
    m_okButton->setEnabled(ok);
    if (!ok) {
        m_hintLabel->clear();
        return;
    }
    if (m_mode == Mode::DeleteRemoteBranch) {
        m_hintLabel->setText(tr("Git: git push %1 --delete %2").arg(remote(), source()));
        return;
    }
    QString command = QStringLiteral("git push %1").arg(remote());
    if (force())
        command += QStringLiteral(" --force-with-lease");
    if (setUpstream())
        command += QStringLiteral(" --set-upstream");
    m_hintLabel->setText(tr("Git: %1 %2").arg(command, source()));
}

} // namespace Guit
