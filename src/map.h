#pragma once

#include <QWidget>

#define MAX_MAP_SIZE 60

class Map : public QWidget
{
    Q_OBJECT

public:
    Map(QWidget *parent = nullptr);
    virtual ~Map();

protected:
    void paintEvent(QPaintEvent *event) override;
    void drawMap(QPainter *painter) const;
    void randomColor();
    void mapInit();
private:
    int n=15, m=15; // 表示Map的大小，需要能够接收用户的输入动态配置
    int boxNum=7;
    int map[MAX_MAP_SIZE][MAX_MAP_SIZE];
    int heigh = 20, wid = 20;

    QColor *color;
};
