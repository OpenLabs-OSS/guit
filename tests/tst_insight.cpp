// Repository insight: reflog, search, refs, LFS, submodules, worktrees,
// .gitignore, and repository facts.

#include "TestTempRepo.h"

#include <git/AdvancedModels.h>
#include <git/GitRepository.h>

#include <QDir>
#include <QFile>
#include <QtTest>

using namespace Guit;

class TestInsight : public QObject
{
    Q_OBJECT

private slots:
    void parsesReflog()
    {
        const QString output = QStringLiteral(
            "abc123\x1f" "HEAD@{0}\x1f" "commit: Second\x1f" "Guit Test\x1f" "2024-01-02T03:04:05+00:00\x1e"
            "def456\x1f" "HEAD@{1}\x1f" "commit (initial): First\x1f" "Guit Test\x1f" "2024-01-01T03:04:05+00:00\x1e");
        const QList<ReflogEntry> entries = ReflogEntry::parse(output);
        QCOMPARE(entries.size(), 2);
        QCOMPARE(entries.at(0).hash, QStringLiteral("abc123"));
        QCOMPARE(entries.at(0).ref, QStringLiteral("HEAD@{0}"));
        QCOMPARE(entries.at(0).message, QStringLiteral("commit: Second"));
        QVERIFY(entries.at(0).date.isValid());
        QVERIFY(ReflogEntry::parse({}).isEmpty());
        QVERIFY(ReflogEntry::parse(QStringLiteral("too\x1f" "few")).isEmpty());
    }

