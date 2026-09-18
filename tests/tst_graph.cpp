// Graph lane assignment: linear history, forks, merges, unknown
// parents, and empty input.

#include <git/GraphLanes.h>

#include <QtTest>

using namespace Guit;

static CommitInfo makeCommit(const QString &hash, const QStringList &parents = {})
{
    CommitInfo commit;
    commit.hash = hash;
    commit.parents = parents;
    commit.subject = hash;
    return commit;
}

class TestGraph : public QObject
{
    Q_OBJECT

private slots:
    void assignsLinearHistoryToOneLane()
    {
        const QList<CommitInfo> commits = {makeCommit(QStringLiteral("c3"), {QStringLiteral("c2")}),
                                           makeCommit(QStringLiteral("c2"), {QStringLiteral("c1")}),
                                           makeCommit(QStringLiteral("c1"))};
        const QList<GraphRow> rows = GraphLanes::compute(commits);
        QCOMPARE(rows.size(), 3);
        for (const GraphRow &row : rows)
            QCOMPARE(row.dotLane, 0);
        // Each row continues straight into its parent below.
        QCOMPARE(rows.at(0).segments.size(), 1);
        QCOMPARE(rows.at(0).segments.at(0).laneA, 0);
        QCOMPARE(rows.at(0).segments.at(0).laneB, 0);
        QCOMPARE(rows.at(0).segments.at(0).rowB, 1);
        // The root commit ends its lane without a continuation segment.
        QVERIFY(rows.at(2).segments.isEmpty());
    }

    void forksAndJoinsBranches()
    {
        // main: m2 -> m1; feature f1 branched from m1; merge mx joins f1.
        const QList<CommitInfo> commits = {makeCommit(QStringLiteral("mx"), {QStringLiteral("m2"), QStringLiteral("f1")}),
                                           makeCommit(QStringLiteral("m2"), {QStringLiteral("m1")}),
                                           makeCommit(QStringLiteral("f1"), {QStringLiteral("m1")}),
                                           makeCommit(QStringLiteral("m1"))};
        const QList<GraphRow> rows = GraphLanes::compute(commits);
        QCOMPARE(rows.size(), 4);
        // The merge opens a second lane for its extra parent.
        QVERIFY(rows.at(0).segments.size() >= 2);
        // Feature commit sits on a different lane than main's tip.
        QVERIFY(rows.at(2).dotLane != rows.at(1).dotLane);
        // Every row's dot lane is non-negative and segments reference
        // non-negative lanes.
        for (const GraphRow &row : rows) {
            QVERIFY(row.dotLane >= 0);
            for (const GraphSegment &segment : row.segments) {
                QVERIFY(segment.laneA >= 0);
                QVERIFY(segment.laneB >= 0);
                QVERIFY(segment.rowB >= 1);
            }
        }
    }

    void survivesUnknownParents()
    {
        // Parent outside the loaded window: line runs off the bottom.
        const QList<CommitInfo> commits = {makeCommit(QStringLiteral("c2"), {QStringLiteral("c1-missing")})};
        const QList<GraphRow> rows = GraphLanes::compute(commits);
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0).dotLane, 0);
        QCOMPARE(rows.at(0).segments.size(), 1);
        QCOMPARE(rows.at(0).segments.at(0).rowB, 1);
    }

    void handlesEmptyInput()
    {
        QVERIFY(GraphLanes::compute({}).isEmpty());
    }

    void paletteIsUsable()
    {
        const QList<QColor> palette = GraphLanes::palette();
        QVERIFY(palette.size() >= 4);
        for (const QColor &color : palette)
            QVERIFY(color.isValid());
    }
};

QTEST_MAIN(TestGraph)
#include "tst_graph.moc"
