// Rebase workflows: clean replays, conflict with continue/skip/abort,
// and operation-state detection.

#include "TestTempRepo.h"

#include <git/AdvancedModels.h>
#include <git/GitRepository.h>

#include <QDir>
#include <QFile>
#include <QtTest>

using namespace Guit;

static QString fileContent(const QString &repoPath, const QString &name)
{
    QFile file(QDir(repoPath).filePath(name));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

class TestRebase : public QObject
{
    Q_OBJECT

private slots:
    void rebasesCleanly()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Feature work"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        repo.writeFile(QStringLiteral("c.txt"), QStringLiteral("c\n"));
        repo.commit(QStringLiteral("Main work"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("feature")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        OperationResult rebased = repository.rebaseOnto(QStringLiteral("main"));
        QVERIFY2(rebased.ok, qPrintable(rebased.message));
        QVERIFY(rebased.command.contains(QStringLiteral("git rebase")));
        QVERIFY(!repository.operationState().isActive());

        const QList<CommitInfo> history = repository.log();
        QCOMPARE(history.size(), 3);
        QCOMPARE(history.at(0).subject, QStringLiteral("Feature work"));
        QCOMPARE(history.at(1).subject, QStringLiteral("Main work"));
    }

    void detectsConflictAndContinues()
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
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("feature")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        const OperationResult result = repository.rebaseOnto(QStringLiteral("main"));
        QVERIFY(!result.ok);
        QVERIFY(result.conflict);
        const OperationState state = repository.operationState();
        QVERIFY(state.rebasing);
        QCOMPARE(state.operation, PendingOperation::Rebasing);

        // Theirs during a rebase = the commit being replayed (feature side).
        QVERIFY(repository.resolveWithTheirs(QStringLiteral("file.txt")).ok);
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("feature\n"));

        QVERIFY(repository.rebaseContinue().ok);
        QVERIFY(!repository.operationState().isActive());
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("feature\n"));
    }

    void skipsConflictingCommit()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("base\n"));
        repo.writeFile(QStringLiteral("other.txt"), QStringLiteral("other\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("feature\n"));
        repo.commit(QStringLiteral("Conflicting change"));
        repo.writeFile(QStringLiteral("extra.txt"), QStringLiteral("extra\n"));
        repo.commit(QStringLiteral("Clean change"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("main\n"));
        repo.commit(QStringLiteral("Main change"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("feature")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.rebaseOnto(QStringLiteral("main")).ok);

        QVERIFY(repository.rebaseSkip().ok);
        QVERIFY(!repository.operationState().isActive());

        const QList<CommitInfo> history = repository.log();
        bool sawConflicting = false;
        bool sawClean = false;
        for (const CommitInfo &commit : history) {
            sawConflicting = sawConflicting || commit.subject == QStringLiteral("Conflicting change");
            sawClean = sawClean || commit.subject == QStringLiteral("Clean change");
        }
        QVERIFY(!sawConflicting);
        QVERIFY(sawClean);
    }

    void abortsRebase()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("base\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("feature\n"));
        repo.commit(QStringLiteral("Feature change"));
        const QString featureTip = GuitTest::TempRepo::runIn(
            repo.path(), {QStringLiteral("rev-parse"), QStringLiteral("feature")}).second.trimmed();
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("main\n"));
        repo.commit(QStringLiteral("Main change"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("feature")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.rebaseOnto(QStringLiteral("main")).ok);

        QVERIFY(repository.rebaseAbort().ok);
        QVERIFY(!repository.operationState().isActive());
        QCOMPARE(repository.head().commitHash, featureTip);
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("feature\n"));
    }

    void rejectsBadRebase()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.rebaseOnto(QStringLiteral("no-such-branch")).ok);
        QVERIFY(!repository.rebaseOnto(QStringLiteral("no-such-branch")).conflict);
        QVERIFY(!repository.rebaseOnto({}).ok);
        QVERIFY(!repository.rebaseContinue().ok);
    }
};

QTEST_MAIN(TestRebase)
#include "tst_rebase.moc"
