#include "powerupmanager.h"
#include "map.h"
#include "box.h"
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QTimer>
#include <QPixmap>

// 道具管理器类构造函数，传入父类
PowerUpManager::PowerUpManager(QObject* parent)
    : QObject(parent)
{
    // 初始化Hint相关计时器
    // hint效果倒计时
    hintTimer = new QTimer(this);
    hintTimer->setSingleShot(true);
    connect(hintTimer, &QTimer::timeout, this, &PowerUpManager::onHintTimeout);

    // 刷新Hint对
    hintUpdateTimer = new QTimer(this);
    connect(hintUpdateTimer, &QTimer::timeout, this, &PowerUpManager::updateHintPair);

    // 闪烁定时器
    hintBlinkTimer = new QTimer(this);
    connect(hintBlinkTimer, &QTimer::timeout, this, &PowerUpManager::toggleHintBlink);
}

// 析构函数，取消hint的10s效果，其余析构交给父类mainWindow对象
PowerUpManager::~PowerUpManager()
{
    deactivateHint();
}

// 初始化道具管理器类，传入map对象指针和场景scene指针（
void PowerUpManager::initialize(Map* map, QGraphicsScene* scene)
{
    gameMap = map;
    gameScene = scene;
}

// 从道具精灵图中裁切帧，传入道具编号
QPixmap PowerUpManager::getPowerUpSprite(int powerUpType)
{
    QPixmap spriteSheet(powerUpSpriteSheetPath);
    if (spriteSheet.isNull()) {
        qWarning() << "Failed to load powerup sprite sheet:" << powerUpSpriteSheetPath;
        return QPixmap();
    }

    // 道具类型1-3对应精灵图的第0-2个位置
    int frameIndex = powerUpType - 1;
    if (frameIndex < 0 || frameIndex >= 3) {
        qWarning() << "Invalid powerup type:" << powerUpType;
        return QPixmap();
    }

    QRect sourceRect(frameIndex * powerUpFrameSize, 0,
                     powerUpFrameSize, powerUpFrameSize);
    return spriteSheet.copy(sourceRect);
}

// 道具生成和10s后自动消除函数。传入道具编号，遍历得到空位个数，并随机选择空格插入对应序号道具box
void PowerUpManager::spawnPowerUp(int powerUpType)
{
    if (!gameMap || !gameScene) return;

    // 获取所有空位置
    int rows = gameMap->getRowCount();
    int cols = gameMap->getColCount();

    QVector<QPoint> emptyPositions;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // 检查该位置是否为空（-1）且没有被道具占据
            if (gameMap->m_map[i][j] == -1) {
                bool occupiedByTool = false;
                for (Box* tool : gameMap->m_tools) {
                    if (tool->row == i && tool->col == j) {
                        //tool属于box类指针，有row和col成员
                        occupiedByTool = true;
                        break;
                    }
                }
                if (!occupiedByTool) {
                    emptyPositions.append(QPoint(j, i));
                }
            }
        }
    }

    if (emptyPositions.isEmpty()) return;

    // 随机选择一个空位置
    int index = QRandomGenerator::global()->bounded(emptyPositions.size());
    QPoint pos = emptyPositions[index];
    int c = pos.x();
    int r = pos.y();

    // 使用精灵图创建道具
    QPixmap powerUpSprite = getPowerUpSprite(powerUpType);
    if (powerUpSprite.isNull()) {
        qWarning() << "Failed to create sprite for powerup type:" << powerUpType;
        return;
    }

    // 创建道具盒子
    Box* powerUpBox = new Box(gameMap->cellCenterPx(r, c), "", gameScene);  // Box初始化的图片传入是QString类型地址，无法直接传QPixmap powerUpSprite
    powerUpBox->setPixmap(powerUpSprite);
    powerUpBox->setOffset(-powerUpSprite.width()/2, -powerUpSprite.height()/2);
    powerUpBox->toolType = powerUpType;  // 设置道具类型标识
    powerUpBox->row = r;
    powerUpBox->col = c;
    gameMap->m_tools.append(powerUpBox);

    // 设置10秒后自动消失
    QTimer::singleShot(10000, [this, powerUpBox]() {
        if (!powerUpBox) return;

        if (gameMap && gameMap->m_tools.contains(powerUpBox)) {
            gameMap->m_tools.removeOne(powerUpBox);
            if (powerUpBox->scene()) {
                powerUpBox->scene()->removeItem(powerUpBox);
            }
            delete powerUpBox;
        }
    });
}

