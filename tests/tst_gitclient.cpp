// GitClient: executable detection, version query, and repository
// probing against isolated temporary repositories.

#include "TestTempRepo.h"

#include <git/GitClient.h>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace Guit;

class TestGitClient : public QObject
{
    Q_OBJECT

private slots:
    void findsGitExecutable()
    {
        GitClient client;
        QVERIFY2(client.hasGit(), "git must be installed for the test suite");
        QVERIFY(!client.gitExecutable().isEmpty());
    }

    void reportsVersion()
    {
        GitClient client;
        QVERIFY(client.hasGit());
        const GitVersion version = client.version();
        QVERIFY2(version.isValid(), qPrintable(QStringLiteral("raw: ") + version.raw));
        QVERIFY(version.major >= 2);
    }

    void detectsRepositoryRootFromSubdirectory()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("README.md"), QStringLiteral("hello\n"));
        repo.commit(QStringLiteral("Initial commit"));

        const QString subDir = QDir(repo.path()).filePath(QStringLiteral("sub/dir"));
        QVERIFY(QDir().mkpath(subDir));

        GitClient client;
        QVERIFY(client.isRepository(subDir));
        QCOMPARE(QDir::cleanPath(client.repositoryRoot(subDir)), QDir::cleanPath(repo.path()));

        const GitClient::RepositoryProbe probe = client.probeRepository(subDir);
        QVERIFY(probe.isRepository);
        QVERIFY(!probe.isBare);
        QVERIFY(!probe.gitDir.isEmpty());
    }

    void rejectsNonRepository()
    {
        QTemporaryDir plainDir;
        QVERIFY(plainDir.isValid());
        GitClient client;
        QVERIFY(!client.isRepository(plainDir.path()));
        QVERIFY(client.repositoryRoot(plainDir.path()).isEmpty());
        const GitClient::RepositoryProbe probe = client.probeRepository(plainDir.path());
        QVERIFY(!probe.isRepository);
    }

    void rejectsEmptyPath()
    {
        GitClient client;
        QVERIFY(!client.isRepository({}));
        QVERIFY(client.repositoryRoot({}).isEmpty());
    }

    void honorsExecutableOverride()
    {
        GitClient client(QStringLiteral("/nonexistent-directory-xyz/guit-no-such-git"));
        QVERIFY(!client.hasGit());
        QVERIFY(!client.version().isValid());
        const GitProcessResult result = client.run({QStringLiteral("--version")}, {});
        QCOMPARE(result.error, GitError::GitNotFound);
    }

    void formatsEquivalentCommand()
    {
        GitClient client;
        QCOMPARE(client.equivalentCommand({QStringLiteral("add"), QStringLiteral("--"), QStringLiteral("README.md")}),
                 QStringLiteral("git add -- README.md"));
    }
};

QTEST_MAIN(TestGitClient)
#include "tst_gitclient.moc"
