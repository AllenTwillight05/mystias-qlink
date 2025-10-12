#include "simpletest.h"
#include "collision.h"
#include <QGraphicsRectItem>
#include <QDebug>

void SimpleTest::testEuclidDistance()
{
    qDebug() << "Testing EuclidDistance...";

    // 测试用例1：相同点
    QPointF p1(0, 0);
    QCOMPARE(Collision::EuclidDistance(p1, p1), 0.0);

    // 测试用例2：水平距离
    QPointF p2(3, 0);
    QPointF p3(0, 0);
    QCOMPARE(Collision::EuclidDistance(p2, p3), 3.0);

    // 测试用例3：垂直距离
    QPointF p4(0, 4);
    QCOMPARE(Collision::EuclidDistance(p4, p3), 4.0);

    // 测试用例4：对角线距离
    QPointF p5(3, 4);
    QCOMPARE(Collision::EuclidDistance(p5, p3), 5.0);

    qDebug() << "EuclidDistance tests passed!";
}

void SimpleTest::testGetDistance()
{
    qDebug() << "Testing getDistance...";

    QPointF p1(1, 1);
    QPointF p2(4, 5);
    QPointF result = Collision::getDistance(p1, p2);

    QCOMPARE(result.x(), 3.0);
    QCOMPARE(result.y(), 4.0);

    qDebug() << "getDistance tests passed!";
}

void SimpleTest::testPointCollision()
{
    qDebug() << "Testing checkPointCollision...";

    // 创建一个测试图形项
    QGraphicsRectItem* item = new QGraphicsRectItem(-50, -50, 50, 50);
    item->setPos(100, 100);

    qreal boxSize = 45;

    // 点在物体中心 - 应该碰撞
    QPointF centerPoint(100, 100);
    QVERIFY(Collision::checkPointCollision(centerPoint, item, boxSize) == true);

    // 点在物体边缘内 - 应该碰撞
    QPointF edgePoint1(100 + 20, 100 + 20);
    QVERIFY(Collision::checkPointCollision(edgePoint1, item, boxSize) == true);

    // 点在物体边缘上 - 不应该碰撞  // corner case
    QPointF edgePoint2(100 + 22.5, 100 + 22.5);
    QVERIFY(Collision::checkPointCollision(edgePoint2, item, boxSize) == false);

    // 点在物体边缘上 - 不应该碰撞  // corner case
    QPointF edgePoint3(100 + 22.5, 100 + 50);
    QVERIFY(Collision::checkPointCollision(edgePoint3, item, boxSize) == false);

    // 点在物体边缘上 - 应该碰撞  // corner case
    QPointF edgePoint4(100 + 22, 100 + 22);
    QVERIFY(Collision::checkPointCollision(edgePoint4, item, boxSize) == true);

    // 点在物体外 - 不应该碰撞
    QPointF outsidePoint1(100 + 100, 100 + 100);
    QVERIFY(Collision::checkPointCollision(outsidePoint1, item, boxSize) == false);

    // 点在物体外 - 不应该碰撞
    QPointF outsidePoint2(100 + 100, 100);
    QVERIFY(Collision::checkPointCollision(outsidePoint2, item, boxSize) == false);

    delete item;
    qDebug() << "checkPointCollision tests passed!";
}

// void SimpleTest::testCheckCollision()
// {
//     qDebug() << "Testing checkCollision...";

//     // 创建两个重叠的图形项
//     QGraphicsRectItem* item1 = new QGraphicsRectItem(0, 0, 50, 50);
//     item1->setPos(100, 100);

//     QGraphicsRectItem* item2 = new QGraphicsRectItem(0, 0, 50, 50);
//     item2->setPos(100, 100);

//     // 应该碰撞
//     QVERIFY(Collision::checkCollision(item1, item2));

//     // 移动第二个物体使其不重叠
//     item2->setPos(200, 200);
//     QVERIFY(!Collision::checkCollision(item1, item2));

//     delete item1;
//     delete item2;
//     qDebug() << "checkCollision tests passed!";
// }

void SimpleTest::testWillCollide()
{
    qDebug() << "Testing willCollide...";

    QGraphicsRectItem* obstacle = new QGraphicsRectItem(-50, -50, 50, 50);
    obstacle->setPos(100, 100); // 原obstacle系的原点，即方块中心坐标（模拟box经过offset传入的中心坐标）
    // 模拟box大小其实不重要，因为判断基于boxSize常数
    // （77.5，122.5）会碰撞

    qreal boxSize = 45;
    qreal moveSpeed = 10.0;


    QPointF moveDirection(1, 0);  // 向右移动

    // 移动不会碰撞
    QPointF Pos1(0, 100);
    QVERIFY(!Collision::willCollide(Pos1, moveDirection, moveSpeed, obstacle, boxSize));

    QPointF Pos2(100, 0);
    QVERIFY(!Collision::willCollide(Pos2, moveDirection, moveSpeed, obstacle, boxSize));

    QPointF Pos3(67.5, 100);  // corner case
    QVERIFY(!Collision::willCollide(Pos3, moveDirection, moveSpeed, obstacle, boxSize));

    QPointF Pos4(122.5, 100);  // corner case
    QVERIFY(!Collision::willCollide(Pos4, moveDirection, moveSpeed, obstacle, boxSize));

    // 移动后会碰撞
    QPointF PosC1(68, 100); // corner case
    QVERIFY(Collision::willCollide(PosC1, moveDirection, moveSpeed, obstacle, boxSize));

    QPointF PosC2(100, 100);
    QVERIFY(Collision::willCollide(PosC2, moveDirection, moveSpeed, obstacle, boxSize));

    delete obstacle;
    qDebug() << "willCollide tests passed!";
}
