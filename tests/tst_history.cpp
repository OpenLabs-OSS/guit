// History workflows through GitRepository: log ordering/limits, commit
// details (message, files incl. renames, diff), and failure paths.

#include "TestTempRepo.h"

#include <git/GitRepository.h>

#include <QtTest>

using namespace Guit;

class TestHistory : public QObject
{
    Q_OBJECT

private slots:
    void logListsNewestFirst()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Second"));
        repo.writeFile(QStringLiteral("c.txt"), QStringLiteral("c\n"));
        repo.commit(QStringLiteral("Third"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QList<CommitInfo> history = repository.log();
        QCOMPARE(history.size(), 3);
        QCOMPARE(history.at(0).subject, QStringLiteral("Third"));
        QCOMPARE(history.at(1).subject, QStringLiteral("Second"));
        QCOMPARE(history.at(2).subject, QStringLiteral("First"));
        QVERIFY(!history.at(0).hash.isEmpty());
        QCOMPARE(history.at(0).parents.size(), 1);
        QCOMPARE(history.at(0).authorName, QStringLiteral("Guit Test"));
    }

    void logRespectsLimit()
    {
        GuitTest::TempRepo repo;
        for (int i = 0; i < 5; ++i) {
            repo.writeFile(QStringLiteral("f%1.txt").arg(i), QStringLiteral("x\n"));
            repo.commit(QStringLiteral("Commit %1").arg(i));
        }
        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QCOMPARE(repository.log(2).size(), 2);
        QCOMPARE(repository.log(50).size(), 5);
        QVERIFY(repository.log(0).isEmpty());
    }

    void showCommitDetails()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\nchanged\n"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Second"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const CommitInfo second = repository.log().constFirst();

        const CommitDetails details = repository.showCommit(second.hash);
        QVERIFY(details.valid);
        QCOMPARE(details.info.subject, QStringLiteral("Second"));
        QCOMPARE(details.files.size(), 2);

        bool sawModified = false;
        bool sawAdded = false;
        for (const ChangedFile &file : details.files) {
            if (file.path == QStringLiteral("a.txt") && file.status == QStringLiteral("M"))
                sawModified = true;
            if (file.path == QStringLiteral("b.txt") && file.status == QStringLiteral("A"))
                sawAdded = true;
        }
        QVERIFY(sawModified);
        QVERIFY(sawAdded);
        QVERIFY(!details.diffs.isEmpty());
    }

    void showRootCommitIncludesFiles()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Only commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const CommitDetails details = repository.showCommit(repository.log().constFirst().hash);
        QVERIFY(details.valid);
        QCOMPARE(details.files.size(), 1);
        QCOMPARE(details.files.constFirst().status, QStringLiteral("A"));
        QVERIFY(details.info.parents.isEmpty());
    }

    void showReportsRename()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("old.txt"), QStringLiteral("content\n"));
        repo.commit(QStringLiteral("First"));
        repo.runOrFail({QStringLiteral("mv"), QStringLiteral("old.txt"), QStringLiteral("new.txt")});
        repo.commit(QStringLiteral("Rename"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const CommitDetails details = repository.showCommit(repository.log().constFirst().hash);
        QVERIFY(details.valid);
        QCOMPARE(details.files.size(), 1);
        QVERIFY(details.files.constFirst().status.startsWith(QLatin1Char('R')));
        QCOMPARE(details.files.constFirst().path, QStringLiteral("new.txt"));
        QCOMPARE(details.files.constFirst().oldPath, QStringLiteral("old.txt"));
    }

    void showRejectsUnknownHash()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const CommitDetails missing = repository.showCommit(QStringLiteral("deadbee"));
        QVERIFY(!missing.valid);
        QVERIFY(!missing.errorMessage.isEmpty());
        QVERIFY(!repository.showCommit({}).valid);
        QVERIFY(repository.showCommit(QStringLiteral("deadbee")).diffs.isEmpty());
    }

    void historyIsEmptyBeforeFirstCommit()
    {
        GuitTest::TempRepo repo; // no commits
        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.log().isEmpty());
    }

    void historyExcludesStashCommits()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\nchanged\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.stashPush(QStringLiteral("shelved"), false).ok);

        // The stash exists, but normal history shows only real commits.
        QCOMPARE(repository.stashList().size(), 1);
        const QList<CommitInfo> history = repository.logAll();
        QCOMPARE(history.size(), 1);
        QCOMPARE(history.constFirst().subject, QStringLiteral("First"));
    }
};

QTEST_MAIN(TestHistory)
#include "tst_history.moc"
