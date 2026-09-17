// Diff parser: synthetic unified diffs plus integration against real
// `git diff` output from a temporary repository.

#include "TestTempRepo.h"

#include <git/DiffInfo.h>
#include <git/GitRepository.h>

#include <QtTest>

using namespace Guit;

class TestDiff : public QObject
{
    Q_OBJECT

private slots:
    void parsesModification()
    {
        const QString output = QStringLiteral(
            "diff --git a/file.txt b/file.txt\n"
            "index 257cc56..5716ca5 100644\n"
            "--- a/file.txt\n"
            "+++ b/file.txt\n"
            "@@ -1,3 +1,4 @@\n"
            " line one\n"
            "-old line\n"
            "+new line\n"
            "+added line\n"
            " line three\n");
        const QList<FileDiff> diffs = parseUnifiedDiff(output);
        QCOMPARE(diffs.size(), 1);
        QCOMPARE(diffs.at(0).oldPath, QStringLiteral("file.txt"));
        QCOMPARE(diffs.at(0).newPath, QStringLiteral("file.txt"));
        QVERIFY(!diffs.at(0).isNewFile);
        QVERIFY(!diffs.at(0).isDeleted);
        QCOMPARE(diffs.at(0).added, 2);
        QCOMPARE(diffs.at(0).removed, 1);
        QCOMPARE(diffs.at(0).hunks.size(), 1);
        const DiffHunk &hunk = diffs.at(0).hunks.constFirst();
        QCOMPARE(hunk.oldStart, 1);
        QCOMPARE(hunk.oldCount, 3);
        QCOMPARE(hunk.newStart, 1);
        QCOMPARE(hunk.newCount, 4);
        QCOMPARE(hunk.lines.size(), 5);
    }

    void parsesNewAndDeletedFiles()
    {
        const QString output = QStringLiteral(
            "diff --git a/new.txt b/new.txt\n"
            "new file mode 100644\n"
            "index 0000000..5716ca5\n"
            "--- /dev/null\n"
            "+++ b/new.txt\n"
            "@@ -0,0 +1,2 @@\n"
            "+one\n"
            "+two\n"
            "diff --git a/gone.txt b/gone.txt\n"
            "deleted file mode 100644\n"
            "index 5716ca5..0000000\n"
            "--- a/gone.txt\n"
            "+++ /dev/null\n"
            "@@ -1,2 +0,0 @@\n"
            "-one\n"
            "-two\n");
        const QList<FileDiff> diffs = parseUnifiedDiff(output);
        QCOMPARE(diffs.size(), 2);
        QVERIFY(diffs.at(0).isNewFile);
        QCOMPARE(diffs.at(0).added, 2);
        QCOMPARE(diffs.at(0).summary(), QStringLiteral("+2"));
        QVERIFY(diffs.at(1).isDeleted);
        QCOMPARE(diffs.at(1).removed, 2);
        QCOMPARE(diffs.at(1).displayPath(), QStringLiteral("gone.txt"));
    }

    void parsesRenameAndBinary()
    {
        const QString output = QStringLiteral(
            "diff --git a/old.txt b/new.txt\n"
            "similarity index 90%\n"
            "rename from old.txt\n"
            "rename to new.txt\n"
            "index 5716ca5..d849bb1 100644\n"
            "--- a/old.txt\n"
            "+++ b/new.txt\n"
            "@@ -1 +1 @@\n"
            "-one\n"
            "+ONE\n"
            "diff --git a/image.png b/image.png\n"
            "new file mode 100644\n"
            "index 0000000..a684eca\n"
            "Binary files /dev/null and b/image.png differ\n");
        const QList<FileDiff> diffs = parseUnifiedDiff(output);
        QCOMPARE(diffs.size(), 2);
        QVERIFY(diffs.at(0).isRename);
        QCOMPARE(diffs.at(0).oldPath, QStringLiteral("old.txt"));
        QCOMPARE(diffs.at(0).newPath, QStringLiteral("new.txt"));
        QVERIFY(diffs.at(1).isBinary);
        QVERIFY(diffs.at(1).hunks.isEmpty());
    }

    void parsesMultipleHunksAndQuotedPaths()
    {
        const QString output = QStringLiteral(
            "diff --git \"a/my file.txt\" \"b/my file.txt\"\n"
            "index 5716ca5..d849bb1 100644\n"
            "--- \"a/my file.txt\"\n"
            "+++ \"b/my file.txt\"\n"
            "@@ -1,2 +1,2 @@ section one\n"
            " context\n"
            "-a\n"
            "+b\n"
            "@@ -10,2 +10,2 @@ section two\n"
            " context\n"
            "-c\n"
            "+d\n");
        const QList<FileDiff> diffs = parseUnifiedDiff(output);
        QCOMPARE(diffs.size(), 1);
        QCOMPARE(diffs.at(0).newPath, QStringLiteral("my file.txt"));
        QCOMPARE(diffs.at(0).hunks.size(), 2);
        QCOMPARE(diffs.at(0).hunks.at(0).sectionHeading, QStringLiteral(" section one"));
    }

    void skipsMalformedSections()
    {
        QVERIFY(parseUnifiedDiff({}).isEmpty());
        QVERIFY(parseUnifiedDiff(QStringLiteral("not a diff at all\njust text\n")).isEmpty());
        // Hunk-like lines outside any file are ignored, not fatal.
        QVERIFY(parseUnifiedDiff(QStringLiteral("@@ -1 +1 @@\n+x\n")).isEmpty());
    }

    void integrationUnstagedAndStaged()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("tracked.txt"), QStringLiteral("one\ntwo\nthree\n"));
        repo.commit(QStringLiteral("Initial commit"));
        repo.writeFile(QStringLiteral("tracked.txt"), QStringLiteral("one\nTWO\nthree\nfour\n"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));

        const QList<FileDiff> unstaged = repository.diffUnstaged();
        QCOMPARE(unstaged.size(), 1);
        QCOMPARE(unstaged.at(0).newPath, QStringLiteral("tracked.txt"));
        QCOMPARE(unstaged.at(0).added, 2);
        QCOMPARE(unstaged.at(0).removed, 1);
        QVERIFY(repository.diffStaged().isEmpty());

        QVERIFY(repository.stagePaths({QStringLiteral("tracked.txt")}).ok);
        QVERIFY(repository.diffUnstaged().isEmpty());
        const QList<FileDiff> staged = repository.diffStaged();
        QCOMPARE(staged.size(), 1);

        const QList<FileDiff> single = repository.diffStaged(QStringLiteral("tracked.txt"));
        QCOMPARE(single.size(), 1);
        QVERIFY(repository.diffStaged(QStringLiteral("no-such-file.txt")).isEmpty());
    }
};

QTEST_MAIN(TestDiff)
#include "tst_diff.moc"
