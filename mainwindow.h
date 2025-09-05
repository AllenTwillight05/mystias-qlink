#pragma once

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QKeyEvent>
#include <QTimer>
#include <QPixmap>


class Box;
class Map;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updateMovement();
    void updateAnimation();

private:
    void setupScene();
    void loadCharacterSpriteSheet();
    void updateCharacterSprite();
    void startMoving(int direction);
    void stopMoving();

private:
    // 图形相关
    QGraphicsScene *scene;
    QGraphicsView *view;
    QGraphicsPixmapItem *character;
    Box* box1;
    QPixmap characterSpriteSheet;

    QGraphicsEllipseItem* m_roleMarker; // 角色坐标标记

    // 计时器
    QTimer *movementTimer;  // 移动计时器 (30FPS)
    QTimer *animationTimer; // 动画计时器 (5FPS)

    // 移动相关
    QPointF moveDirection;
    bool isMoving;
    const qreal moveSpeed = 8.0;

    // 动画相关
    int currentDirection;    // 0=Down, 1=Left, 2=Right, 3=Up
    int currentFrame;        // 0=静止, 1=左脚, 2=右脚
    const int frameWidth = 64;
    const int frameHeight = 64;

    // 地图相关
    const qreal mapWidth = 800;
    const qreal mapHeight = 600;
    int yNum = 6, xNum = 8, typeNum = 6;
    Map* gameMap;  // 持有Map实例指针

    // 消除相关
    Box* lastActivatedBox = nullptr;
    void handleActivation(Box* box);
    QGraphicsPathItem* currentPathItem = nullptr;

    // 按键状态
    //bool keys[4]; // W, A, S, D多按键控制
};

