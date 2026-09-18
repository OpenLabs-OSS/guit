// Merge workflows: clean merges, conflict detection, ours/theirs
// resolution with continue, abort, and operation-state detection.

#include "TestTempRepo.h"

#include <git/AdvancedModels.h>
#include <git/GitRepository.h>

#include <QDir>
#include <QFile>
#include <QtTest>

using namespace Guit;

// Builds a repo where `feature` and `main` both changed file.txt
// differently, ready for a conflicting merge or rebase.
static void makeDivergedRepo(GuitTest::TempRepo &repo)
{
    repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("base\n"));
    repo.commit(QStringLiteral("Base"));
    repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
    repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("feature\n"));
    repo.commit(QStringLiteral("Feature change"));
    repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
    repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("main\n"));
    repo.commit(QStringLiteral("Main change"));
}

static QString fileContent(const QString &repoPath, const QString &name)
{
    QFile file(QDir(repoPath).filePath(name));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

class TestMerge : public QObject
{
    Q_OBJECT

private slots:
    void mergesCleanly()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Feature work"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        OperationResult merged = repository.mergeBranch(QStringLiteral("feature"), false);
        QVERIFY2(merged.ok, qPrintable(merged.message));
        QVERIFY(merged.command.contains(QStringLiteral("git merge")));
        QCOMPARE(fileContent(repo.path(), QStringLiteral("b.txt")), QStringLiteral("b\n"));
        QVERIFY(!repository.operationState().isActive());
    }

    void mergesWithNoFastForward()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Feature work"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.mergeBranch(QStringLiteral("feature"), true).ok);
        const CommitInfo tip = repository.log().constFirst();
        QVERIFY(tip.isMerge());
        QCOMPARE(tip.parents.size(), 2);
    }

    void detectsConflictAndReportsState()
    {
        GuitTest::TempRepo repo;
        makeDivergedRepo(repo);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.operationState().isActive());

        const OperationResult result = repository.mergeBranch(QStringLiteral("feature"), false);
        QVERIFY(!result.ok);
        QVERIFY(result.conflict);

        const OperationState state = repository.operationState();
        QVERIFY(state.isActive());
        QVERIFY(state.merging);
        QCOMPARE(state.operation, PendingOperation::Merging);

        const StatusSnapshot status = repository.status();
        QVERIFY(!status.entries.isEmpty());
        QVERIFY(status.entries.constFirst().isConflicted());
    }

    void resolvesWithOursAndContinues()
    {
        GuitTest::TempRepo repo;
        makeDivergedRepo(repo);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.mergeBranch(QStringLiteral("feature"), false).ok);

        QVERIFY(repository.resolveWithOurs(QStringLiteral("file.txt")).ok);
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("main\n"));
        // Ours equals HEAD here, so nothing remains staged — but crucially
        // nothing is conflicted anymore.
        const StatusSnapshot resolved = repository.status();
        for (const FileStatusEntry &entry : resolved.entries)
            QVERIFY(!entry.isConflicted());

        OperationResult continued = repository.mergeContinue();
        QVERIFY2(continued.ok, qPrintable(continued.message));
        QVERIFY(!repository.operationState().isActive());
        const CommitInfo tip = repository.log().constFirst();
        QVERIFY(tip.isMerge());
    }

    void resolvesWithTheirsAndContinues()
    {
        GuitTest::TempRepo repo;
        makeDivergedRepo(repo);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.mergeBranch(QStringLiteral("feature"), false).ok);

        QVERIFY(repository.resolveWithTheirs(QStringLiteral("file.txt")).ok);
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("feature\n"));

        QVERIFY(repository.mergeContinue().ok);
        QVERIFY(!repository.operationState().isActive());
    }

    void abortsMerge()
    {
        GuitTest::TempRepo repo;
        makeDivergedRepo(repo);
        const QString mainTip = GuitTest::TempRepo::runIn(
            repo.path(), {QStringLiteral("rev-parse"), QStringLiteral("main")}).second.trimmed();

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.mergeBranch(QStringLiteral("feature"), false).ok);
        QVERIFY(repository.operationState().merging);

        QVERIFY(repository.mergeAbort().ok);
        QVERIFY(!repository.operationState().isActive());
        QCOMPARE(repository.head().commitHash, mainTip);
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("main\n"));
    }

    void rejectsBadMerge()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.mergeBranch(QStringLiteral("no-such-branch"), false).ok);
        QVERIFY(!repository.mergeBranch(QStringLiteral("no-such-branch"), false).conflict);
        QVERIFY(!repository.mergeBranch({}, false).ok);
        // Continuing with no merge in progress fails gracefully.
        QVERIFY(!repository.mergeContinue().ok);
    }
};

QTEST_MAIN(TestMerge)
#include "tst_merge.moc"
