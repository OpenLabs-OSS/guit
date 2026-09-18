// Stash workflows: list parsing, save (tracked + untracked), inspect,
// apply/pop/drop/clear.

#include "TestTempRepo.h"

#include <git/AdvancedModels.h>
#include <git/GitRepository.h>

#include <QDir>
#include <QFile>
#include <QtTest>

using namespace Guit;

class TestStash : public QObject
{
    Q_OBJECT

private slots:
    void parsesList()
    {
        const QString output = QStringLiteral("stash@{0}: WIP on main: 34307fa Subject line\n"
                                              "stash@{1}: On feature: custom message\n");
        const QList<StashInfo> entries = StashInfo::parseList(output);
        QCOMPARE(entries.size(), 2);
        QCOMPARE(entries.at(0).ref, QStringLiteral("stash@{0}"));
        QCOMPARE(entries.at(0).index, 0);
        QCOMPARE(entries.at(0).message, QStringLiteral("WIP on main: 34307fa Subject line"));
        QCOMPARE(entries.at(1).ref, QStringLiteral("stash@{1}"));
        QCOMPARE(entries.at(1).index, 1);

        QVERIFY(StashInfo::parseList({}).isEmpty());
        QVERIFY(StashInfo::parseList(QStringLiteral("not a stash line\n")).isEmpty());
    }

    void savesAndRestoresTrackedChanges()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\nchanged\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.stashList().isEmpty());

        QVERIFY(repository.stashPush(QStringLiteral("my work"), false).ok);
        QVERIFY(repository.status().isClean());

        QList<StashInfo> entries = repository.stashList();
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0).ref, QStringLiteral("stash@{0}"));
        QVERIFY(entries.at(0).message.contains(QStringLiteral("my work")));

        // Inspect shows the shelved diff.
        const QList<FileDiff> diffs = repository.stashShow(QStringLiteral("stash@{0}"));
        QCOMPARE(diffs.size(), 1);
        QCOMPARE(diffs.at(0).newPath, QStringLiteral("a.txt"));

        // Apply keeps the entry; pop removes it.
        QVERIFY(repository.stashApply(QStringLiteral("stash@{0}")).ok);
        QCOMPARE(repository.stashList().size(), 1);
        QVERIFY(!repository.status().isClean());
        // Re-shelve via pop path: stash again, then pop.
        QVERIFY(repository.stashPush({}, false).ok);
        QCOMPARE(repository.stashList().size(), 2);
        QVERIFY(repository.stashPop(QStringLiteral("stash@{1}")).ok);
        QCOMPARE(repository.stashList().size(), 1);
        QVERIFY(!repository.status().isClean());
    }

    void includesUntrackedFiles()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("new.txt"), QStringLiteral("untracked\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        // Without the flag the untracked file survives (nothing is shelved).
        QVERIFY(repository.stashPush({}, false).ok);
        QVERIFY(QFile::exists(QDir(repo.path()).filePath(QStringLiteral("new.txt"))));
        QVERIFY(repository.stashList().isEmpty());

        // With the flag it is shelved and restored.
        QVERIFY(repository.stashPush({}, true).ok);
        QVERIFY(!QFile::exists(QDir(repo.path()).filePath(QStringLiteral("new.txt"))));
        QVERIFY(repository.stashPop(QStringLiteral("stash@{0}")).ok);
        QVERIFY(QFile::exists(QDir(repo.path()).filePath(QStringLiteral("new.txt"))));
        QVERIFY(repository.stashList().isEmpty());
    }

    void dropsAndClears()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("one\n"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.stashPush(QStringLiteral("first"), false).ok);
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("two\n"));
        QVERIFY(repository.stashPush(QStringLiteral("second"), false).ok);
        QCOMPARE(repository.stashList().size(), 2);

        QVERIFY(repository.stashDrop(QStringLiteral("stash@{1}")).ok);
        QCOMPARE(repository.stashList().size(), 1);
        QVERIFY(repository.stashClear().ok);
        QVERIFY(repository.stashList().isEmpty());
    }

    void failsGracefullyWithoutStash()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.stashApply(QStringLiteral("stash@{0}")).ok);
        QVERIFY(!repository.stashPop(QStringLiteral("stash@{0}")).ok);
        QVERIFY(!repository.stashDrop(QStringLiteral("stash@{0}")).ok);
        QVERIFY(repository.stashShow(QStringLiteral("stash@{0}")).isEmpty());
        // Clearing an empty stash list is harmless.
        QVERIFY(repository.stashClear().ok);
    }
};

QTEST_MAIN(TestStash)
#include "tst_stash.moc"
