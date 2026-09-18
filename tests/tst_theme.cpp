// Design-system checks: spec sanity (contrast between roles, dark/light
// distinction), theme application (palette + stylesheet install without
// errors), and mode string round-trips.

#include <app/ThemeManager.h>
#include <ui/Theme.h>

#include <QApplication>
#include <QFontInfo>
#include <QPalette>
#include <QtTest>

using namespace Guit;

static int luminance(const QColor &color)
{
    return static_cast<int>(0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue());
}

class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void specsAreInternallyConsistent()
    {
        for (const ThemeSpec &spec : {Theme::lightSpec(), Theme::darkSpec()}) {
            for (const QColor &color :
                 {spec.window, spec.surface, spec.border, spec.primaryText, spec.accent, spec.danger,
                  spec.selection, spec.diffAddText, spec.diffDelText}) {
                QVERIFY2(color.isValid(), "Every semantic color must be valid");
            }
            // Text must stand apart from its background in both themes.
            QVERIFY(qAbs(luminance(spec.primaryText) - luminance(spec.window)) > 80);
            QVERIFY(qAbs(luminance(spec.secondaryText) - luminance(spec.window)) > 50);
            // Surfaces must differ from each other (layered darks, clean lights).
            QVERIFY(spec.window != spec.surface);
            QVERIFY(spec.border != spec.window);
        }
        QVERIFY(Theme::darkSpec().isDark);
        QVERIFY(!Theme::lightSpec().isDark);
        // Accent and danger must be distinguishable everywhere.
        QVERIFY(Theme::lightSpec().accent != Theme::lightSpec().danger);
        QVERIFY(Theme::darkSpec().accent != Theme::darkSpec().danger);
    }

    void modeStringsRoundTrip()
    {
        QCOMPARE(Theme::modeToString(ThemeMode::System), QStringLiteral("system"));
        QCOMPARE(Theme::modeToString(ThemeMode::Light), QStringLiteral("light"));
        QCOMPARE(Theme::modeToString(ThemeMode::Dark), QStringLiteral("dark"));
        QCOMPARE(Theme::modeFromString(QStringLiteral("dark")), ThemeMode::Dark);
        QCOMPARE(Theme::modeFromString(QStringLiteral("LIGHT")), ThemeMode::Light);
        QCOMPARE(Theme::modeFromString(QStringLiteral("nonsense")), ThemeMode::System);
        QCOMPARE(ThemeManager::themeToString(ThemeManager::Theme::Dark), QStringLiteral("dark"));
        QCOMPARE(ThemeManager::themeFromString(QStringLiteral("light")), ThemeManager::Theme::Light);
    }

    void applyInstallsPaletteAndStylesheet()
    {
        Theme::apply(ThemeMode::Light);
        QVERIFY(!qApp->styleSheet().isEmpty());
        QCOMPARE(qApp->palette().color(QPalette::Window), Theme::lightSpec().window);
        QVERIFY(qApp->styleSheet().contains(Theme::lightSpec().accent.name(QColor::HexRgb)));

        Theme::apply(ThemeMode::Dark);
        QCOMPARE(qApp->palette().color(QPalette::Window), Theme::darkSpec().window);
        QVERIFY(qApp->styleSheet().contains(Theme::darkSpec().accent.name(QColor::HexRgb)));
        // Disabled roles must be populated (no light-default fallbacks).
        QVERIFY(qApp->palette().color(QPalette::Disabled, QPalette::WindowText).isValid());
        QVERIFY(qApp->palette().color(QPalette::Disabled, QPalette::ButtonText).isValid());
        // Tooltip roles must be dark, never default white.
        QCOMPARE(qApp->palette().color(QPalette::ToolTipBase), Theme::darkSpec().elevated);

        Theme::apply(ThemeMode::Light); // leave a sane default
    }

    void monoFontIsFixedPitch()
    {
        // QFontInfo resolves real font matching; QFont::fixedPitch only
        // reflects an explicitly set flag.
        QVERIFY(QFontInfo(Theme::monoFont()).fixedPitch());
    }
};

QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
