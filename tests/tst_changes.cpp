// Working-tree workflows through GitRepository: status, stage/unstage,
// discard, commit, and amend — plus validation and failure paths.

#include "TestTempRepo.h"

#include <git/GitRepository.h>

#include <QDir>
#include <QFile>
#include <QtTest>

using namespace Guit;

class TestChanges : public QObject
{
    Q_OBJECT

private slots:
    void statusIsCleanAfterCommit()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const StatusSnapshot snapshot = repository.status();
        QVERIFY(snapshot.valid);
        QVERIFY(snapshot.isClean());
    }

    void stageAndUnstageRoundTrip()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\nmore\n"));
        repo.writeFile(QStringLiteral("new.txt"), QStringLiteral("new\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        StatusSnapshot snapshot = repository.status();
        QCOMPARE(snapshot.entries.size(), 2);
        QVERIFY(snapshot.staged().isEmpty());
        QCOMPARE(snapshot.unstaged().size(), 2);

        OperationResult staged = repository.stagePaths({QStringLiteral("file.txt")});
        QVERIFY2(staged.ok, qPrintable(staged.message));
        QVERIFY(staged.command.contains(QStringLiteral("git add")));
        snapshot = repository.status();
        QCOMPARE(snapshot.staged().size(), 1);
        // file.txt is fully staged (clean worktree); only new.txt is unstaged.
        QCOMPARE(snapshot.unstaged().size(), 1);

        OperationResult unstaged = repository.unstagePaths({QStringLiteral("file.txt")});
        QVERIFY2(unstaged.ok, qPrintable(unstaged.message));
        QVERIFY(unstaged.command.contains(QStringLiteral("restore")));
        QVERIFY(repository.status().staged().isEmpty());

        QVERIFY(repository.stageAll().ok);
        QCOMPARE(repository.status().staged().size(), 2);
        QVERIFY(repository.unstageAll().ok);
        QVERIFY(repository.status().staged().isEmpty());
    }

    void discardsTrackedModification()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("original\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("modified\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const StatusSnapshot before = repository.status();
        QVERIFY(!before.isClean());

        QVERIFY(repository.discardEntries(before.entries).ok);
        const StatusSnapshot after = repository.status();
        QVERIFY(after.isClean());

        QFile file(QDir(repo.path()).filePath(QStringLiteral("file.txt")));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("original\n"));
    }

    void discardsUntrackedFile()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("scratch.txt"), QStringLiteral("temp\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const StatusSnapshot before = repository.status();
        QCOMPARE(before.entries.size(), 1);

        QVERIFY(repository.discardEntries(before.entries).ok);
        QVERIFY(repository.status().isClean());
        QVERIFY(!QFile::exists(QDir(repo.path()).filePath(QStringLiteral("scratch.txt"))));
    }

    void refusesDiscardOnConflict()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("base\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("feature\n"));
        repo.commit(QStringLiteral("Feature change"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("main\n"));
        repo.commit(QStringLiteral("Main change"));
        QVERIFY(repo.runRaw({QStringLiteral("merge"), QStringLiteral("feature")}) != 0);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const StatusSnapshot snapshot = repository.status();
        QVERIFY(!snapshot.entries.isEmpty());
        QVERIFY(snapshot.entries.constFirst().isConflicted());

        const OperationResult result = repository.discardEntries(snapshot.entries);
        QVERIFY(!result.ok);
        QVERIFY(result.message.contains(QStringLiteral("conflict"), Qt::CaseInsensitive));
    }

    void validatesCommitMessage()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const CommitResult empty = repository.commit({}, {}, false);
        QVERIFY(!empty.ok);
        const CommitResult blank = repository.commit(QStringLiteral("   "), {}, false);
        QVERIFY(!blank.ok);
    }

    void commitsAndAmends()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("second.txt"), QStringLiteral("more\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.stageAll().ok);

        CommitResult committed = repository.commit(QStringLiteral("Second commit"), QStringLiteral("Details here."), false);
        QVERIFY2(committed.ok, qPrintable(committed.message));
        QVERIFY(committed.command.contains(QStringLiteral("git commit")));
        QVERIFY(!committed.commitHash.isEmpty());
        QVERIFY(repository.status().isClean());

        repo.writeFile(QStringLiteral("third.txt"), QStringLiteral("amended\n"));
        QVERIFY(repository.stageAll().ok);
        const QString beforeAmend = repository.head().commitHash;
        CommitResult amended = repository.commit(QStringLiteral("Second commit (amended)"), {}, true);
        QVERIFY2(amended.ok, qPrintable(amended.message));
        QVERIFY(amended.command.contains(QStringLiteral("--amend")));
        QVERIFY(amended.commitHash != beforeAmend);

        const QList<CommitInfo> history = repository.log();
        QCOMPARE(history.size(), 2);
        QCOMPARE(history.constFirst().subject, QStringLiteral("Second commit (amended)"));
    }

    void failsGracefullyWithoutRepository()
    {
        GitRepository repository; // never opened
        QVERIFY(!repository.status().valid);
        QVERIFY(!repository.stageAll().ok);
        QVERIFY(!repository.unstageAll().ok);
        QVERIFY(!repository.stagePaths({QStringLiteral("x")}).ok);
        QVERIFY(!repository.discardEntries({}).ok);
        QVERIFY(!repository.commit(QStringLiteral("msg"), {}, false).ok);
        QVERIFY(repository.log().isEmpty());
        QVERIFY(!repository.showCommit(QStringLiteral("abc")).valid);
        QVERIFY(repository.branches().isEmpty());
    }
};

QTEST_MAIN(TestChanges)
#include "tst_changes.moc"
