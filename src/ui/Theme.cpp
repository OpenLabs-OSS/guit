#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QWidget>

namespace Guit
{

ThemeSpec Theme::s_current = Theme::lightSpec();

ThemeSpec Theme::lightSpec()
{
    ThemeSpec spec;
    spec.isDark = false;
    spec.window = QColor(0xF4, 0xF5, 0xF7);
    spec.sidebar = QColor(0xE8, 0xEA, 0xED);
    spec.surface = QColor(0xFF, 0xFF, 0xFF);
    spec.elevated = QColor(0xFF, 0xFF, 0xFF);
    spec.border = QColor(0xD0, 0xD4, 0xD9);
    spec.primaryText = QColor(0x1F, 0x23, 0x28);
    spec.secondaryText = QColor(0x57, 0x60, 0x6A);
    spec.mutedText = QColor(0x8C, 0x95, 0x9F);
    spec.accent = QColor(0x09, 0x69, 0xDA);
    spec.accentHover = QColor(0x07, 0x57, 0xBA);
    spec.accentText = QColor(0xFF, 0xFF, 0xFF);
    spec.danger = QColor(0xCF, 0x22, 0x2E);
    spec.warning = QColor(0x9A, 0x67, 0x00);
    spec.success = QColor(0x1A, 0x7F, 0x37);
    spec.selection = QColor(0xCD, 0xE3, 0xFA);
    spec.selectionText = QColor(0x1F, 0x23, 0x28);
    spec.diffAddText = QColor(0x1A, 0x7F, 0x37);
    spec.diffAddBg = QColor(0xE6, 0xF4, 0xEA);
    spec.diffDelText = QColor(0xCF, 0x22, 0x2E);
    spec.diffDelBg = QColor(0xFC, 0xE8, 0xEA);
    spec.hunkText = QColor(0x05, 0x50, 0xAE);
    spec.link = QColor(0x09, 0x69, 0xDA);
    return spec;
}

ThemeSpec Theme::darkSpec()
{
    // Layered dark surfaces — never flat black-on-white.
    ThemeSpec spec;
    spec.isDark = true;
    spec.window = QColor(0x1C, 0x21, 0x28);
    spec.sidebar = QColor(0x16, 0x1B, 0x22);
    spec.surface = QColor(0x22, 0x27, 0x2E);
    spec.elevated = QColor(0x2D, 0x33, 0x3B);
    spec.border = QColor(0x3A, 0x41, 0x49);
    spec.primaryText = QColor(0xE6, 0xED, 0xF3);
    spec.secondaryText = QColor(0xAD, 0xBA, 0xC7);
    spec.mutedText = QColor(0x76, 0x83, 0x90);
    spec.accent = QColor(0x2F, 0x81, 0xF7);
    spec.accentHover = QColor(0x1F, 0x6F, 0xEB);
    spec.accentText = QColor(0xFF, 0xFF, 0xFF);
    spec.danger = QColor(0xF8, 0x51, 0x49);
    spec.warning = QColor(0xD2, 0x99, 0x22);
    spec.success = QColor(0x3F, 0xB9, 0x50);
    spec.selection = QColor(0x26, 0x41, 0x5E);
    spec.selectionText = QColor(0xE6, 0xED, 0xF3);
    spec.diffAddText = QColor(0x3F, 0xB9, 0x50);
    spec.diffAddBg = QColor(0x1A, 0x2E, 0x22);
    spec.diffDelText = QColor(0xF8, 0x51, 0x49);
    spec.diffDelBg = QColor(0x3A, 0x1D, 0x20);
    spec.hunkText = QColor(0x79, 0xC0, 0xFF);
    spec.link = QColor(0x6C, 0xB2, 0xFF);
    return spec;
}

ThemeSpec Theme::resolve(ThemeMode mode)
{
    if (mode == ThemeMode::Light)
        return lightSpec();
    if (mode == ThemeMode::Dark)
        return darkSpec();
    // System: follow the OS scheme without forcing QStyleHints, so native
    // dialogs keep following the OS too.
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    return dark ? darkSpec() : lightSpec();
}

const ThemeSpec &Theme::currentSpec()
{
    return s_current;
}

QFont Theme::monoFont()
{
    // Prefer verified monospace families over a bare style hint: the
    // returned font is guaranteed fixed-pitch where fonts are installed.
    static const QStringList candidates = {QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"),
                                           QStringLiteral("Courier New"), QStringLiteral("DejaVu Sans Mono"),
                                           QStringLiteral("Monospace")};
    for (const QString &family : candidates) {
        if (QFontDatabase::isFixedPitch(family)) {
            QFont font(family);
            font.setStyleHint(QFont::TypeWriter);
            font.setFixedPitch(true);
            return font;
        }
    }
    QFont fallback = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fallback.setStyleHint(QFont::TypeWriter);
    return fallback;
}

void Theme::applyTitle(QLabel *label)
{
    QFont font = label->font();
    font.setPointSize(font.pointSize() + 3);
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
}

void Theme::applySection(QLabel *label)
{
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    label->setProperty("secondary", true);
}

void Theme::applySecondary(QLabel *label)
{
    label->setProperty("secondary", true);
}

void Theme::applyMuted(QLabel *label)
{
    label->setProperty("muted", true);
}

void Theme::applyMono(QLabel *label)
{
    label->setFont(monoFont());
}

void Theme::applyDanger(QLabel *label)
{
    label->setProperty("danger", true);
}

void Theme::repolish(QWidget *widget)
{
    if (widget == nullptr)
        return;
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QString Theme::modeToString(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Light: return QStringLiteral("light");
    case ThemeMode::Dark:  return QStringLiteral("dark");
    case ThemeMode::System:
    default:               return QStringLiteral("system");
    }
}

ThemeMode Theme::modeFromString(const QString &name)
{
    const QString normalized = name.trimmed().toLower();
    if (normalized == QStringLiteral("light"))
        return ThemeMode::Light;
    if (normalized == QStringLiteral("dark"))
        return ThemeMode::Dark;
    return ThemeMode::System;
}

namespace
{

QString colorCss(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

// A complete Fusion palette: every role in every group, so no widget
// falls back to a light default in dark mode (tooltips, disabled
// controls, separators, headers, ...).
QPalette buildPalette(const ThemeSpec &s)
{
    QPalette palette;
    const QColor disabledText = s.mutedText;
    const QColor disabledBase = s.isDark ? QColor(0x25, 0x2B, 0x33) : QColor(0xEE, 0xF0, 0xF2);
    const QColor disabledButton = s.isDark ? QColor(0x25, 0x2B, 0x33) : QColor(0xEE, 0xF0, 0xF2);
    const QColor mid = s.isDark ? QColor(0x30, 0x36, 0x3F) : QColor(0xE2, 0xE5, 0xE9);

    palette.setColor(QPalette::Window, s.window);
    palette.setColor(QPalette::WindowText, s.primaryText);
    palette.setColor(QPalette::Base, s.surface);
    palette.setColor(QPalette::AlternateBase, s.isDark ? s.elevated : QColor(0xF0, 0xF1, 0xF3));
    palette.setColor(QPalette::Text, s.primaryText);
    palette.setColor(QPalette::Button, s.isDark ? s.elevated : QColor(0xF6, 0xF7, 0xF8));
    palette.setColor(QPalette::ButtonText, s.primaryText);
    palette.setColor(QPalette::BrightText, s.isDark ? Qt::white : Qt::black);
    palette.setColor(QPalette::Highlight, s.isDark ? s.selection : s.accent);
    palette.setColor(QPalette::HighlightedText, s.selectionText);
    palette.setColor(QPalette::Link, s.link);
    palette.setColor(QPalette::LinkVisited, s.link);
    palette.setColor(QPalette::ToolTipBase, s.elevated);
    palette.setColor(QPalette::ToolTipText, s.primaryText);
    palette.setColor(QPalette::PlaceholderText, s.mutedText);
    palette.setColor(QPalette::Light, s.isDark ? QColor(0x48, 0x51, 0x5C) : Qt::white);
    palette.setColor(QPalette::Midlight, mid);
    palette.setColor(QPalette::Dark, s.border);
    palette.setColor(QPalette::Mid, mid);
    palette.setColor(QPalette::Shadow, s.isDark ? Qt::black : QColor(0x9A, 0x9E, 0xA4));

    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Base, disabledBase);
    palette.setColor(QPalette::Disabled, QPalette::Button, disabledButton);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, mid);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, disabledText);

