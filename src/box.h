#pragma once

#include<QGraphicsPixmapItem>
#include<QObject>

class Box : public QGraphicsPixmapItem
{
public:
    //初始化，explicit防止隐式类型转换，QString是Qt的字符类型
    explicit Box(const QPointF &pos, const QString &imagePath, QGraphicsScene *scene);
    explicit Box(const QString &imagePath, QGraphicsScene *scene, const QPointF& characterPos);
    const qreal boxSize = 45;//用于碰撞检测的距离
private:
    void setupSprite(const QPixmap &imagePath, int frameSize = 26);//帧大小
    QPointF generateRandomPosition(const QRectF &sceneRect,const QPointF &characterPos);//随机位置辅助构造函数
};


