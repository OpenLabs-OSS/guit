#include "Theme.h"
#include "ResetDialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Guit
{

ResetDialog::ResetDialog(const QString &target, QWidget *parent)
    : QDialog(parent)
    , m_softButton(new QRadioButton(resetModeLabel(ResetMode::Soft), this))
    , m_mixedButton(new QRadioButton(resetModeLabel(ResetMode::Mixed), this))
    , m_hardButton(new QRadioButton(resetModeLabel(ResetMode::Hard), this))
{
    setWindowTitle(tr("Reset Branch"));
    m_mixedButton->setChecked(true);
    m_softButton->setToolTip(resetModeExplanation(ResetMode::Soft));
    m_mixedButton->setToolTip(resetModeExplanation(ResetMode::Mixed));
    m_hardButton->setToolTip(resetModeExplanation(ResetMode::Hard));

    auto *targetLabel = new QLabel(tr("Reset the current branch to %1?").arg(target), this);
    targetLabel->setWordWrap(true);
    auto *softInfo = new QLabel(resetModeExplanation(ResetMode::Soft), this);
    auto *mixedInfo = new QLabel(resetModeExplanation(ResetMode::Mixed), this);
    auto *hardInfo = new QLabel(resetModeExplanation(ResetMode::Hard), this);
    hardInfo->setWordWrap(true);
    Theme::applyDanger(hardInfo);
    for (QLabel *label : {softInfo, mixedInfo, hardInfo})
        label->setWordWrap(true);

    auto *command = new QLabel(tr("Git: git reset --soft/--mixed/--hard %1").arg(target), this);
    command->setTextInteractionFlags(Qt::TextSelectableByMouse);
    Theme::applyMono(command);
    Theme::applyMuted(command);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Reset"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(targetLabel);
    layout->addWidget(m_softButton);
    layout->addWidget(softInfo);
    layout->addWidget(m_mixedButton);
    layout->addWidget(mixedInfo);
    layout->addWidget(m_hardButton);
    layout->addWidget(hardInfo);
    layout->addWidget(command);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ResetMode ResetDialog::mode() const
{
    if (m_softButton->isChecked())
        return ResetMode::Soft;
    if (m_hardButton->isChecked())
        return ResetMode::Hard;
    return ResetMode::Mixed;
}

} // namespace Guit
