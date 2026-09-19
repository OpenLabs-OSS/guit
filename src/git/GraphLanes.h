#pragma once

#include "GitModels.h"

#include <QColor>
#include <QList>
#include <QMap>
#include <QString>

namespace Guit
{

// One segment of a commit-graph row, in lane coordinates. Rows are laid
// out newest-first; a segment connects (laneA, rowA) to (laneB, rowB)
// where rows are relative to the commit's own row (0 = this row,
// 1 = the row below, ...).
struct GraphSegment
{
    int laneA = 0;
    int rowA = 0;
    int laneB = 0;
    int rowB = 1;
};

struct GraphRow
{
    QString hash;
    int dotLane = 0;
    QList<GraphSegment> segments;
};

// Assigns lanes to an ordered (newest-first) commit list so the history
// can be drawn as a graph. The algorithm tracks one "active" lane per
// open line of history: a commit takes over the lane waiting for it (or
// opens a new one), then hands lanes to its parents. Merges fork/join
// lanes; a parent outside the loaded window terminates its line at the
// boundary instead of connecting into another row.
class GraphLanes
{
public:
    static QList<GraphRow> compute(const QList<CommitInfo> &commits);

    // Lane colors that stay readable on light and dark themes.
    static QList<QColor> palette();
};

} // namespace Guit
