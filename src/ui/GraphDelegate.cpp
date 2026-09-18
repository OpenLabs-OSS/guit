#include "GraphDelegate.h"

#include <QApplication>
#include <QPainter>

namespace Guit
{

GraphDelegate::GraphDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void GraphDelegate::setGraph(const QList<GraphRow> &rows)
{
    m_rows = rows;
}

void GraphDelegate::setRefs(const QMap<QString, QStringList> &refs)
{
    m_refs = refs;
}

void GraphDelegate::setHeadHash(const QString &hash)
{
    m_headHash = hash;
}

void GraphDelegate::setRowHash(int row, const QString &hash)
{
    m_rowHash.insert(row, hash);
}

static QPoint lanePoint(int lane, double row, const QRect &area, int laneCount)
{
    const double y = area.top() + row * area.height();
    const double x = area.left() + lane * GraphDelegate::laneWidth + GraphDelegate::laneWidth / 2.0;
    Q_UNUSED(laneCount);
    return QPoint(static_cast<int>(x), static_cast<int>(y));
}

void GraphDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    else if (option.state & QStyle::State_MouseOver)
        painter->fillRect(option.rect, option.palette.alternateBase());

    const int row = index.row();
    const QString hash = m_rowHash.value(row);
    const GraphRow graph = (row >= 0 && row < m_rows.size()) ? m_rows.at(row) : GraphRow();
    const QList<QColor> colors = GraphLanes::palette();

    // Lane area width covers the busiest row.
    int maxLane = graph.dotLane;
    for (const GraphSegment &segment : graph.segments)
        maxLane = qMax(maxLane, qMax(segment.laneA, segment.laneB));
    const int graphWidth = (maxLane + 1) * laneWidth + 4;
    const QRect graphArea(option.rect.left() + 2, option.rect.top(), graphWidth, option.rect.height());

    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const GraphSegment &segment : graph.segments) {
        const QColor color = colors.at(segment.laneA % colors.size());
        painter->setPen(QPen(color, 2));
        painter->drawLine(lanePoint(segment.laneA, segment.rowA + 0.5, graphArea, maxLane),
                          lanePoint(segment.laneB, segment.rowB + 0.5, graphArea, maxLane));
    }
    if (!hash.isEmpty()) {
        const QColor color = colors.at(graph.dotLane % colors.size());
        painter->setPen(QPen(color, 2));
        painter->setBrush(color);
        const QPoint center = lanePoint(graph.dotLane, 0.5, graphArea, maxLane);
        const bool isHead = hash == m_headHash;
        painter->drawEllipse(center, isHead ? 6 : 5, isHead ? 6 : 5);
        if (isHead) {
            painter->setPen(QPen(option.palette.text(), 1));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(center, 8, 8); // HEAD ring: "you are here"
        }
    }
    painter->setRenderHint(QPainter::Antialiasing, false);

    // Text: subject + ref badges, then details line.
    const int textLeft = graphArea.right() + 6;
    QRect textRect(textLeft, option.rect.top() + 2, option.rect.right() - textLeft - 2, option.rect.height() - 4);
    const QColor textColor = (option.state & QStyle::State_Selected) ? option.palette.highlightedText().color()
                                                                     : option.palette.text().color();
    painter->setPen(textColor);
    QFont subjectFont = option.font;
    painter->setFont(subjectFont);
    QString subject = index.data(Qt::DisplayRole).toString();
    const QStringList refs = m_refs.value(hash);
    if (!refs.isEmpty())
        subject += QStringLiteral("  [%1]").arg(refs.join(QStringLiteral(", ")));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, subject);

    QFont detailFont = option.font;
    detailFont.setPointSize(qMax(detailFont.pointSize() - 1, 7));
    detailFont.setItalic(true);
    painter->setFont(detailFont);
    painter->setPen((option.state & QStyle::State_Selected) ? option.palette.highlightedText().color()
                                                            : option.palette.placeholderText().color());
    const QString details = index.data(Qt::UserRole + 1).toString();
    painter->drawText(textRect.adjusted(0, 22, 0, 0), Qt::AlignLeft | Qt::AlignTop, details);
    painter->restore();
}

QSize GraphDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(200, rowHeight);
}

} // namespace Guit
