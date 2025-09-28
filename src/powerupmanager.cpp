#include "powerupmanager.h"
#include "map.h"
#include "box.h"
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QTimer>

PowerUpManager::PowerUpManager(QObject* parent)
    : QObject(parent)
{
}

void PowerUpManager::initialize(Map* map, QGraphicsScene* scene)
{
    gameMap = map;
    gameScene = scene;
}

void PowerUpManager::spawnPowerUp(int powerUpType)
{
    if (!gameMap || !gameScene) return;

    //padding矩阵初始化为全-1
    int rows = gameMap->getRowCount();
    int cols = gameMap->getColCount();
    QVector<QVector<int>> grid(rows + 2, QVector<int>(cols + 2, -1));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            grid[i+1][j+1] = gameMap->m_map[i][j];

    // 获取所有空位置
    QVector<QPoint> emptyPositions;
    for (int row = 0; row < rows + 2; row++) {
        for (int col = 0; col < cols + 2; col++) {
            if (grid[row][col] == -1) {
                emptyPositions.append(QPoint(col-1, row-1));
            }
        }
    }

    if (emptyPositions.isEmpty()) return;

    // 随机选择一个空位置
    int index = QRandomGenerator::global()->bounded(emptyPositions.size());
    QPoint pos = emptyPositions[index];
    int c = pos.x();
    int r = pos.y();

    // 根据道具类型选择贴图
    QString imagePath;
    switch (powerUpType) {
    case 1: // +1s道具
        imagePath = ":/assets/wellington.png";
        break;
    // 可以添加其他道具类型
    default:
        return; // 未知类型不生成
    }

    // 创建道具盒子
    Box* powerUpBox = new Box(gameMap->cellCenterPx(r,c), imagePath, gameScene);
    powerUpBox->toolType = powerUpType;  // 设置道具类型标识
    gameMap->m_tools.append(powerUpBox);

    // 设置10秒后自动消失
    QTimer::singleShot(10000, [powerUpBox]() {
        if (powerUpBox && powerUpBox->scene()) {
            powerUpBox->scene()->removeItem(powerUpBox);
            delete powerUpBox;
        }
    });
}
