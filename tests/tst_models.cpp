// Model parsers: synthetic unit tests (including malformed input) plus
// integration tests against real temporary repositories.

#include "TestTempRepo.h"

#include <git/FileStatus.h>
#include <git/GitClient.h>
#include <git/GitModels.h>

#include <QtTest>

using namespace Guit;

class TestModels : public QObject
{
    Q_OBJECT

private slots:
    // --- FileStatus (porcelain v1) ---------------------------------------
    void parsesPorcelainStates()
    {
        const QString output = QStringLiteral("M  staged.txt\n"
                                              " M unstaged.txt\n"
                                              "MM both.txt\n"
                                              "A  added.txt\n"
                                              " D deleted.txt\n"
                                              "?? untracked.txt\n");
        const QList<FileStatusEntry> entries = parsePorcelainV1Status(output);
        QCOMPARE(entries.size(), 6);

        QCOMPARE(entries.at(0).path, QStringLiteral("staged.txt"));
        QCOMPARE(entries.at(0).stagedState, FileState::Modified);
        QCOMPARE(entries.at(0).worktreeState, FileState::Unmodified);
        QVERIFY(entries.at(0).isStaged());
        QVERIFY(!entries.at(0).isUnstaged());

        QCOMPARE(entries.at(1).stagedState, FileState::Unmodified);
        QCOMPARE(entries.at(1).worktreeState, FileState::Modified);

        QCOMPARE(entries.at(2).stagedState, FileState::Modified);
        QCOMPARE(entries.at(2).worktreeState, FileState::Modified);

        QCOMPARE(entries.at(3).stagedState, FileState::Added);
        QCOMPARE(entries.at(4).worktreeState, FileState::Deleted);
        QCOMPARE(entries.at(5).stagedState, FileState::Untracked);
    }

