#include "ThemeManager.h"

#include "AppSettings.h"

#include <QApplication>
#include <QStyleHints>

namespace Guit
{

ThemeManager::ThemeManager(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_current = themeFromString(m_settings != nullptr ? m_settings->theme() : QString());
    apply(m_current);
    // Follow OS light/dark switches while in System mode.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &ThemeManager::reapply);
}

ThemeManager::Theme ThemeManager::themeFromString(const QString &name)
{
    switch (::Guit::Theme::modeFromString(name)) {
    case ThemeMode::Light: return Theme::Light;
    case ThemeMode::Dark:  return Theme::Dark;
    case ThemeMode::System:
    default:               return Theme::System;
    }
}

QString ThemeManager::themeToString(Theme theme)
{
    return ::Guit::Theme::modeToString(toMode(theme));
}

ThemeMode ThemeManager::toMode(Theme theme)
{
    switch (theme) {
    case Theme::Light: return ThemeMode::Light;
    case Theme::Dark:  return ThemeMode::Dark;
    case Theme::System:
    default:           return ThemeMode::System;
    }
}

bool ThemeManager::isDark() const
{
    return ::Guit::Theme::resolve(toMode(m_current)).isDark;
}

void ThemeManager::setTheme(Theme theme)
{
    if (theme == m_current) {
        apply(theme);
        return;
    }
    m_current = theme;
    if (m_settings != nullptr)
        m_settings->setTheme(themeToString(theme));
    apply(theme);
    emit themeChanged(theme);
}

void ThemeManager::reapply()
{
    // OS scheme changed: only System mode is affected. Re-applying also
    // re-polishes every widget through the fresh stylesheet.
    if (m_current == Theme::System)
        apply(m_current);
}

void ThemeManager::apply(Theme theme)
{
    ::Guit::Theme::apply(toMode(theme));
}

} // namespace Guit
