#pragma once

#include "../ui/Theme.h"

#include <QObject>
#include <QString>

namespace Guit
{

class AppSettings;

// Single source of truth for theme STATE (mode + persistence + change
// notification). All rendering lives in Theme (colors, palette,
// stylesheet); this class only decides which mode is active and when to
// re-apply it (including OS scheme changes in System mode).
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class Theme
    {
        System,
        Light,
        Dark
    };
    Q_ENUM(Theme)

    explicit ThemeManager(AppSettings *settings, QObject *parent = nullptr);

    [[nodiscard]] Theme current() const { return m_current; }
    [[nodiscard]] bool isDark() const;
    void setTheme(Theme theme);

    static Theme themeFromString(const QString &name);
    static QString themeToString(Theme theme);
    static ThemeMode toMode(Theme theme);

signals:
    void themeChanged(Guit::ThemeManager::Theme theme);

private slots:
    void reapply();

private:
    void apply(Theme theme);

    AppSettings *m_settings = nullptr;
    Theme m_current = Theme::System;
};

} // namespace Guit
