#pragma once

#include <QObject>
#include <QTimer>
#include <QPair>

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

    QPixmap getPowerUpSprite(int powerUpType);

    // 生成道具：输入道具类型，在随机位置生成对应box
    void spawnPowerUp(int powerUpType);

    // Hint相关方法
    void activateHint();
    void deactivateHint();

private slots:
    void onHintTimeout();
    void updateHintPair();

private:
    Map* gameMap = nullptr;
    QGraphicsScene* gameScene = nullptr;

    // Hint相关成员变量
    QTimer* hintTimer = nullptr;
    QTimer* hintUpdateTimer = nullptr;
    QPair<Box*, Box*> currentHintPair;
    bool isHintActive = false;

    // 道具精灵图相关
    QString powerUpSpriteSheetPath = ":/assets/powerups.png";
    const int powerUpFrameSize = 32;

    // 获取一对可连接的方块
    QPair<Box*, Box*> getHintPair();
};
