#include "Collision.h"
#include <QGraphicsItem>
#include <cmath>

bool Collision::checkCollision(const QGraphicsItem* item1, const QGraphicsItem* item2)
{
    return item1->collidesWithItem(item2);//内置矩形边框检测
}

bool Collision::checkPointCollision(const QPointF& point, const QGraphicsItem* item, qreal boxSize)
{
    QPointF del = item->pos() - point;
    QPointF distance(std::abs(del.x()), std::abs(del.y()));
    return (distance.x() < boxSize && distance.y() < boxSize);
}

bool Collision::willCollide(const QPointF& currentPos, const QPointF& moveDirection,
                            qreal moveSpeed, const QGraphicsItem* obstacle, qreal boxSize)
{
    QPointF newPos = currentPos + moveDirection * moveSpeed;
    return checkPointCollision(newPos, obstacle, boxSize);
}

QPointF Collision::getDistance(const QPointF& pos1, const QPointF& pos2)
{
    return pos2 - pos1;
}
