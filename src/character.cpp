#include "character.h"
#include "box.h"
#include "map.h"
#include "collision.h"   // 这里需要用到 Collision::willCollide 等函数

#include <cmath>
#include <limits>

Character::Character(const QString& spritePath, QObject* parent)
    : QObject(parent), QGraphicsPixmapItem(),
    isMoving(false), currentDirection(0), currentFrame(0)
{
    spriteSheet.load(spritePath);
    setOffset(-frameWidth/2, -frameHeight/2);

    updateCharacterSprite();

    // 移动计时器 (30 FPS)
    movementTimer = new QTimer(this);
    connect(movementTimer, &QTimer::timeout, this, &Character::updateMovement);
    movementTimer->start(33);

    // 动画计时器 (5 FPS)
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &Character::updateAnimation);
    animationTimer->start(200);
}

void Character::updateCharacterSprite() {
    if (spriteSheet.isNull()) return;

    QRect sourceRect(
        currentDirection * frameWidth,
        currentFrame * frameHeight,
        frameWidth, frameHeight);

    setPixmap(spriteSheet.copy(sourceRect));
}

void Character::startMoving(int direction) {
    currentDirection = direction;
    isMoving = true;

    switch (direction) {
    case 0: moveDirection = QPointF(0, 1);  break; // Down
    case 1: moveDirection = QPointF(-1, 0); break; // Left
    case 2: moveDirection = QPointF(1, 0);  break; // Right
    case 3: moveDirection = QPointF(0, -1); break; // Up
    }
}

void Character::stopMoving() {
    isMoving = false;
    moveDirection = QPointF(0, 0);
    currentFrame = 0;
    updateCharacterSprite();
}

void Character::updateMovement() {
    if (!isMoving) return;

    QPointF newPos = pos() + moveDirection * moveSpeed;
    bool willCollide = false;
    Box* nearestBox = nullptr;
    qreal nearestDist = std::numeric_limits<qreal>::max();

    if (gameMap) {
        // 遍历所有 Box
        for (Box* box : gameMap->m_boxes) {
            // 碰撞检测
            if (Collision::willCollide(pos(), moveDirection, moveSpeed, box, box->boxSize)) {
                willCollide = true;
                emit collidedWithBox(box);   // 通知 MainWindow
                break;
            }

            // Z 值调整
            QPointF del = Collision::getDistance(pos(), box->pos());
            setZValue(del.y() > 0 ? 0 : 2);

            // 距离检测，找最近的
            qreal dist = Collision::EuclidDistance(del);
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestBox = box;
            }
        }

        // 最近 Box 高亮
        for (Box* box : gameMap->m_boxes) {
            if (box == nearestBox && nearestDist < frameWidth * 0.75)
                box->preAct();
            else
                box->npreAct();
        }
    }

    // 碰撞则停住
    if (willCollide) {
        moveDirection = QPointF(0, 0);
        newPos = pos();
    }

    // 边界循环（写死 800x600，可改成参数传入）
    qreal x = std::fmod(newPos.x(), 800.0);
    qreal y = std::fmod(newPos.y(), 600.0);
    if (x < 0) x += 800;
    if (y < 0) y += 600;
    setPos(x, y);
}

void Character::updateAnimation() {
    if (!isMoving) return;

    static bool nextIsTwo = false;
    switch (currentFrame) {
    case 0:
        currentFrame = nextIsTwo ? 2 : 1;
        nextIsTwo = !nextIsTwo;
        break;
    case 1: case 2:
        currentFrame = 0;
        break;
    default:
        currentFrame = 1;
        nextIsTwo = true;
        break;
    }
    updateCharacterSprite();
}

void Character::handleKeyPress(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;

    if (event->key() == controls.upKey) startMoving(3);
    else if (event->key() == controls.leftKey) startMoving(1);
    else if (event->key() == controls.downKey) startMoving(0);
    else if (event->key() == controls.rightKey) startMoving(2);

    if (isMoving && currentFrame == 0) {
        currentFrame = 1;
        updateCharacterSprite();
    }
}

void Character::handleKeyRelease(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;

    if (event->key() == controls.upKey ||
        event->key() == controls.leftKey ||
        event->key() == controls.downKey ||
        event->key() == controls.rightKey) {
        stopMoving();
    }
}

void Character::stopTimers() {
    if (movementTimer) movementTimer->stop();
    if (animationTimer) animationTimer->stop();
}
