#pragma once

#include <QPointF>

class QGraphicsItem;

class Collision
{
public:
    // 检查两个物体是否碰撞（基于矩形边界框）（基于Qt内置方法）
    static bool checkCollision(const QGraphicsItem* item1, const QGraphicsItem* item2);

    // 检查点与物体是否碰撞
    static bool checkPointCollision(const QPointF& point, const QGraphicsItem* item, qreal boxSize);

    // 检查移动后的位置是否会与物体碰撞
    static bool willCollide(const QPointF& currentPos, const QPointF& moveDirection,qreal moveSpeed, const QGraphicsItem* obstacle, qreal boxSize);

    // 获取两个物体中心点之间的距离
    static QPointF getDistance(const QPointF& pos1, const QPointF& pos2);
    static qreal EuclidDistance(const QPointF& pos1, const QPointF& pos2 = QPointF(0, 0));
};

