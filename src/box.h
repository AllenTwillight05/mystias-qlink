#ifndef BOX_H
#define BOX_H

#include<QGraphicsPixmapItem>
#include<QObject>

class Box : public QGraphicsPixmapItem
{
public:
    //初始化，explicit防止隐式类型转换，QString是Qt的字符类型
    explicit Box(const QPointF &pos, const QString &imagePath, QGraphicsScene *scene);
    explicit Box(const QString &imagePath, QGraphicsScene *scene, const QPointF& characterPos);
    void push(const QPointF &moveDirection,const qreal mapWidth, const qreal mapHeight);// 推动箱子
    const qreal boxSize = 32;//用于碰撞检测的距离
private:
    void setupSprite(const QPixmap &imagePath, int frameSize = 26);//帧大小
    void sidePush(const QPointF &moveDirection);//推箱子辅助函数
    QPointF generateRandomPosition(const QRectF &sceneRect,const QPointF &characterPos);//随机位置辅助构造函数
    qreal PUSH_SPEED = 8.0; // 与角色移动速度一致
};

#endif // BOX_H

