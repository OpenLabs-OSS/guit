#include "GitignoreDialog.h"

#include "../controllers/RepositoryInfoController.h"
#include "../git/GitRepository.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QPushButton>
#include <QVBoxLayout>

namespace Guit
{

GitignoreDialog::GitignoreDialog(RepositoryInfoController *controller, const QString &currentContent, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_editor(new QTextEdit(this))
{
    setWindowTitle(tr("Edit .gitignore"));
    resize(560, 420);
    m_editor->setPlainText(currentContent);
    m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_editor->setToolTip(tr("Every line is one pattern. # starts a comment. Existing lines are never touched unless you edit them."));

    auto *presetBox = new QComboBox(this);
    presetBox->addItem(tr("Add common patterns…"));
    for (auto it = GitRepository::gitignorePresets().constBegin(); it != GitRepository::gitignorePresets().constEnd(); ++it)
        presetBox->addItem(it.key());
    auto *addButton = new QPushButton(tr("Add"), this);

    auto *presetRow = new QHBoxLayout();
    presetRow->addWidget(presetBox, 1);
    presetRow->addWidget(addButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(presetRow);
    layout->addWidget(m_editor, 1);
    layout->addWidget(buttons);

    connect(addButton, &QPushButton::clicked, this, [this, presetBox]() {
        if (presetBox->currentIndex() > 0)
            addPreset(presetBox->currentText());
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &GitignoreDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void GitignoreDialog::addPreset(const QString &preset)
{
    const QStringList patterns = GitRepository::gitignorePresets().value(preset);
    if (patterns.isEmpty())
        return;
    QString text = m_editor->toPlainText();
    if (!text.isEmpty() && !text.endsWith(QLatin1Char('\n')))
        text += QLatin1Char('\n');
    text += QStringLiteral("\n# %1\n").arg(preset) + patterns.join(QLatin1Char('\n')) + QLatin1Char('\n');
    m_editor->setPlainText(text);
}

void GitignoreDialog::save()
{
    m_controller->saveGitignore(m_editor->toPlainText());
    accept();
}

} // namespace Guit
