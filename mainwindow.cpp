#include "mainwindow.h"
#include "box.h"
#include "map.h"
#include "collision.h"
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)

    , scene(nullptr)
    , view(nullptr)
    , character(nullptr)
    , movementTimer(new QTimer(this))
    , animationTimer(new QTimer(this))
    , moveDirection(0, 0)
    , isMoving(false)
    , currentDirection(0)
    , currentFrame(0)
{
    setupScene();
    loadCharacterSpriteSheet();

    // 设置移动计时器 (30FPS)
    connect(movementTimer, &QTimer::timeout, this, &MainWindow::updateMovement);
    movementTimer->start(33); // 约30FPS

    // 设置动画计时器 (5FPS)
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::updateAnimation);
    animationTimer->start(200); // 0.2秒更新一次

    setWindowTitle("Yukari Simple Map");
    resize(1600, 900);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupScene()
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);

    // 设置场景大小
    scene->setSceneRect(0, 0, mapWidth, mapHeight);
    scene->setBackgroundBrush(Qt::darkGray);

    // 加载背景图片
    QPixmap bgPixmap(":/assets/background.jpg");
    if (!bgPixmap.isNull()) {
        QGraphicsPixmapItem *bgItem = scene->addPixmap(bgPixmap);
        bgItem->setZValue(-100);
    }

    view->scale(2.0, 2.0); // 等比例放大场景
    setCentralWidget(view);
}

void MainWindow::loadCharacterSpriteSheet()
{
    characterSpriteSheet.load(":/assets/sprites0.png");

    character = new QGraphicsPixmapItem();
    character->setPos(mapWidth/5, mapHeight/5);
    //character->setScale(1.5);
    character->setOffset(-frameWidth/2, -frameHeight/2);

    // 设置初始精灵图像
    updateCharacterSprite();

    scene->addItem(character);

    // 初始化标记（在构造函数或初始化函数中）
    // m_roleMarker = new QGraphicsEllipseItem(-3, -3, 6, 6); // 坐标小圆点
    // m_roleMarker->setBrush(Qt::green);
    // m_roleMarker->setZValue(100);
    // scene->addItem(m_roleMarker); // 添加到场景

    // 初始化箱子
    //box1 = new Box(":/assets/recipe.png", scene, character->pos());

    // 初始化地图
    gameMap = new Map(yNum, xNum, typeNum, ":/assets/recipe.png", scene, 26);
    gameMap->addToScene();


}

void MainWindow::updateCharacterSprite()
{
    if (characterSpriteSheet.isNull()) return;

    // 计算源矩形 (精灵图布局: 每行一个动画帧，每列一个方向)
    QRect sourceRect(
        currentDirection * frameWidth,  // x: 方向索引 * 帧宽度
        currentFrame * frameHeight,     // y: 帧索引 * 帧高度
        frameWidth,
        frameHeight
        );

    QPixmap framePixmap = characterSpriteSheet.copy(sourceRect);
    character->setPixmap(framePixmap);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return; // 忽略重复按键！！！重要！！！

    bool wasMoving = isMoving;

    switch (event->key()) {
    case Qt::Key_W:
        startMoving(3); // Up
        break;
    case Qt::Key_A:
        startMoving(1); // Left
        break;
    case Qt::Key_S:
        startMoving(0); // Down
        break;
    case Qt::Key_D:
        startMoving(2); // Right
        break;
    case Qt::Key_Q:
        //delete box1;
        //box1 = new Box(":/assets/recipe.png", scene, character->pos());
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }

    // 如果从静止变为移动，立即切换到第一个行走帧（此时wasMoving初始为0，isMoving在switch中变1）
    if (!wasMoving && isMoving) {
        currentFrame = 1;
        updateCharacterSprite();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;// 同keyPressEvent，忽略重复按键

    switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_A:
    case Qt::Key_S:
    case Qt::Key_D:stopMoving();break;
    default:
        QMainWindow::keyReleaseEvent(event);
        return;
    }
}

