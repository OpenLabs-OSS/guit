// Regression test for the gitignore crash bug.
// The bug was caused by iterating over a temporary QMap returned by gitignorePresets()
// twice (once for constBegin, once for constEnd), creating iterators from different
// temporary objects.

#include "TestTempRepo.h"

#include <git/GitRepository.h>

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace Guit;

class TestGitignoreCrash : public QObject
{
    Q_OBJECT

private slots:
    void openDialogWithExistingGitignore()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        QFile gitignore(QDir(repo.path()).filePath(QStringLiteral(".gitignore")));
        gitignore.open(QIODevice::WriteOnly | QIODevice::Text);
        gitignore.write("*.o\n*.tmp\n");
        gitignore.close();

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QString content = repository.readGitignore();
        QCOMPARE(content.trimmed(), QStringLiteral("*.o\n*.tmp"));
    }

    void openDialogWithoutGitignore()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QString content = repository.readGitignore();
        QVERIFY(content.isEmpty());
    }

    void emptyGitignore()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        QFile gitignore(QDir(repo.path()).filePath(QStringLiteral(".gitignore")));
        gitignore.open(QIODevice::WriteOnly | QIODevice::Text);
        gitignore.close();

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QString content = repository.readGitignore();
        QVERIFY(content.isEmpty());
    }

    void nonEmptyGitignore()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        QFile gitignore(QDir(repo.path()).filePath(QStringLiteral(".gitignore")));
        gitignore.open(QIODevice::WriteOnly | QIODevice::Text);
        gitignore.write("*.log\nbuild/\n");
        gitignore.close();

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QString content = repository.readGitignore();
        QCOMPARE(content.trimmed(), QStringLiteral("*.log\nbuild/"));
    }

    void writeGitignore()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.writeGitignore("*.obj\nbuild/\n").ok);
        QString content = repository.readGitignore();
        QCOMPARE(content.trimmed(), QStringLiteral("*.obj\nbuild/"));
    }

    void presetsRoundTrip()
    {
        // Test that presets can be retrieved without crashing
        QMap<QString, QStringList> presets = GitRepository::gitignorePresets();
        QVERIFY(presets.contains(QStringLiteral("Python")));
        QVERIFY(presets.contains(QStringLiteral("Windows")));
        QVERIFY(presets.contains(QStringLiteral("Node")));

        // Iterating over the map should not crash
        int count = 0;
        for (auto it = presets.constBegin(); it != presets.constEnd(); ++it) {
            QVERIFY(!it.key().isEmpty());
            QVERIFY(!it.value().isEmpty());
            ++count;
        }
        QVERIFY(count > 0);
    }
};

QTEST_MAIN(TestGitignoreCrash)
#include "tst_gitignore_crash.moc"