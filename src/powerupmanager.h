#pragma once

#include <QObject>

class Map;
class Box;
class MainWindow;
class QGraphicsScene;

class PowerUpManager : public QObject
{
    Q_OBJECT
public:
    explicit PowerUpManager(QObject* parent = nullptr);

    // 初始化，设置地图和场景
    void initialize(Map* map, QGraphicsScene* scene);

    // 生成道具：输入道具类型，在随机位置生成对应box
    void spawnPowerUp(int powerUpType);

private:
    Map* gameMap = nullptr;
    QGraphicsScene* gameScene = nullptr;
};
