#pragma once

#include "../git/GraphLanes.h"

#include <QMap>
#include <QStyledItemDelegate>
#include <QString>

namespace Guit
{

// Paints one history row: commit-graph lanes on the left, then subject,
// ref badges, and a details line (hash · author · date).
class GraphDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    static constexpr int laneWidth = 16;
    static constexpr int rowHeight = 46;

    explicit GraphDelegate(QObject *parent = nullptr);

    void setGraph(const QList<GraphRow> &rows);
    void setRefs(const QMap<QString, QStringList> &refs);
    void setHeadHash(const QString &hash);
    void setRowHash(int row, const QString &hash);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QList<GraphRow> m_rows;
    QMap<QString, QStringList> m_refs;
    QString m_headHash;
    QMap<int, QString> m_rowHash;
};

} // namespace Guit
