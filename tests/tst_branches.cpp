// Branch workflows through GitRepository: create/switch/rename/delete,
// validation, comparison, and failure paths.

#include "TestTempRepo.h"

#include <git/GitRepository.h>

#include <QtTest>

using namespace Guit;

class TestBranches : public QObject
{
    Q_OBJECT

private slots:
    void createsAndSwitches()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        OperationResult created = repository.createBranch(QStringLiteral("feature/login"), {}, true);
        QVERIFY2(created.ok, qPrintable(created.message));
        QVERIFY(created.command.contains(QStringLiteral("switch")));
        QCOMPARE(repository.head().branch, QStringLiteral("feature/login"));

        OperationResult back = repository.switchBranch(QStringLiteral("main"));
        QVERIFY2(back.ok, qPrintable(back.message));
        QCOMPARE(repository.head().branch, QStringLiteral("main"));

        // Created without checkout: branch exists, HEAD stays.
        OperationResult detached = repository.createBranch(QStringLiteral("side"), {}, false);
        QVERIFY2(detached.ok, qPrintable(detached.message));
        QVERIFY(detached.command.contains(QStringLiteral("git branch")));
        QCOMPARE(repository.head().branch, QStringLiteral("main"));

        const QList<BranchInfo> branches = repository.branches();
        QCOMPARE(branches.size(), 3);
        int currentCount = 0;
        for (const BranchInfo &branch : branches) {
            QVERIFY(branch.isLocal());
            if (branch.isCurrent) {
                ++currentCount;
                QCOMPARE(branch.name, QStringLiteral("main"));
            }
        }
        QCOMPARE(currentCount, 1);
    }

    void rejectsInvalidNames()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.validateBranchName({}));
        QVERIFY(!repository.validateBranchName(QStringLiteral("has space")));
        QVERIFY(!repository.validateBranchName(QStringLiteral("bad..name")));
        QVERIFY(repository.validateBranchName(QStringLiteral("feature/ok-name_1")));

        QVERIFY(!repository.createBranch(QStringLiteral("bad name"), {}, true).ok);
        QVERIFY(!repository.renameBranch(QStringLiteral("main"), QStringLiteral("bad name")).ok);
    }

    void renamesBranch()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.createBranch(QStringLiteral("old-name"), {}, false).ok);

        OperationResult renamed = repository.renameBranch(QStringLiteral("old-name"), QStringLiteral("new-name"));
        QVERIFY2(renamed.ok, qPrintable(renamed.message));
        QVERIFY(renamed.command.contains(QStringLiteral("--move")));

        const QList<BranchInfo> branches = repository.branches();
        bool sawNew = false;
        bool sawOld = false;
        for (const BranchInfo &branch : branches) {
            sawNew = sawNew || branch.name == QStringLiteral("new-name");
            sawOld = sawOld || branch.name == QStringLiteral("old-name");
        }
        QVERIFY(sawNew);
        QVERIFY(!sawOld);
    }

    void deletesBranchesSafely()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.runOrFail({QStringLiteral("branch"), QStringLiteral("merged-branch")});
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("diverged")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Diverged work"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        // Refuses to delete the checked-out branch.
        OperationResult current = repository.deleteBranch(QStringLiteral("main"), false);
        QVERIFY(!current.ok);

        // Merged branch deletes cleanly.
        QVERIFY(repository.deleteBranch(QStringLiteral("merged-branch"), false).ok);

        // Unmerged branch: safe delete fails, force succeeds.
        OperationResult safe = repository.deleteBranch(QStringLiteral("diverged"), false);
        QVERIFY(!safe.ok);
        QVERIFY(safe.message.contains(QStringLiteral("not fully merged")));
        QVERIFY(repository.deleteBranch(QStringLiteral("diverged"), true).ok);
        QVERIFY(repository.deleteBranch(QStringLiteral("diverged"), true).command.contains(QStringLiteral("-D")));
    }

    void comparesBranches()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Feature work"));
        repo.writeFile(QStringLiteral("c.txt"), QStringLiteral("c\n"));
        repo.commit(QStringLiteral("More feature work"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        const AheadBehind counts = repository.aheadBehind(QStringLiteral("main"), QStringLiteral("feature"));
        QVERIFY(counts.valid);
        QCOMPARE(counts.ahead, 2);
        QCOMPARE(counts.behind, 0);

        const AheadBehind reverse = repository.aheadBehind(QStringLiteral("feature"), QStringLiteral("main"));
        QVERIFY(reverse.valid);
        QCOMPARE(reverse.ahead, 0);
        QCOMPARE(reverse.behind, 2);

        const QList<FileDiff> diffs = repository.compareDiff(QStringLiteral("main"), QStringLiteral("feature"));
        QCOMPARE(diffs.size(), 2);

        QVERIFY(!repository.aheadBehind(QStringLiteral("main"), QStringLiteral("nope")).valid);
        QVERIFY(repository.compareDiff(QStringLiteral("main"), QStringLiteral("nope")).isEmpty());
    }

    void rejectsSwitchWithDirtyTree()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("base\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("other")});
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("other\n"));
        repo.commit(QStringLiteral("Other change"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        // Local modification that the switch would overwrite.
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("local\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const OperationResult result = repository.switchBranch(QStringLiteral("other"));
        QVERIFY(!result.ok);
        // HEAD must not have moved.
        QCOMPARE(repository.head().branch, QStringLiteral("main"));
    }
};

QTEST_MAIN(TestBranches)
#include "tst_branches.moc"
