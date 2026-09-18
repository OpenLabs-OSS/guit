// Remote workflows: verbose-list parsing, add/remove/rename/set-url, and
// fetch/pull/push against a local bare repository (no network needed).

#include "TestTempRepo.h"

#include <git/AdvancedModels.h>
#include <git/GitRepository.h>

#include <QSignalSpy>
#include <QtTest>

using namespace Guit;

class TestRemotes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<Guit::OperationResult>();
    }

    void parsesVerboseList()
    {
        const QString output = QStringLiteral(
            "origin\thttps://example.com/a.git (fetch)\n"
            "origin\thttps://example.com/a.git (push)\n"
            "backup\tgit@example.com:b.git (fetch)\n"
            "backup\tgit@example.com:b.git (push)\n");
        const QList<RemoteInfo> remotes = RemoteInfo::parseVerboseList(output);
        QCOMPARE(remotes.size(), 2);
        QCOMPARE(remotes.at(0).name, QStringLiteral("origin"));
        QCOMPARE(remotes.at(0).fetchUrl, QStringLiteral("https://example.com/a.git"));
        QCOMPARE(remotes.at(0).pushUrl, QStringLiteral("https://example.com/a.git"));
        QCOMPARE(remotes.at(1).name, QStringLiteral("backup"));

        QVERIFY(RemoteInfo::parseVerboseList({}).isEmpty());
        QVERIFY(RemoteInfo::parseVerboseList(QStringLiteral("garbage line\n")).isEmpty());
    }

    void managesRemotes()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.remotes().isEmpty());

        QVERIFY(repository.addRemote(QStringLiteral("origin"), QStringLiteral("https://example.com/a.git")).ok);
        QVERIFY(!repository.addRemote({}, QStringLiteral("https://example.com/a.git")).ok);
        QVERIFY(!repository.addRemote(QStringLiteral("other"), {}).ok);

        QList<RemoteInfo> remotes = repository.remotes();
        QCOMPARE(remotes.size(), 1);
        QCOMPARE(remotes.at(0).fetchUrl, QStringLiteral("https://example.com/a.git"));

        QVERIFY(repository.setRemoteUrl(QStringLiteral("origin"), QStringLiteral("https://example.com/b.git")).ok);
        QCOMPARE(repository.remotes().constFirst().fetchUrl, QStringLiteral("https://example.com/b.git"));

        QVERIFY(repository.renameRemote(QStringLiteral("origin"), QStringLiteral("upstream")).ok);
        QCOMPARE(repository.remotes().constFirst().name, QStringLiteral("upstream"));

        QVERIFY(repository.removeRemote(QStringLiteral("upstream")).ok);
        QVERIFY(repository.remotes().isEmpty());
        QVERIFY(!repository.removeRemote(QStringLiteral("missing")).ok);
    }

    void fetchPullPushAgainstLocalRemote()
    {
        // Seed repo with one commit, mirrored into a bare "server".
        GuitTest::TempRepo seed;
        seed.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        seed.commit(QStringLiteral("Seed"));
        GuitTest::TempRepo server(true);
        seed.runOrFail({QStringLiteral("push"), server.path(), QStringLiteral("main")});

        GuitTest::TempRepo work;
        GitRepository repository;
        QVERIFY(repository.open(work.path()));
        QVERIFY(repository.addRemote(QStringLiteral("origin"), server.path()).ok);

        // Fetch brings the remote-tracking branch without touching HEAD.
        OperationResult fetched = waitForNetwork(repository, [&repository]() { repository.startFetch(QStringLiteral("origin"), false); });
        QVERIFY2(fetched.ok, qPrintable(fetched.message));
        QVERIFY(fetched.command.contains(QStringLiteral("git fetch")));
        bool sawTracking = false;
        for (const BranchInfo &branch : repository.branches()) {
            if (branch.isRemote && branch.name == QStringLiteral("origin/main"))
                sawTracking = true;
        }
        QVERIFY(sawTracking);

        // Base local main on the fetched branch (fresh TempRepos start
        // unborn) and track it, so a bare `git pull` knows what to merge.
        // A commit lands on the server first so pull has something to do.
        work.runOrFail({QStringLiteral("checkout"), QStringLiteral("-B"), QStringLiteral("main"),
                        QStringLiteral("origin/main")});
        work.runOrFail({QStringLiteral("branch"), QStringLiteral("--set-upstream-to=origin/main")});
        seed.runOrFail({QStringLiteral("checkout"), QStringLiteral("main")});
        seed.writeFile(QStringLiteral("c.txt"), QStringLiteral("c\n"));
        seed.commit(QStringLiteral("Server side"));
        seed.runOrFail({QStringLiteral("push"), server.path(), QStringLiteral("main")});

        OperationResult pulled = waitForNetwork(repository, [&repository]() {
            repository.startPull(QStringLiteral("origin"));
        });
        QVERIFY2(pulled.ok, qPrintable(pulled.message));
        bool sawPulled = false;
        for (const CommitInfo &commit : repository.log()) {
            if (commit.subject == QStringLiteral("Server side"))
                sawPulled = true;
        }
        QVERIFY(sawPulled);

        // Now push local work on top (fast-forward on the server).
        work.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        work.commit(QStringLiteral("Work"));
        OperationResult pushed = waitForNetwork(repository, [&repository]() {
            repository.startPush(QStringLiteral("origin"), QStringLiteral("main"), true, false);
        });
        QVERIFY2(pushed.ok, qPrintable(pushed.message));
        const auto serverLog =
            GuitTest::TempRepo::runIn(server.path(), {QStringLiteral("log"), QStringLiteral("--format=%s")});
        QCOMPARE(serverLog.first, 0);
        QVERIFY(serverLog.second.contains(QStringLiteral("Work")));
    }

    void pushTagAndDeleteRemoteBranch()
    {
        GuitTest::TempRepo seed;
        seed.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        seed.commit(QStringLiteral("Seed"));
        GuitTest::TempRepo server(true);
        seed.runOrFail({QStringLiteral("push"), server.path(), QStringLiteral("main")});

        GuitTest::TempRepo work;
        GitRepository repository;
        QVERIFY(repository.open(work.path()));
        QVERIFY(repository.addRemote(QStringLiteral("origin"), server.path()).ok);
        QVERIFY(waitForNetwork(repository, [&repository]() { repository.startFetch(QStringLiteral("origin"), false); }).ok);
        // Base local main on the fetched branch (a fresh repo's unborn main
        // cannot be "switched" onto the remote-tracking branch).
        work.runOrFail({QStringLiteral("checkout"), QStringLiteral("-B"), QStringLiteral("main"),
                        QStringLiteral("origin/main")});

        QVERIFY(repository.createTag(QStringLiteral("v1.0"), QStringLiteral("Release"), {}).ok);
        OperationResult tagPushed = waitForNetwork(repository, [&repository]() {
            repository.startPush(QStringLiteral("origin"), QStringLiteral("v1.0"), false, false, false);
        });
        QVERIFY2(tagPushed.ok, qPrintable(tagPushed.message));
        const auto serverTags = GuitTest::TempRepo::runIn(server.path(), {QStringLiteral("tag"), QStringLiteral("--list")});
        QCOMPARE(serverTags.first, 0);
        QVERIFY(serverTags.second.contains(QStringLiteral("v1.0")));

        // Push a branch, then delete it remotely.
        QVERIFY(repository.createBranch(QStringLiteral("temp"), {}, true).ok);
        QVERIFY(waitForNetwork(repository, [&repository]() {
            repository.startPush(QStringLiteral("origin"), QStringLiteral("temp"), false, false, false);
        }).ok);
        OperationResult deleted = waitForNetwork(repository, [&repository]() {
            repository.startPush(QStringLiteral("origin"), QStringLiteral("temp"), false, false, true);
        });
        QVERIFY2(deleted.ok, qPrintable(deleted.message));
        const auto serverBranches =
            GuitTest::TempRepo::runIn(server.path(), {QStringLiteral("branch"), QStringLiteral("--list")});
        QCOMPARE(serverBranches.first, 0);
        QVERIFY(!serverBranches.second.contains(QStringLiteral("temp")));
    }

private:
    // Waits for one async network operation and returns its result.
    // The starter lambda (e.g. a startFetch call) runs after the spy is
    // installed so the finished signal cannot be missed.
    template <typename Starter>
    OperationResult waitForNetwork(GitRepository &repository, Starter start)
    {
        QSignalSpy spy(&repository, &GitRepository::networkFinished);
        start();
        if (!spy.wait(60000)) {
            QTest::qFail("Network operation timed out", __FILE__, __LINE__);
            return {};
        }
        if (spy.count() != 1) {
            QTest::qFail("Expected exactly one networkFinished signal", __FILE__, __LINE__);
            return {};
        }
        return spy.at(0).at(0).value<OperationResult>();
    }
};

QTEST_MAIN(TestRemotes)
#include "tst_remotes.moc"
