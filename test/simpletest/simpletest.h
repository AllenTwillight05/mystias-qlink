#pragma once

#include <QtTest>
#include <QObject>
#include "collision.h"

class SimpleTest : public QObject
{
    Q_OBJECT

private slots:
    // 测试函数声明
    void testEuclidDistance();
    void testGetDistance();
    void testPointCollision();
    //void testCheckCollision();//弃用
    void testWillCollide();
};