    void readsRealReflog()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Second"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QList<ReflogEntry> entries = repository.reflog();
        QVERIFY(entries.size() >= 3);
        QCOMPARE(entries.constFirst().ref, QStringLiteral("HEAD@{0}"));
        QVERIFY(!entries.constFirst().hash.isEmpty());
        QVERIFY(!entries.constFirst().message.isEmpty());
        QVERIFY(repository.reflog(1).size() <= 1);
    }

    void searchesCommits()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Fix login bug"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Add dashboard"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        const QList<CommitInfo> bySubject = repository.searchCommits(QStringLiteral("login"));
        QCOMPARE(bySubject.size(), 1);
        QCOMPARE(bySubject.constFirst().subject, QStringLiteral("Fix login bug"));

        const QList<CommitInfo> byAuthor = repository.searchCommits(QStringLiteral("guit test"));
        QCOMPARE(byAuthor.size(), 2);

        const QString prefix = repository.log().constFirst().hash.left(7);
        QCOMPARE(repository.searchCommits(prefix).size(), 1);

        QVERIFY(repository.searchCommits(QStringLiteral("no-such-thing-xyz")).isEmpty());
        QVERIFY(repository.searchCommits({}).isEmpty());
        QVERIFY(repository.searchCommits(QStringLiteral("   ")).isEmpty());
    }

    void mapsRefsByHash()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.createTag(QStringLiteral("v1.0"), {}, {}).ok);

        const QString head = repository.head().commitHash;
        const QMap<QString, QStringList> refs = repository.refsByHash();
        QVERIFY(refs.contains(head));
        QVERIFY(refs.value(head).contains(QStringLiteral("main")));
        QVERIFY(refs.value(head).contains(QStringLiteral("v1.0")));
    }

    void detectsLfs()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const LfsInfo info = repository.lfsInfo();
        if (!info.available)
            QSKIP("git-lfs is not installed");
        QVERIFY(!info.version.isEmpty());
        QVERIFY(!info.enabledInRepo);

        QVERIFY(repository.lfsTrack(QStringLiteral("*.bin")).ok);
        QFile attributes(QDir(repo.path()).filePath(QStringLiteral(".gitattributes")));
        QVERIFY(attributes.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(attributes.readAll()).contains(QStringLiteral("filter=lfs")));
        QVERIFY(!repository.lfsTrack({}).ok);

        // Repo-local LFS filters (no global machine state touched), then a
        // real LFS-tracked file.
        GuitTest::TempRepo::runIn(repo.path(), {QStringLiteral("lfs"), QStringLiteral("install"), QStringLiteral("--local")});
        QFile binary(QDir(repo.path()).filePath(QStringLiteral("big.bin")));
        QVERIFY(binary.open(QIODevice::WriteOnly));
        binary.write(QByteArray(2048, 'x'));
        binary.close();
        repo.commit(QStringLiteral("Add binary"));
        QVERIFY(repository.lfsInfo().enabledInRepo);
        QVERIFY(repository.lfsInfo().trackedFiles >= 1);
    }

    void managesSubmodules()
    {
        GuitTest::TempRepo library;
        library.writeFile(QStringLiteral("lib.txt"), QStringLiteral("lib\n"));
        library.commit(QStringLiteral("Library"));

        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.runOrFail({QStringLiteral("-c"), QStringLiteral("protocol.file.allow=always"), QStringLiteral("submodule"),
                        QStringLiteral("add"), library.path(), QStringLiteral("vendor/lib")});
        repo.commit(QStringLiteral("Add submodule"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QList<SubmoduleInfo> modules = repository.submodules();
        QCOMPARE(modules.size(), 1);
        QCOMPARE(modules.at(0).path, QStringLiteral("vendor/lib"));
        QVERIFY(modules.at(0).initialized());
        QVERIFY(modules.at(0).inSync());

        QVERIFY(repository.submoduleUpdate(true).ok);
        QVERIFY(repository.submoduleSync().ok);

        // Malformed lines never crash the parser.
        QVERIFY(SubmoduleInfo::parseStatus({}).isEmpty());
    }

    void managesWorktrees()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QCOMPARE(repository.worktrees().size(), 1);
        QVERIFY(repository.worktrees().constFirst().isMain);

        const QString wtPath = QDir(repo.path()).filePath(QStringLiteral("wt-feature"));
        QVERIFY(repository.worktreeAdd(wtPath, QStringLiteral("feature"), true).ok);
        QVERIFY(QDir(wtPath).exists());
        const QList<WorktreeInfo> trees = repository.worktrees();
        QCOMPARE(trees.size(), 2);
        bool sawFeature = false;
        for (const WorktreeInfo &tree : trees) {
            if (tree.branch == QStringLiteral("feature"))
                sawFeature = true;
        }
        QVERIFY(sawFeature);

        QVERIFY(repository.worktreeRemove(wtPath, false).ok);
        QCOMPARE(repository.worktrees().size(), 1);
        QVERIFY(repository.worktreePrune().ok);
        QVERIFY(!repository.worktreeAdd({}, QStringLiteral("x"), true).ok);
    }

    void editsGitignore()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.readGitignore().isEmpty());

        QVERIFY(repository.writeGitignore(QStringLiteral("# build\nbuild/\n")).ok);
        QCOMPARE(repository.readGitignore(), QStringLiteral("# build\nbuild/\n"));

        // Presets provide real content.
        const QMap<QString, QStringList> presets = GitRepository::gitignorePresets();
        QVERIFY(presets.contains(QStringLiteral("Python")));
        QVERIFY(!presets.value(QStringLiteral("Windows")).isEmpty());

        GitRepository closed; // never opened
        QVERIFY(!closed.writeGitignore(QStringLiteral("x")).ok);
    }

    void reportsRepositoryInfo()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Second"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.addRemote(QStringLiteral("origin"), QStringLiteral("https://example.com/a.git")).ok);
        QVERIFY(repository.createTag(QStringLiteral("v1.0"), {}, {}).ok);
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\nchanged\n"));
        QVERIFY(repository.stashPush({}, false).ok);

        const RepoInfo info = repository.repositoryInfo();
        QVERIFY(info.valid);
        QCOMPARE(QDir::cleanPath(info.rootPath), QDir::cleanPath(repo.path()));
        QVERIFY(!info.isBare);
        QCOMPARE(info.branch, QStringLiteral("main"));
        QCOMPARE(info.commitCount, 2);
        QCOMPARE(info.remoteCount, 1);
        QCOMPARE(info.tagCount, 1);
        QCOMPARE(info.stashCount, 1);
        QVERIFY(info.branchCount >= 1);
        QVERIFY(info.gitDirSizeBytes > 0);
        QVERIFY(!info.headHash.isEmpty());

        GitRepository closed;
        QVERIFY(!closed.repositoryInfo().valid);
    }
};

QTEST_MAIN(TestInsight)
#include "tst_insight.moc"
