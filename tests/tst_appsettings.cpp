// AppSettings persistence in an isolated QSettings location: theme,
// mode, notifications, geometry, and the recent-repositories list.

#include <app/AppSettings.h>

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using namespace Guit;

class TestAppSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Never touch the user's real configuration.
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
        QCoreApplication::setOrganizationName(QStringLiteral("GuitTestOrg"));
        QCoreApplication::setApplicationName(QStringLiteral("GuitTestApp"));
    }

    void defaultsWithoutPriorWrites()
    {
        AppSettings settings;
        QCOMPARE(settings.theme(), QStringLiteral("system"));
        QVERIFY(!settings.advancedMode());
        QVERIFY(settings.notificationsEnabled());
        QVERIFY(settings.gitExecutableOverride().isEmpty());
        QVERIFY(settings.recentRepositories().isEmpty());
        QVERIFY(settings.windowGeometry().isEmpty());
    }

    void persistsThemeAndMode()
    {
        {
            AppSettings settings;
            settings.setTheme(QStringLiteral("dark"));
            settings.setAdvancedMode(true);
            settings.setNotificationsEnabled(false);
        }
        AppSettings reloaded;
        QCOMPARE(reloaded.theme(), QStringLiteral("dark"));
        QVERIFY(reloaded.advancedMode());
        QVERIFY(!reloaded.notificationsEnabled());
        // Restore defaults for other tests.
        reloaded.setTheme(QStringLiteral("system"));
        reloaded.setAdvancedMode(false);
        reloaded.setNotificationsEnabled(true);
    }

    void managesRecentRepositories()
    {
        AppSettings settings;
        settings.clearRecentRepositories();
        settings.addRecentRepository(QStringLiteral("/tmp/b"));
        settings.addRecentRepository(QStringLiteral("/tmp/a"));
        settings.addRecentRepository(QStringLiteral("/tmp/b")); // most-recent-first, deduplicated
        QCOMPARE(settings.recentRepositories(), QStringList({QStringLiteral("/tmp/b"), QStringLiteral("/tmp/a")}));
        settings.removeRecentRepository(QStringLiteral("/tmp/b"));
        QCOMPARE(settings.recentRepositories(), QStringList({QStringLiteral("/tmp/a")}));
        for (int i = 0; i < 15; ++i)
            settings.addRecentRepository(QStringLiteral("/tmp/repo%1").arg(i));
        QVERIFY(settings.recentRepositories().size() <= AppSettings::maxRecentRepositories);
        QCOMPARE(settings.recentRepositories().constFirst(), QStringLiteral("/tmp/repo14"));
        settings.clearRecentRepositories();
        QVERIFY(settings.recentRepositories().isEmpty());
    }

    void persistsGitOverrideAndGeometry()
    {
        AppSettings settings;
        settings.setGitExecutableOverride(QStringLiteral("/usr/bin/git"));
        settings.setWindowGeometry(QByteArray("geometry-bytes"));
        AppSettings reloaded;
        QCOMPARE(reloaded.gitExecutableOverride(), QStringLiteral("/usr/bin/git"));
        QCOMPARE(reloaded.windowGeometry(), QByteArray("geometry-bytes"));
        reloaded.setGitExecutableOverride({});
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TestAppSettings)
#include "tst_appsettings.moc"