void MainWindow::startMoving(int direction)
{
    currentDirection = direction;
    isMoving = true;

    // 设置移动方向
    switch (direction) {
    case 0: moveDirection = QPointF(0, 1);  break; // Down
    case 1: moveDirection = QPointF(-1, 0); break; // Left
    case 2: moveDirection = QPointF(1, 0);  break; // Right
    case 3: moveDirection = QPointF(0, -1); break; // Up
    }
}

void MainWindow::stopMoving()
{
    isMoving = false;
    moveDirection = QPointF(0, 0);
    currentFrame = 0; // 回到静止帧
    updateCharacterSprite();
}

void MainWindow::updateMovement()
{
    if (!isMoving) return;

    QPointF newPos = character->pos() + moveDirection * moveSpeed;
    bool willCollide = false;
    //m_roleMarker->setPos(newPos);//坐标小圆点

    Box* nearestBox = nullptr;
    qreal nearestDist = std::numeric_limits<qreal>::max();

    // 遍历所有Box
    for (Box* box : gameMap->m_boxes) {
        // 碰撞检测
        if (Collision::willCollide(character->pos(), moveDirection, moveSpeed, box, box->boxSize)) {
            willCollide = true;
            handleActivation(box);   // 单独函数处理激活/消除逻辑
            break;
        }

        // Z值调整
        QPointF del = Collision::getDistance(character->pos(), box->pos());
        character->setZValue(del.y() > 0 ? 0 : 2);

        // 距离检测，找最近的
        qreal dist = Collision::EuclidDistance(del);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestBox = box;
        }
    }

    // 只让最近的 Box 进入预选中（阈值控制）
    for (Box* box : gameMap->m_boxes) {
        if (box == nearestBox && nearestDist < frameWidth * 0.75)box->preAct();
        else box->npreAct();
    }

    // 如果发生碰撞，停住
    if (willCollide) {
        moveDirection = QPointF(0, 0);
        newPos = character->pos();
    }

    // 边界循环
    qreal x = std::fmod(newPos.x(), mapWidth);
    qreal y = std::fmod(newPos.y(), mapHeight);
    if (x < 0) x += mapWidth;
    if (y < 0) y += mapHeight;
    character->setPos(x, y);
}


void MainWindow::updateAnimation()
{
    if (!isMoving) return;

    // 动画帧循环：1 -> 0 -> 2 -> 0 -> 1 -> 0 -> 2 -> ...
    static bool nextIsTwo = false; // 静态变量记录下次是否应该是帧2

    switch (currentFrame) {
    case 0:
        // 从0帧切换到1帧或2帧（交替）
        if (nextIsTwo) {
            currentFrame = 2;
            nextIsTwo = false;
        } else {
            currentFrame = 1;
            nextIsTwo = true;
        }
        break;
    case 1:
        currentFrame = 0;
        break;
    case 2:
        currentFrame = 0;
        break;
    default:
        currentFrame = 1;
        nextIsTwo = true;
        break;
    }

    updateCharacterSprite();
}

void MainWindow::handleActivation(Box* box) {
    if (!box) return;

    // 如果还没有激活过任何方块
    if (!lastActivatedBox) {
        lastActivatedBox = box;
        box->activate();
        return;
    }

    // 如果点击到同一个方块，忽略
    if (box == lastActivatedBox) return;

    // 判定是否能消除
    if (gameMap->canConnect(lastActivatedBox, box)) {
        // 消除逻辑
        lastActivatedBox->deactivate();
        box->deactivate();
        gameMap->m_map[lastActivatedBox->row][lastActivatedBox->col] = -1;
        gameMap->m_map[box->row][box->col] = -1;
        // 从 scene 删除
        gameMap->getScene()->removeItem(lastActivatedBox);
        gameMap->getScene()->removeItem(box);

        // **从 m_boxes 删除**
        gameMap->m_boxes.removeOne(lastActivatedBox);
        gameMap->m_boxes.removeOne(box);

        lastActivatedBox = nullptr; //清空状态
    } else {
        // 不满足条件，重置上一个，激活当前的
        lastActivatedBox->deactivate();
        lastActivatedBox = box;
        box->activate();
    }
}

