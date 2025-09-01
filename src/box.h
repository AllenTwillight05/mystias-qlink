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
    int boxType = 0;//0空，1-164为类型
    bool boxCond = 0;//0未激活，1激活
    void activate();
    void preAct();
    void npreAct();
private:
    void setupSprite(const QPixmap &imagePath, int frameSize = 26);//帧大小
    QPointF generateRandomPosition(const QRectF &sceneRect,const QPointF &characterPos);//随机位置辅助构造函数
    QGraphicsRectItem* m_overlay = nullptr;  // 成员变量存储遮罩，用于预选中效果
};