// 获取一对可连接的方块
QPair<Box*, Box*> PowerUpManager::getHintPair()
{
    if (!gameMap) return qMakePair(nullptr, nullptr);

    // 使用类似isSolvable的逻辑，但返回第一对可连接的方块
    QMap<int, QVector<Box*>> typeGroups;

    for (Box* box : gameMap->m_boxes) {
        if (!box) continue;
        int r = box->row;
        int c = box->col;
        if (r < 0 || r >= gameMap->getRowCount() || c < 0 || c >= gameMap->getColCount()) continue;
        int type = gameMap->m_map[r][c];
        if (type != -1) {
            typeGroups[type].append(box);
        }
    }

    for (auto it = typeGroups.begin(); it != typeGroups.end(); ++it) {
        const QVector<Box*>& group = it.value();
        int n = group.size();
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                Box* a = group[i];
                Box* b = group[j];
                if (!a || !b) continue;
                if (gameMap->canConnect(a, b)) {
                    return qMakePair(a, b);
                }
            }
        }
    }

    return qMakePair(nullptr, nullptr);
}

// 激活Hint效果
void PowerUpManager::activateHint()
{
    if (isHintActive) return;

    isHintActive = true;
    isHintBlinking = true;  // 开始闪烁
    blinkCount = 0;         // 重置闪烁计数

    // 开始10秒倒计时
    hintTimer->start(10000);

    // 每0.5秒检查一次是否需要更新Hint对（当当前对已被消除时）
    hintUpdateTimer->start(500);

    // 开始闪烁（0.5s间隔）
    hintBlinkTimer->start(500);

    // 显示第一对Hint
    updateHintPair();

    qDebug() << "Hint activated for 10 seconds";
}

// 取消Hint效果
void PowerUpManager::deactivateHint()
{
    isHintActive = false;
    hintTimer->stop();
    hintUpdateTimer->stop();
    hintBlinkTimer->stop();

    // 取消当前高亮
    if (currentHintPair.first && currentHintPair.second) {
        currentHintPair.first->deactivate();
        currentHintPair.second->deactivate();
        currentHintPair.first = nullptr;
        currentHintPair.second = nullptr;
    }

    qDebug() << "Hint deactivated";
}

// Hint时间结束
void PowerUpManager::onHintTimeout()
{
    deactivateHint();
}

// 闪烁切换函数
void PowerUpManager::toggleHintBlink()
{
    if (!isHintActive || !isHintBlinking) return;

    // 如果当前Hint对不存在或无效，尝试更新
    if (!currentHintPair.first || !currentHintPair.second ||
        !currentHintPair.first->scene() || !currentHintPair.second->scene()) {
        updateHintPair();
        return;
    }

    blinkCount++;

    // 切换激活/取消激活状态
    if (blinkCount % 2 == 1) {
        // 奇数次闪烁：激活状态（黄色）
        currentHintPair.first->activate();
        currentHintPair.second->activate();
    } else {
        // 偶数次闪烁：取消激活状态
        currentHintPair.first->deactivate();
        currentHintPair.second->deactivate();
    }

    // 限制最大闪烁次数，避免无限闪烁
    // if (blinkCount >= 6) { // 3秒后停止闪烁（* 500ms）
    //     isHintBlinking = false;
    //     hintBlinkTimer->stop();
    // }
}

// 更新提示box
void PowerUpManager::updateHintPair()
{
    if (!isHintActive) return;

    // 如果当前Hint对仍然有效且存在，保持（闪烁会处理状态切换）
    if (currentHintPair.first && currentHintPair.second &&
        currentHintPair.first->scene() && currentHintPair.second->scene() &&
        gameMap->canConnect(currentHintPair.first, currentHintPair.second)) {
        return;
    }

    // 取消之前的高亮
    if (currentHintPair.first && currentHintPair.second) {
        currentHintPair.first->deactivate();
        currentHintPair.second->deactivate();
    }

    // 获取新的Hint对
    currentHintPair = getHintPair();

    if (currentHintPair.first && currentHintPair.second) {
        // 重置闪烁状态，确保新的一对从激活状态开始闪烁
        blinkCount = 0;
        // 立即激活新的一对（闪烁定时器会在500ms后切换状态）
        currentHintPair.first->activate();
        currentHintPair.second->activate();
    } else {
        qDebug() << "No connectable pair found for hint";
    }
}