    palette.setColor(QPalette::Inactive, QPalette::Highlight, s.isDark ? s.selection : s.accent);
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, s.selectionText);
    return palette;
}

QString buildStyleSheet(const ThemeSpec &s)
{
    const QString border = colorCss(s.border);
    const QString accent = colorCss(s.accent);
    const QString accentHover = colorCss(s.accentHover);
    const QString accentText = colorCss(s.accentText);
    const QString danger = colorCss(s.danger);
    const QString warning = colorCss(s.warning);
    const QString surface = colorCss(s.surface);
    const QString elevated = colorCss(s.elevated);
    const QString sidebar = colorCss(s.sidebar);
    const QString primary = colorCss(s.primaryText);
    const QString secondary = colorCss(s.secondaryText);
    const QString muted = colorCss(s.mutedText);
    const QString selection = colorCss(s.selection);
    const QString selectionText = colorCss(s.selectionText);
    const int rowPad = Theme::listRowPadding();
    const int btnH = Theme::buttonMinHeight();

    return QString(
        // Semantic label roles (set via dynamic properties in code).
        "QLabel[secondary=\"true\"] { color: %1; }"
        "QLabel[muted=\"true\"] { color: %2; }"
        "QLabel[danger=\"true\"] { color: %3; }"
        // Buttons: quiet secondary default, clear primary hierarchy.
        "QPushButton { min-height: %4px; padding: 4px 14px; border: 1px solid %5; border-radius: 4px; background: %6; color: %7; }"
        "QPushButton:hover { border-color: %8; }"
        "QPushButton:pressed { background: %9; }"
        "QPushButton:disabled { color: %2; border-color: %5; background: %6; }"
        "QPushButton[primary=\"true\"] { background: %8; border-color: %8; color: %10; font-weight: 600; }"
        "QPushButton[primary=\"true\"]:hover { background: %11; border-color: %11; }"
        "QPushButton[primary=\"true\"]:disabled { background: %5; border-color: %5; color: %10; }"
        "QPushButton[destructive=\"true\"] { color: %3; border-color: %3; }"
        "QPushButton[destructive=\"true\"]:hover { background: %3; color: %10; }"
        "QToolButton { border: 1px solid transparent; border-radius: 4px; padding: 4px; }"
        "QToolButton:hover { border-color: %5; background: %6; }"
        // Inputs.
        "QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox { background: %12; border: 1px solid %5; border-radius: 4px; padding: 6px 8px; color: %7; selection-background-color: %13; }"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus { border-color: %8; }"
        "QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QComboBox:disabled { color: %2; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        // Lists, trees, tables.
        "QListWidget, QListView, QTreeWidget, QTreeView, QTableView { background: %12; border: 1px solid %5; border-radius: 4px; outline: none; alternate-background-color: %6; }"
        "QListWidget::item, QListView::item, QTreeWidget::item, QTreeView::item { padding-top: %14px; padding-bottom: %14px; border: none; }"
        "QListWidget::item:selected, QListView::item:selected, QTreeWidget::item:selected, QTreeView::item:selected { background: %13; color: %15; }"
        "QListWidget::item:hover, QListView::item:hover, QTreeWidget::item:hover, QTreeView::item:hover { background: %6; }"
        "QTreeView::branch { background: transparent; }"
        "QHeaderView::section { background: %16; color: %1; border: none; border-bottom: 1px solid %5; padding: 6px 8px; }"
        // Sidebar navigation: flat list with a subtle accent indicator.
        "QListWidget#SidebarNav { background: %16; border: none; border-radius: 0px; outline: none; }"
        "QListWidget#SidebarNav::item { padding: 8px 12px; border: none; border-left: 3px solid transparent; }"
        "QListWidget#SidebarNav::item:selected { background: %13; color: %15; border-left: 3px solid %8; }"
        "QListWidget#SidebarNav::item:hover:!selected { background: %6; }"
        // Menus.
        "QMenuBar { background: %16; color: %7; }"
        "QMenuBar::item:selected { background: %13; color: %15; }"
        "QMenu { background: %17; color: %7; border: 1px solid %5; padding: 4px; }"
        "QMenu::item { padding: 6px 24px 6px 12px; }"
        "QMenu::item:selected { background: %13; color: %15; }"
        "QMenu::item:disabled { color: %2; }"
        "QMenu::separator { height: 1px; background: %5; margin: 4px 8px; }"
        // Tooltips, status bar, splitters, scrollbars.
        "QToolTip { background: %17; color: %7; border: 1px solid %5; padding: 6px; }"
        "QStatusBar { background: %16; color: %1; }"
        "QStatusBar::item { border: none; }"
        "QSplitter::handle { background: %5; }"
        "QSplitter::handle:horizontal { width: 1px; }"
        "QSplitter::handle:vertical { height: 1px; }"
        "QScrollBar:vertical { background: transparent; width: 12px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: %5; min-height: 30px; border-radius: 5px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar:horizontal { background: transparent; height: 12px; margin: 0px; }"
        "QScrollBar::handle:horizontal { background: %5; min-width: 30px; border-radius: 5px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        // Groups, tabs, progress.
        "QGroupBox { color: %7; border: 1px solid %5; border-radius: 4px; margin-top: 14px; padding-top: 8px; font-weight: 600; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QTabWidget::pane { border: 1px solid %5; border-radius: 4px; }"
        "QTabBar::tab { padding: 6px 14px; border: none; color: %1; }"
        "QTabBar::tab:selected { color: %7; border-bottom: 2px solid %8; }"
        "QProgressBar { border: 1px solid %5; border-radius: 4px; background: %12; text-align: center; color: %7; }"
        "QProgressBar::chunk { background: %8; border-radius: 2px; }"
        // Dialog buttons keep platform order; destructive role styling.
        "QDialogButtonBox QPushButton[destructive=\"true\"] { color: %3; border-color: %3; }"
        // Completion toast overlay.
        "Toast { background: %17; color: %7; border: 1px solid %5; border-radius: 6px; }"
        // Conflict lifecycle bar: elevated panel with a warning edge.
        "QWidget#ConflictBar { background: %17; border: 1px solid %18; border-radius: 4px; }"
        ).arg(secondary, muted, danger).arg(btnH).arg(border, surface, primary).arg(accent, elevated, accentText).arg(accentHover, surface, selection, QString::number(rowPad)).arg(selectionText, sidebar, elevated, warning);
}

} // namespace

void Theme::apply(ThemeMode mode)
{
    s_current = resolve(mode);
    if (QApplication::style() == nullptr || QApplication::style()->objectName() != QStringLiteral("fusion")) {
        if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
            QApplication::setStyle(fusion);
    }
    qApp->setPalette(buildPalette(s_current));
    qApp->setStyleSheet(buildStyleSheet(s_current));
}

} // namespace Guit
