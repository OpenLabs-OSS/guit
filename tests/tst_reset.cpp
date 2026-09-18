// Reset, revert, and cherry-pick workflows through GitRepository.

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

// Two commits (A, B) plus a staged change and an unstaged change.
static void makeTwoCommitRepo(GuitTest::TempRepo &repo)
{
    repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("A\n"));
    repo.commit(QStringLiteral("Commit A"));
    repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("A\nB\n"));
    repo.commit(QStringLiteral("Commit B"));
    repo.writeFile(QStringLiteral("staged.txt"), QStringLiteral("staged\n"));
    repo.runOrFail({QStringLiteral("add"), QStringLiteral("staged.txt")});
    repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("A\nB\nunstaged\n"));
}

class TestReset : public QObject
{
    Q_OBJECT

private slots:
    void softResetKeepsIndexAndFiles()
    {
        GuitTest::TempRepo repo;
        makeTwoCommitRepo(repo);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QString commitA = repository.log().last().hash;

        OperationResult result = repository.resetTo(commitA, ResetMode::Soft);
        QVERIFY2(result.ok, qPrintable(result.message));
        QVERIFY(result.command.contains(QStringLiteral("--soft")));
        QCOMPARE(repository.head().commitHash.left(7), commitA.left(7));
        // Index untouched: staged.txt still staged, a.txt still staged-modified.
        QVERIFY(!repository.status().staged().isEmpty());
        QCOMPARE(fileContent(repo.path(), QStringLiteral("a.txt")), QStringLiteral("A\nB\nunstaged\n"));
    }

    void mixedResetClearsIndexKeepsFiles()
    {
        GuitTest::TempRepo repo;
        makeTwoCommitRepo(repo);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QString commitA = repository.log().last().hash;

        OperationResult result = repository.resetTo(commitA, ResetMode::Mixed);
        QVERIFY2(result.ok, qPrintable(result.message));
        QVERIFY(result.command.contains(QStringLiteral("--mixed")));
        QCOMPARE(repository.head().commitHash.left(7), commitA.left(7));
        // Index cleared, files untouched: everything shows as unstaged.
        QVERIFY(repository.status().staged().isEmpty());
        QVERIFY(!repository.status().unstaged().isEmpty());
        QCOMPARE(fileContent(repo.path(), QStringLiteral("a.txt")), QStringLiteral("A\nB\nunstaged\n"));
    }

    void hardResetDiscardsEverything()
    {
        GuitTest::TempRepo repo;
        makeTwoCommitRepo(repo);

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QString commitA = repository.log().last().hash;

        OperationResult result = repository.resetTo(commitA, ResetMode::Hard);
        QVERIFY2(result.ok, qPrintable(result.message));
        QVERIFY(result.command.contains(QStringLiteral("--hard")));
        QCOMPARE(repository.head().commitHash.left(7), commitA.left(7));
        QVERIFY(repository.status().isClean());
        QCOMPARE(fileContent(repo.path(), QStringLiteral("a.txt")), QStringLiteral("A\n"));
    }

    void rejectsBadTarget()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(!repository.resetTo(QStringLiteral("no-such-ref"), ResetMode::Mixed).ok);
        QVERIFY(!repository.resetTo({}, ResetMode::Mixed).ok);
    }

    void revertsCommit()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\nchanged\n"));
        repo.commit(QStringLiteral("Change"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QString changeHash = repository.log().constFirst().hash;

        OperationResult result = repository.revertCommit(changeHash);
        QVERIFY2(result.ok, qPrintable(result.message));
        QVERIFY(result.command.contains(QStringLiteral("git revert")));
        QCOMPARE(fileContent(repo.path(), QStringLiteral("a.txt")), QStringLiteral("a\n"));

        const QList<CommitInfo> history = repository.log();
        QCOMPARE(history.size(), 3); // history rewritten never; a commit was added
        QVERIFY(history.constFirst().subject.contains(QStringLiteral("Revert")));
        QVERIFY(!repository.operationState().isActive());
    }

    void cherryPicksCommit()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("side")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Side work"));
        const QString sideHash = GuitTest::TempRepo::runIn(
            repo.path(), {QStringLiteral("rev-parse"), QStringLiteral("side")}).second.trimmed();
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        OperationResult result = repository.cherryPick(sideHash);
        QVERIFY2(result.ok, qPrintable(result.message));
        QVERIFY(result.command.contains(QStringLiteral("git cherry-pick")));
        QCOMPARE(fileContent(repo.path(), QStringLiteral("b.txt")), QStringLiteral("b\n"));
        QCOMPARE(repository.log().constFirst().subject, QStringLiteral("Side work"));
        QVERIFY(!repository.operationState().isActive());
    }

    void abortsConflictingCherryPick()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("base\n"));
        repo.commit(QStringLiteral("Base"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("side")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("side\n"));
        repo.commit(QStringLiteral("Side change"));
        const QString sideHash = GuitTest::TempRepo::runIn(
            repo.path(), {QStringLiteral("rev-parse"), QStringLiteral("side")}).second.trimmed();
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("main\n"));
        repo.commit(QStringLiteral("Main change"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const OperationResult result = repository.cherryPick(sideHash);
        QVERIFY(!result.ok);
        QVERIFY(result.conflict);
        QVERIFY(repository.operationState().cherryPicking);

        QVERIFY(repository.cherryPickAbort().ok);
        QVERIFY(!repository.operationState().isActive());
        QCOMPARE(fileContent(repo.path(), QStringLiteral("file.txt")), QStringLiteral("main\n"));

        QVERIFY(!repository.cherryPick(QStringLiteral("deadbee")).ok);
    }
};

QTEST_MAIN(TestReset)
#include "tst_reset.moc"
