#include "path.h"
#include <QPainter>
#include <QPainterPath>

// 在两点之间绘制直线
// void Path::drawLine2(QPainter* painter, const QPointF& p1, const QPointF& p2) {
//     // QPen pen(Qt::yellow, 4);  // 黄色，4像素宽
//     // painter->setPen(pen);
//     // painter->drawLine(p1, p2);
//
//     QPainterPath path;
//     path.moveTo(p1);
//     path.lineTo(p2);
//     painter->setPen(QPen(Qt::yellow, 4, Qt::SolidLine));
//     painter->drawPath(path);
// }

void Path::draw(QPainter* painter, const QVector<QPointF>& points) {
    if (points.size() < 2) return; // 至少需要两个点

    QPainterPath path;
    path.moveTo(points[0]);
    for (int i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }

    painter->setPen(QPen(Qt::yellow, 4, Qt::SolidLine));
    painter->drawPath(path);
}
