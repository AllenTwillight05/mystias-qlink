#include "map.h"
#include <iostream>
#include <QPainter>
#include <QTime>
#include <QTimer>

Map::Map(QWidget *parent)
    : QWidget(parent)
{
    srand((int)time(0));
    mapInit();

    // 设置窗口的标题
    setWindowTitle(tr("Map"));
    // 设置 widget 大小
    resize(500, 500);
}

Map::~Map()
{
    // 注意释放资源
    delete [] color;
}


// 绘制事件
void Map::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing,true);

    //设置视口比例，防止地图变形
    int heightSide, widthSide;
    if (((double)(width())/(double)(height())) > ((double)(wid) / (double)(heigh))) {
        heightSide = height();
        widthSide = wid * heightSide / heigh;
    } else {
        widthSide = width();
        heightSide = heigh * widthSide / wid;
    }
    painter.setViewport((width()-widthSide)/2,(height()-heightSide)/2,widthSide,heightSide);

    //设置painter的坐标，方便画图
    double widSpace, heiSpace;
    heiSpace = heigh / std::min(heigh/3, wid/3);
    widSpace = wid / std::min(heigh/3, wid/3);
    painter.setWindow(-widSpace, -heiSpace, wid+2*widSpace, heigh+2*heiSpace);

    drawMap(&painter); //画地图
}

//为地图随机生成颜色
void Map::randomColor()
{
    color = new QColor[boxNum+1];
    for (int i = 1; i <= boxNum; ++i) {
        int green = (std::rand()) % 256;
        int red = (std::rand()) % 256;
        int blue = (std::rand()) % 256;
        color[i] = QColor(red, green, blue);
    }
}

//地图的初始化
void Map::mapInit()
{
    for (int i = 0; i <= m+1; ++i)
        for (int j = 0; j <= n+1; ++j) {
            map[i][j] = 0;
        }

    // 随机填入一个类型的box
    for (int i = 1; i <= m; ++i)
        for (int j = 1; j <= n; ++j) {
            map[i][j] = ((i+j+std::rand())) % boxNum;
        }

    randomColor(); //随机颜色
}


//********************画图函数************************
void Map::drawMap(QPainter *painter) const
{
    painter->setPen(Qt::NoPen);
    for (int i = 1; i <= m; ++i)
        for (int j = 1; j <= n; ++j) {
            if (map[i][j] == 0) continue;
            if (map[i][j] < 0) {
                std::cout<< "Error! map should not contain value<0" << std::endl;
            } else { //i.e., (map[i][j] > 0)
                QLinearGradient boxGradient(i-1,j-1,i,j);
                boxGradient.setColorAt(0.0,Qt::white);
                boxGradient.setColorAt(0.8,color[map[i][j]]);
                boxGradient.setColorAt(1.0,Qt::lightGray);

                painter->setBrush(boxGradient);
                painter->drawRect(j-1,i-1,1,1);
            }

        }
}
