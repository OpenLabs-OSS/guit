#pragma once

#include "../app/AppSettings.h"
#include "../app/ThemeManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Guit
{

// Application settings: theme, Git executable override, beginner/
// advanced mode, and notifications. Applied live on accept.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AppSettings *settings, ThemeManager *themes, QWidget *parent = nullptr);

private slots:
    void browseGit();
    void clearGit();
    void validateGit();
    void apply();

private:
    AppSettings *m_settings = nullptr;
    ThemeManager *m_themes = nullptr;
    QComboBox *m_themeBox = nullptr;
    QLineEdit *m_gitEdit = nullptr;
    QLabel *m_gitStatus = nullptr;
    QCheckBox *m_advancedBox = nullptr;
    QCheckBox *m_notificationsBox = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit
