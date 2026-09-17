#pragma once

#include <QObject>
#include <QString>

namespace Guit
{

class AppSettings;

// Light / dark / system theme support. Dark mode uses an explicit palette
// so it looks correct on every platform; light and system modes follow the
// platform style (via QStyleHints::colorScheme on Qt 6.5+).
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
    void setTheme(Theme theme);

    static Theme themeFromString(const QString &name);
    static QString themeToString(Theme theme);

signals:
    void themeChanged(Guit::ThemeManager::Theme theme);

private:
    void apply(Theme theme);

    AppSettings *m_settings = nullptr;
    Theme m_current = Theme::System;
};

} // namespace Guit
