// Tag workflows: format parsing, lightweight/annotated creation,
// inspection, deletion, and failure paths.

#include "TestTempRepo.h"

#include <git/AdvancedModels.h>
#include <git/GitRepository.h>

#include <QtTest>

using namespace Guit;

class TestTags : public QObject
{
    Q_OBJECT

private slots:
    void parsesForEachRef()
    {
        const QString lightweight = QStringLiteral("v0.9\x1f" "commit\x1f" "abc123\x1f" "\x1f" "\x1f" "\x1f" "Initial commit");
        const QString annotated = QStringLiteral("v1.0\x1f" "tag\x1f" "def456\x1f" "abc123\x1f" "Guit Test\x1f"
                                                 "2024-01-02T03:04:05+00:00\x1f" "Release notes");
        const QList<TagInfo> tags = TagInfo::parseForEachRef(lightweight + QStringLiteral("\n") + annotated);
        QCOMPARE(tags.size(), 2);
        QCOMPARE(tags.at(0).name, QStringLiteral("v0.9"));
        QVERIFY(!tags.at(0).annotated);
        QCOMPARE(tags.at(0).targetHash, QStringLiteral("abc123"));
        QCOMPARE(tags.at(1).name, QStringLiteral("v1.0"));
        QVERIFY(tags.at(1).annotated);
        QCOMPARE(tags.at(1).targetHash, QStringLiteral("abc123")); // peeled commit, not the tag object
        QCOMPARE(tags.at(1).tagger, QStringLiteral("Guit Test"));
        QVERIFY(tags.at(1).date.isValid());
        QCOMPARE(tags.at(1).message, QStringLiteral("Release notes"));

        QVERIFY(TagInfo::parseForEachRef({}).isEmpty());
        QVERIFY(TagInfo::parseForEachRef(QStringLiteral("incomplete\x1f" "line")).isEmpty());
    }

    void createsLightweightAndAnnotated()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.tags().isEmpty());

        QVERIFY(repository.createTag(QStringLiteral("v0.9"), {}, {}).ok);
        QVERIFY(repository.createTag(QStringLiteral("v1.0"), QStringLiteral("First release"), {}).ok);
        QVERIFY(!repository.createTag({}, {}, {}).ok);
        QVERIFY(!repository.createTag(QStringLiteral("v1.0"), {}, {}).ok); // duplicate

        const QList<TagInfo> tags = repository.tags();
        QCOMPARE(tags.size(), 2);
        bool sawLight = false;
        bool sawAnnotated = false;
        for (const TagInfo &tag : tags) {
            if (tag.name == QStringLiteral("v0.9") && !tag.annotated)
                sawLight = true;
            if (tag.name == QStringLiteral("v1.0") && tag.annotated && tag.message == QStringLiteral("First release"))
                sawAnnotated = true;
        }
        QVERIFY(sawLight);
        QVERIFY(sawAnnotated);
    }

    void createsTagOnTarget()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("First"));
        repo.writeFile(QStringLiteral("b.txt"), QStringLiteral("b\n"));
        repo.commit(QStringLiteral("Second"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        const QString firstHash = repository.log().last().hash;
        QVERIFY(repository.createTag(QStringLiteral("old"), {}, firstHash).ok);

        const CommitDetails details = repository.showTag(QStringLiteral("old"));
        QVERIFY(details.valid);
        QCOMPARE(details.info.subject, QStringLiteral("First"));
    }

    void inspectsAndDeletes()
    {
        GuitTest::TempRepo repo;
        repo.writeFile(QStringLiteral("a.txt"), QStringLiteral("a\n"));
        repo.commit(QStringLiteral("Initial commit"));

        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        QVERIFY(repository.createTag(QStringLiteral("v1.0"), QStringLiteral("Notes"), {}).ok);

        const CommitDetails details = repository.showTag(QStringLiteral("v1.0"));
        QVERIFY(details.valid);
        QCOMPARE(details.info.subject, QStringLiteral("Initial commit"));
        QVERIFY(!details.diffs.isEmpty()); // root commit diff via --root

        QVERIFY(!repository.showTag(QStringLiteral("missing")).valid);

        QVERIFY(repository.deleteTag(QStringLiteral("v1.0")).ok);
        QVERIFY(repository.tags().isEmpty());
        QVERIFY(!repository.deleteTag(QStringLiteral("v1.0")).ok);
    }
};

QTEST_MAIN(TestTags)
#include "tst_tags.moc"
