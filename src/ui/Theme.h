#pragma once

#include <QColor>
#include <QFont>
#include <QLabel>
#include <QMargins>
#include <QString>
#include <QWidget>

namespace Guit
{

enum class ThemeMode
{
    System,
    Light,
    Dark
};

// Semantic color roles. Widgets use these through Theme helpers or QSS
// dynamic properties ([danger="true"], [primary="true"], ...) — never
// literal colors, so both themes stay consistent by construction.
struct ThemeSpec
{
    QColor window;          // main background
    QColor sidebar;         // sidebar / status bar background
    QColor surface;         // inputs, lists, text areas
    QColor elevated;        // menus, tooltips, dialogs popups, toasts
    QColor border;          // 1px separators and control borders
    QColor primaryText;
    QColor secondaryText;
    QColor mutedText;
    QColor accent;
    QColor accentHover;
    QColor accentText;      // text on top of accent
    QColor danger;
    QColor warning;
    QColor success;
    QColor selection;
    QColor selectionText;
    QColor diffAddText;
    QColor diffAddBg;
    QColor diffDelText;
    QColor diffDelBg;
    QColor hunkText;
    QColor link;
    bool isDark = false;
};

// Single source of truth for the visual system: semantic colors,
// typography, spacing, palette construction, and the global stylesheet.
// ThemeManager owns the mode state and persistence; Theme owns rendering.
class Theme
{
public:
    Theme() = delete;

    static ThemeSpec lightSpec();
    static ThemeSpec darkSpec();
    static ThemeSpec resolve(ThemeMode mode);

    [[nodiscard]] static const ThemeSpec &currentSpec();

    // Applies palette + Fusion style + global stylesheet application-wide.
    static void apply(ThemeMode mode);

    // Typography (restrained levels; base sizes derive from the app font).
    static QFont monoFont();
    static void applyTitle(QLabel *label);     // page titles
    static void applySection(QLabel *label);   // section headings
    static void applySecondary(QLabel *label); // secondary text
    static void applyMuted(QLabel *label);     // metadata
    static void applyMono(QLabel *label);      // hashes, commands, paths
    static void applyDanger(QLabel *label);    // error text

    // Re-applies the stylesheet to a widget after a dynamic property
    // ([danger="true"], ...) changed at runtime.
    static void repolish(QWidget *widget);

    // Spacing system (px).
    static int pageMargin() { return 16; }
    static int sectionSpacing() { return 12; }
    static int controlSpacing() { return 8; }
    static int dialogMargin() { return 16; }
    static int buttonMinHeight() { return 30; }
    static int sidebarWidth() { return 232; }
    static int listRowPadding() { return 6; }

    static QString modeToString(ThemeMode mode);
    static ThemeMode modeFromString(const QString &name);

private:
    static ThemeSpec s_current;
};

} // namespace Guit
