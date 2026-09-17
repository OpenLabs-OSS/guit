// GitVersion parsing: valid outputs, platform suffixes, missing patch
// versions, and malformed input.

#include <git/GitVersion.h>

#include <QtTest>

using namespace Guit;

class TestGitVersion : public QObject
{
    Q_OBJECT

private slots:
    void parsesPlainVersion()
    {
        const GitVersion version = GitVersion::parse(QStringLiteral("git version 2.43.0"));
        QVERIFY(version.isValid());
        QCOMPARE(version.major, 2);
        QCOMPARE(version.minor, 43);
        QCOMPARE(version.patch, 0);
        QCOMPARE(version.toString(), QStringLiteral("2.43.0"));
    }

    void parsesWindowsSuffixedVersion()
    {
        const GitVersion version = GitVersion::parse(QStringLiteral("git version 2.55.0.windows.5"));
        QVERIFY(version.isValid());
        QCOMPARE(version.major, 2);
        QCOMPARE(version.minor, 55);
        QCOMPARE(version.patch, 0);
    }

    void parsesVersionWithoutPatch()
    {
        const GitVersion version = GitVersion::parse(QStringLiteral("git version 2.39"));
        QVERIFY(version.isValid());
        QCOMPARE(version.major, 2);
        QCOMPARE(version.minor, 39);
        QCOMPARE(version.patch, -1);
        QCOMPARE(version.toString(), QStringLiteral("2.39"));
    }

    void rejectsMalformedOutput_data()
    {
        QTest::addColumn<QString>("output");
        QTest::newRow("empty") << QString();
        QTest::newRow("garbage") << QStringLiteral("not git at all");
        QTest::newRow("missing numbers") << QStringLiteral("git version x.y.z");
        QTest::newRow("partial") << QStringLiteral("git version 2");
    }

    void rejectsMalformedOutput()
    {
        QFETCH(QString, output);
        const GitVersion version = GitVersion::parse(output);
        QVERIFY(!version.isValid());
    }
};

QTEST_MAIN(TestGitVersion)
#include "tst_gitversion.moc"
