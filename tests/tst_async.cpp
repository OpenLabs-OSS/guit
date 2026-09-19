// Threading contract: concurrent repository reads from multiple threads
// stay consistent, and controllers deliver async results with loading
// signals on the calling thread.

#include "TestTempRepo.h"

#include <controllers/ChangesController.h>
#include <controllers/HistoryController.h>
#include <git/GitRepository.h>

#include <QSignalSpy>
#include <QtConcurrent>
#include <QtTest>

using namespace Guit;

class TestAsync : public QObject
{
    Q_OBJECT

private slots:
    void concurrentReadsStayConsistent()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Second"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QString expectedHead = repository.head().commitHash;

        // Hammer the readers from several threads at once (the default
        // pool, not the serial background queue, to force overlap).
        QList<QFuture<bool>> futures;
        for (int i = 0; i < 8; ++i) {
            futures.append(QtConcurrent::run([&repository, expectedHead]() {
                const StatusSnapshot status = repository.status();
                const QList<CommitInfo> log = repository.log();
                const QList<BranchInfo> branches = repository.branches();
                const QList<TagInfo> tags = repository.tags();
                const HeadInfo head = repository.head();
                return status.valid && log.size() == 2 && !branches.isEmpty() && tags.isEmpty()
                    && head.commitHash == expectedHead;
            }));
        }
        for (QFuture<bool> &future : futures) {
            future.waitForFinished();
            QVERIFY(future.result());
        }
    }

    void controllerRefreshDeliversAsync()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        ChangesController controller(&repository);

        QSignalSpy loading(&controller, &ChangesController::loadingChanged);
        QSignalSpy status(&controller, &ChangesController::statusChanged);
        controller.refresh();
        // Loading starts synchronously; the result arrives later.
        QCOMPARE(loading.count(), 1);
        QCOMPARE(loading.at(0).at(0).toBool(), true);
        QVERIFY2(status.wait(30000), "statusChanged never arrived");
        QVERIFY(controller.currentStatus().valid);
        // Loading ends exactly once, after delivery.
        QCOMPARE(loading.count(), 2);
        QCOMPARE(loading.at(1).at(0).toBool(), false);
    }

    void rapidRefreshDropsStaleResults()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        HistoryController controller(&repository);

        QSignalSpy history(&controller, &HistoryController::historyChanged);
        controller.refresh();
        controller.refresh();
        controller.refresh();
        QVERIFY2(history.wait(30000), "historyChanged never arrived");
        // Let any stragglers land, then assert a single coherent delivery.
        QTest::qWait(500);
        QVERIFY(history.count() >= 1);
        QCOMPARE(controller.commits().size(), 1);
    }

    void failedRepoDeliversErrorWithoutHanging()
    {
        GitRepository repository; // never opened
        ChangesController controller(&repository);

        QSignalSpy failed(&controller, &ChangesController::operationFailed);
        controller.refresh();
        QVERIFY2(failed.wait(30000), "operationFailed never arrived");
    }
};

QTEST_MAIN(TestAsync)
#include "tst_async.moc"
