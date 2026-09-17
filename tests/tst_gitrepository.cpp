// GitRepository: opening valid/invalid directories, HEAD state on normal,
// detached, and unborn (no commits) repositories.

#include "TestTempRepo.h"

#include <git/GitRepository.h>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace Guit;

class TestGitRepository : public QObject
{
    Q_OBJECT

private slots:
    void opensRepositoryFromSubdirectory()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));

        const QString subDir = QDir(repo.path()).filePath(QStringLiteral("a/b"));
        QVERIFY(QDir().mkpath(subDir));

        GitRepository repository;
        QSignalSpy changed(&repository, &GitRepository::repositoryChanged);
        QVERIFY(repository.open(subDir));
        QVERIFY(repository.isValid());
        QCOMPARE(QDir::cleanPath(repository.rootPath()), QDir::cleanPath(repo.path()));
        QCOMPARE(changed.count(), 1);
    }

    void reportsHeadOnNormalBranch()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const HeadInfo head = repository.head();
        QVERIFY(head.known);
        QVERIFY(!head.unborn);
        QVERIFY(!head.detached);
        QCOMPARE(head.branch, QStringLiteral("main"));
        QVERIFY(!head.commitHash.isEmpty());
    }

    void reportsUnbornRepository()
    {
        GuitTest::TempRepo repo; // no commits

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const HeadInfo head = repository.head();
        QVERIFY(head.known);
        QVERIFY(head.unborn);
        QVERIFY(!head.detached);
        QVERIFY(head.commitHash.isEmpty());
    }

    void reportsDetachedHead()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("file.txt"), QStringLiteral("data\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.runOrFail({QStringLiteral("checkout"), QStringLiteral("--detach"), QStringLiteral("HEAD")});

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const HeadInfo head = repository.head();
        QVERIFY(head.known);
        QVERIFY(!head.unborn);
        QVERIFY(head.detached);
        QVERIFY(head.branch.isEmpty());
        QVERIFY(!head.commitHash.isEmpty());
    }

    void rejectsNonRepository()
    {
        QTemporaryDir plainDir;
        QVERIFY(plainDir.isValid());

        GitRepository repository;
        QSignalSpy failed(&repository, &GitRepository::openFailed);
        QVERIFY(!repository.open(plainDir.path()));
        QVERIFY(!repository.isValid());
        QCOMPARE(failed.count(), 1);
    }

    void rejectsEmptyPath()
    {
        GitRepository repository;
        QVERIFY(!repository.open({}));
        QVERIFY(!repository.isValid());
    }

    void closesRepository()
    {
        GuitTest::TempRepo repo;
        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QSignalSpy closed(&repository, &GitRepository::repositoryClosed);
        repository.close();
        QVERIFY(!repository.isValid());
        QVERIFY(repository.rootPath().isEmpty());
        QCOMPARE(closed.count(), 1);
    }
};

QTEST_MAIN(TestGitRepository)
#include "tst_gitrepository.moc"
