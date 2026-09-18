#include "GraphLanes.h"

namespace Guit
{

QList<QColor> GraphLanes::palette()
{
    return {
        QColor(0x2F7AD6), // blue
        QColor(0x1A7F37), // green
        QColor(0x9A6700), // dark yellow (readable on both themes)
        QColor(0xCF222E), // red
        QColor(0x8250DF), // purple
        QColor(0x1B7C83), // teal
        QColor(0xD65C00), // orange
        QColor(0x59636E), // gray
    };
}

QList<GraphRow> GraphLanes::compute(const QList<CommitInfo> &commits)
{
    QList<GraphRow> rows;
    rows.reserve(commits.size());
    if (commits.isEmpty())
        return rows;

    // Row index of every commit for parent lookups. Parents beyond the
    // loaded window (or otherwise unknown) target the next row: their
    // lines run off the bottom instead of breaking the layout.
    QMap<QString, int> rowOf;
    for (int i = 0; i < commits.size(); ++i)
        rowOf.insert(commits.at(i).hash, i);
    const auto targetOf = [&](const QString &hash, int row) -> int {
        return rowOf.contains(hash) ? rowOf.value(hash) - row : 1;
    };

    // Active lanes, one expected commit hash each. Finished lanes keep
    // their slot as empty (free for reuse) so indices stay stable.
    QList<QString> lanes;
    const auto acquireLane = [&](const QString &hash) -> int {
        const int found = static_cast<int>(lanes.indexOf(hash));
        if (found >= 0)
            return found;
        const int free = static_cast<int>(lanes.indexOf(QString()));
        if (free >= 0) {
            lanes[free] = hash;
            return free;
        }
        lanes.append(hash);
        return static_cast<int>(lanes.size()) - 1;
    };

    for (int row = 0; row < commits.size(); ++row) {
        const CommitInfo &commit = commits.at(row);
        GraphRow graph;
        graph.hash = commit.hash;

        const int lane = acquireLane(commit.hash);
        graph.dotLane = lane;

        // Every other live lane passes straight through this row.
        for (int l = 0; l < lanes.size(); ++l) {
            if (l != lane && !lanes.at(l).isEmpty())
                graph.segments.append(GraphSegment{l, 0, l, 1});
        }

        const QStringList parents = commit.parents;
        if (parents.isEmpty()) {
            lanes[lane].clear(); // root: the line ends here
        } else {
            const QString first = parents.constFirst();
            const int firstLane = lanes.indexOf(first);
            if (firstLane < 0) {
                lanes[lane] = first; // straight continuation
                graph.segments.append(GraphSegment{lane, 0, lane, targetOf(first, row)});
            } else if (firstLane == lane) {
                graph.segments.append(GraphSegment{lane, 0, lane, targetOf(first, row)});
            } else {
                lanes[lane].clear(); // join into the parent's lane
                graph.segments.append(GraphSegment{lane, 0, firstLane, targetOf(first, row)});
            }
            // Extra parents fork new (or reuse existing) lanes.
            for (int p = 1; p < parents.size(); ++p) {
                const QString &parent = parents.at(p);
                const int parentLane = acquireLane(parent);
                graph.segments.append(GraphSegment{lane, 0, parentLane, targetOf(parent, row)});
            }
        }
        rows.append(graph);
    }
    return rows;
}

} // namespace Guit
