#include "ThemeManager.h"

#include "AppSettings.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

namespace Guit
{

ThemeManager::ThemeManager(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_current = themeFromString(m_settings != nullptr ? m_settings->theme() : QString());
    apply(m_current);
}

ThemeManager::Theme ThemeManager::themeFromString(const QString &name)
{
    const QString normalized = name.trimmed().toLower();
    if (normalized == QStringLiteral("light"))
        return Theme::Light;
    if (normalized == QStringLiteral("dark"))
        return Theme::Dark;
    return Theme::System;
}

QString ThemeManager::themeToString(Theme theme)
{
    switch (theme) {
    case Theme::Light: return QStringLiteral("light");
    case Theme::Dark:  return QStringLiteral("dark");
    case Theme::System:
    default:           return QStringLiteral("system");
    }
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

void ThemeManager::apply(Theme theme)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QStyleHints *hints = QGuiApplication::styleHints();
    switch (theme) {
    case Theme::Light:
        hints->setColorScheme(Qt::ColorScheme::Light);
        qApp->setPalette(qApp->style()->standardPalette());
        break;
    case Theme::Dark:
        hints->setColorScheme(Qt::ColorScheme::Dark);
        break;
    case Theme::System:
    default:
        hints->unsetColorScheme();
        qApp->setPalette(qApp->style()->standardPalette());
        break;
    }
#endif
    if (theme == Theme::Dark) {
        // Explicit dark palette: readable in every widget without
        // decorative styling (no gradients, no heavy custom CSS).
        QPalette dark;
        const QColor window(0x2B2B2B);
        const QColor base(0x1E1E1E);
        const QColor text(0xE8E8E8);
        const QColor disabled(0x808080);
        const QColor highlight(0x2F7AD6);
        dark.setColor(QPalette::Window, window);
        dark.setColor(QPalette::WindowText, text);
        dark.setColor(QPalette::Base, base);
        dark.setColor(QPalette::AlternateBase, window);
        dark.setColor(QPalette::Text, text);
        dark.setColor(QPalette::Button, window);
        dark.setColor(QPalette::ButtonText, text);
        dark.setColor(QPalette::BrightText, Qt::white);
        dark.setColor(QPalette::Highlight, highlight);
        dark.setColor(QPalette::HighlightedText, Qt::white);
        dark.setColor(QPalette::Link, QColor(0x6CB2FF));
        dark.setColor(QPalette::PlaceholderText, disabled);
        dark.setColor(QPalette::Disabled, QPalette::Text, disabled);
        dark.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
        dark.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
        qApp->setPalette(dark);
    }
}

} // namespace Guit