    void parsesPorcelainRename()
    {
        const QString output = QStringLiteral("R  new.txt -> old.txt\n").replace(QStringLiteral("new.txt -> old.txt"),
                                                                                QStringLiteral("old.txt -> new.txt"));
        const QList<FileStatusEntry> entries = parsePorcelainV1Status(output);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0).path, QStringLiteral("new.txt"));
        QCOMPARE(entries.at(0).originalPath, QStringLiteral("old.txt"));
        QCOMPARE(entries.at(0).stagedState, FileState::Renamed);
    }

    void parsesPorcelainConflicts()
    {
        const QString output = QStringLiteral("UU conflicted.txt\nAA both-added.txt\nDD both-deleted.txt\n");
        const QList<FileStatusEntry> entries = parsePorcelainV1Status(output);
        QCOMPARE(entries.size(), 3);
        for (const FileStatusEntry &entry : entries)
            QVERIFY(entry.isConflicted());
    }

    void skipsMalformedPorcelainLines()
    {
        const QString output = QStringLiteral("garbage\n"
                                              "X\n"
                                              "M \n"
                                              "M  good.txt\n");
        const QList<FileStatusEntry> entries = parsePorcelainV1Status(output);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0).path, QStringLiteral("good.txt"));
    }

    void parsesEmptyPorcelain()
    {
        QVERIFY(parsePorcelainV1Status({}).isEmpty());
    }

    // --- CommitInfo --------------------------------------------------------
    void parsesLogRecords()
    {
        const QString output =
            QStringLiteral("abc123\x1f" "\x1f" "Alice\x1f" "a@example.com\x1f"
                           "2024-01-02T03:04:05+00:00\x1f" "First\x1f" "Body line\x1e"
                           "def456\x1f" "abc123\x1f" "Bob\x1f" "b@example.com\x1f"
                           "2024-01-03T03:04:05+00:00\x1f" "Second\x1f" "\x1e");
        const QList<CommitInfo> commits = CommitInfo::parseLog(output);
        QCOMPARE(commits.size(), 2);
        QCOMPARE(commits.at(0).hash, QStringLiteral("abc123"));
        QVERIFY(commits.at(0).parents.isEmpty());
        QCOMPARE(commits.at(0).authorName, QStringLiteral("Alice"));
        QCOMPARE(commits.at(0).subject, QStringLiteral("First"));
        QCOMPARE(commits.at(0).body, QStringLiteral("Body line"));
        QVERIFY(commits.at(0).authorDate.isValid());
        QCOMPARE(commits.at(1).parents, QStringList{QStringLiteral("abc123")});
        QVERIFY(!commits.at(0).isMerge());
    }

    void skipsMalformedLogRecords()
    {
        QVERIFY(CommitInfo::parseLog({}).isEmpty());
        QVERIFY(CommitInfo::parseLog(QStringLiteral("too\x1f" "few\x1e" "fields")).isEmpty());
    }

    // --- BranchInfo ----------------------------------------------------------
    void parsesForEachRef()
    {
        const QString output =
            QStringLiteral("refs/heads/main\x1f" "main\x1f" "*\x1f" "abc123\x1f" "origin/main\n"
                           "refs/heads/feature\x1f" "feature\x1f" "\x1f" "def456\x1f" "\n"
                           "refs/remotes/origin/main\x1f" "origin/main\x1f" "\x1f" "abc123\x1f" "\n");
        const QList<BranchInfo> branches = BranchInfo::parseForEachRef(output);
        QCOMPARE(branches.size(), 3);

        QCOMPARE(branches.at(0).name, QStringLiteral("main"));
        QVERIFY(branches.at(0).isCurrent);
        QVERIFY(branches.at(0).isLocal());
        QCOMPARE(branches.at(0).upstream, QStringLiteral("origin/main"));

        QVERIFY(!branches.at(1).isCurrent);
        QVERIFY(branches.at(1).upstream.isEmpty());

        QVERIFY(branches.at(2).isRemote);
        QVERIFY(!branches.at(2).isLocal());
    }

    void skipsMalformedBranchLines()
    {
        QVERIFY(BranchInfo::parseForEachRef({}).isEmpty());
        QVERIFY(BranchInfo::parseForEachRef(QStringLiteral("incomplete\x1fline")).isEmpty());
    }

    // --- Integration against a real repository -------------------------------
    void integrationStatusLogBranch()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("tracked.txt"), QStringLiteral("one\n"));
        repo.writeFile(QStringLiteral("staged.txt"), QStringLiteral("new\n"));
        repo.commit(QStringLiteral("Initial commit"));

        // Unstaged modification + untracked file.
        repo.writeFile(QStringLiteral("tracked.txt"), QStringLiteral("one\ntwo\n"));
        repo.writeFile(QStringLiteral("untracked.txt"), QStringLiteral("new\n"));

        GitClient client;
        const GitProcessResult status = client.run(
            {QStringLiteral("status"), QStringLiteral("--porcelain=v1")}, repo.path());
        QVERIFY(status.isSuccess());
        const QList<FileStatusEntry> entries = parsePorcelainV1Status(status.standardOutput);
        QCOMPARE(entries.size(), 2);

        bool sawModified = false;
        bool sawUntracked = false;
        for (const FileStatusEntry &entry : entries) {
            if (entry.path == QStringLiteral("tracked.txt")) {
                sawModified = true;
                QCOMPARE(entry.worktreeState, FileState::Modified);
            }
            if (entry.path == QStringLiteral("untracked.txt")) {
                sawUntracked = true;
                QCOMPARE(entry.stagedState, FileState::Untracked);
            }
        }
        QVERIFY(sawModified);
        QVERIFY(sawUntracked);

        const GitProcessResult log = client.run(
            {QStringLiteral("log"), QStringLiteral("--format=") + CommitInfo::logFormat()}, repo.path());
        QVERIFY(log.isSuccess());
        const QList<CommitInfo> commits = CommitInfo::parseLog(log.standardOutput);
        QCOMPARE(commits.size(), 1);
        QCOMPARE(commits.at(0).subject, QStringLiteral("Initial commit"));
        QCOMPARE(commits.at(0).authorName, QStringLiteral("Guit Test"));
        QCOMPARE(commits.at(0).shortHash(), commits.at(0).hash.left(7));

        const GitProcessResult branches = client.run(
            {QStringLiteral("for-each-ref"),
             QStringLiteral("--format=") + BranchInfo::forEachRefFormat(),
             QStringLiteral("refs/heads"),
             QStringLiteral("refs/remotes")},
            repo.path());
        QVERIFY(branches.isSuccess());
        const QList<BranchInfo> branchList = BranchInfo::parseForEachRef(branches.standardOutput);
        QCOMPARE(branchList.size(), 1);
        QCOMPARE(branchList.at(0).name, QStringLiteral("main"));
        QVERIFY(branchList.at(0).isCurrent);
    }
};

QTEST_MAIN(TestModels)
#include "tst_models.moc"
