#pragma once

#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QPointF>
#include <QPixmap>
#include <QKeyEvent>

// 前向声明，避免循环包含
class Box;
class Map;
class Score;

// 键位配置结构体
struct ControlScheme {
    int upKey;  //Qt::Key_W等按键常量本质上是int型枚举值enum value
    int downKey;
    int leftKey;
    int rightKey;
};

class Character : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    Character(const QString& spritePath, QObject* parent = nullptr);

    void setControls(const ControlScheme& scheme) { controls = scheme; }
    void setGameMap(Map* map) { gameMap = map; }
    Score* getCharacterScore() const { return characterScore; }

    void handleKeyPress(QKeyEvent* event);
    void handleKeyRelease(QKeyEvent* event);
    void stopTimers();
    bool isPaused;

signals:
    void collidedWithBox(Box* box, Character* sender);  // sender参数,碰撞时发射信号，交给 MainWindow 处理

private slots:
    void updateMovement();
    void updateAnimation();

private:
    void updateCharacterSprite();
    void startMoving(int direction);
    void stopMoving();

    // 控制配置
    ControlScheme controls;

    // 移动相关
    QPointF moveDirection;
    bool isMoving;
    const qreal moveSpeed = 8.0;

    // 动画相关
    int currentDirection;    // 0=Down, 1=Left, 2=Right, 3=Up
    int currentFrame;        // 0=静止, 1=左脚, 2=右脚
    const int frameWidth = 64;
    const int frameHeight = 64;
    QPixmap spriteSheet;

    // 计时器
    QTimer* movementTimer;
    QTimer* animationTimer;

    // 地图引用（用于碰撞检测）
    Map* gameMap = nullptr;

    // 分数对象指针
    Score* characterScore;

    // debug用坐标小圆点
    bool debugMarkerEnabled = false;
    QGraphicsEllipseItem* roleMarker = nullptr;
};
