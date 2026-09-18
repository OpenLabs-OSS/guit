#include "Theme.h"
#include "SettingsDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Guit
{

SettingsDialog::SettingsDialog(AppSettings *settings, ThemeManager *themes, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_themes(themes)
    , m_themeBox(new QComboBox(this))
    , m_gitEdit(new QLineEdit(this))
    , m_gitStatus(new QLabel(this))
    , m_advancedBox(new QCheckBox(tr("Advanced mode (reflog, LFS, submodules, worktrees, extended facts)"), this))
    , m_notificationsBox(new QCheckBox(tr("Show completion notifications"), this))
{
    setWindowTitle(tr("Settings"));
    m_themeBox->addItem(tr("System"), QStringLiteral("system"));
    m_themeBox->addItem(tr("Light"), QStringLiteral("light"));
    m_themeBox->addItem(tr("Dark"), QStringLiteral("dark"));
    m_themeBox->setCurrentIndex(qMax(0, m_themeBox->findData(m_settings->theme())));
    m_gitEdit->setText(m_settings->gitExecutableOverride());
    m_gitEdit->setPlaceholderText(tr("Auto-detected from PATH"));
    m_gitEdit->setToolTip(tr("Leave empty to use the Git found on PATH. Only set this if Guit cannot find your Git."));
    m_advancedBox->setChecked(m_settings->advancedMode());
    m_advancedBox->setToolTip(tr("Beginner mode (default) emphasizes explanations; advanced mode reveals lower-level details."));
    m_notificationsBox->setChecked(m_settings->notificationsEnabled());
    m_gitStatus->setWordWrap(true);

    auto *browseButton = new QPushButton(tr("Browse…"), this);
    auto *clearButton = new QPushButton(tr("Clear"), this);
    auto *gitRow = new QHBoxLayout();
    gitRow->addWidget(m_gitEdit, 1);
    gitRow->addWidget(browseButton);
    gitRow->addWidget(clearButton);

    auto *form = new QFormLayout();
    form->addRow(tr("Theme:"), m_themeBox);
    form->addRow(tr("Git executable:"), gitRow);
    form->addRow(QString(), m_gitStatus);
    form->addRow(tr("Mode:"), m_advancedBox);
    form->addRow(tr("Notifications:"), m_notificationsBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::browseGit);
    connect(clearButton, &QPushButton::clicked, this, &SettingsDialog::clearGit);
    connect(m_gitEdit, &QLineEdit::textChanged, this, &SettingsDialog::validateGit);
    validateGit();
}

void SettingsDialog::browseGit()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Choose Git Executable"));
    if (!path.isEmpty()) {
        m_gitEdit->setText(path);
        validateGit();
    }
}

void SettingsDialog::clearGit()
{
    m_gitEdit->clear();
    validateGit();
}

void SettingsDialog::validateGit()
{
    const QString path = m_gitEdit->text().trimmed();
    const bool ok = path.isEmpty() || QFileInfo(path).isExecutable();
    m_gitStatus->setProperty("danger", !ok);
    Theme::repolish(m_gitStatus);
    if (path.isEmpty()) {
        m_gitStatus->setText(tr("Using the Git found on PATH."));
    } else if (ok) {
        m_gitStatus->setText(tr("Custom Git executable selected. Takes effect immediately."));
    } else {
        m_gitStatus->setText(tr("Not an executable file."));
    }
    m_okButton->setEnabled(ok);
}

void SettingsDialog::apply()
{
    m_settings->setTheme(m_themeBox->currentData().toString());
    m_themes->setTheme(ThemeManager::themeFromString(m_themeBox->currentData().toString()));
    m_settings->setGitExecutableOverride(m_gitEdit->text().trimmed());
    m_settings->setAdvancedMode(m_advancedBox->isChecked());
    m_settings->setNotificationsEnabled(m_notificationsBox->isChecked());
    accept();
}

} // namespace Guit
