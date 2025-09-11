#pragma once

#include <QPointF>

class QPainter;
class QPainterPath;

class Path
{
public:
    static void draw(QPainter* painter, const QVector<QPointF>& points);
    // void drawLine2(QPainter* painter, const QPointF& p1, const QPointF& p2);
    // void drawLine3(QPainter* painter, const QPointF& p1, const QPointF& p2, const QPointF& p3);
    // void drawLine4(QPainter* painter, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPointF& p4);
};


