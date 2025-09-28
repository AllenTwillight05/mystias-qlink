#include "powerupmanager.h"
#include "map.h"
#include "box.h"
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QTimer>
#include <QPixmap>

PowerUpManager::PowerUpManager(QObject* parent)
    : QObject(parent)
{
    // 初始化Hint相关计时器
    hintTimer = new QTimer(this);
    hintTimer->setSingleShot(true);
    connect(hintTimer, &QTimer::timeout, this, &PowerUpManager::onHintTimeout);

    hintUpdateTimer = new QTimer(this);
    connect(hintUpdateTimer, &QTimer::timeout, this, &PowerUpManager::updateHintPair);
}

PowerUpManager::~PowerUpManager()
{
    deactivateHint();
}

void PowerUpManager::initialize(Map* map, QGraphicsScene* scene)
{
    gameMap = map;
    gameScene = scene;
}

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

    // 根据道具类型选择贴图
    // QString imagePath;
    // switch (powerUpType) {
    // case 1: // +1s道具
    //     imagePath = ":/assets/time.png";
    //     break;
    // case 2: // shuffle
    //     imagePath = ":/assets/wellington.png";
    //     break;
    // case 3: // hint道具
    //     imagePath = ":/assets/hint.png"; // 你需要准备一个Hint道具的图片
    //     break;
    // default:
    //     return; // 未知类型不生成
    // }

    // 创建道具盒子
    Box* powerUpBox = new Box(gameMap->cellCenterPx(r, c), "", gameScene);
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

    // 开始10秒倒计时
    hintTimer->start(10000);

    // 每0.5秒检查一次是否需要更新Hint对（当当前对已被消除时）
    hintUpdateTimer->start(500);

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

// 更新Hint对
void PowerUpManager::updateHintPair()
{
    if (!isHintActive) return;

    // 如果当前Hint对仍然有效且存在，保持高亮
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
        // 高亮显示这对方块
        currentHintPair.first->activate();
        currentHintPair.second->activate();

        // 可以添加特殊的高亮效果，比如不同的颜色
        // 这里使用默认的activate效果，你也可以自定义
    } else {
        qDebug() << "No connectable pair found for hint";
    }
}
