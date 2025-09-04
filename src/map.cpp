#include "map.h"
#include <QPixmap>
#include <QRandomGenerator>
#include <QDebug>

Map::Map(int rows, int cols, int typeCount,
         const QString &spriteSheetPath,
         QGraphicsScene *scene, int frameSize)
    : m_rows(rows), m_cols(cols),
    m_typeCount(typeCount),
    m_frameSize(frameSize),
    m_spriteSheetPath(spriteSheetPath),
    m_scene(scene)
{
    initMap(); // 随机生成地图矩阵
}

Map::~Map()
{
    // 清理 Box 对象，避免内存泄漏
    for (Box* b : m_boxes) {
        if (b) {
            m_scene->removeItem(b);
            delete b;
        }
    }
    delete[] disOrder;
}

void Map::initMap()
{
    disOrder = new int[m_typeCount];
    for(int i = 0; i < m_typeCount; ++i){
        disOrder[i] = QRandomGenerator::global()->bounded(164);
    }
    m_map.resize(m_rows);
    for (int i = 0; i < m_rows; i++) {
        m_map[i].resize(m_cols);
        for (int j = 0; j < m_cols; j++) {
            // 随机分配一个类型 [0, m_typeCount-1]
            m_map[i][j] = disOrder[QRandomGenerator::global()->bounded(m_typeCount)];
        }
    }
}

QPixmap Map::getSpriteByType(int typeId)
{
    QPixmap spriteSheet(m_spriteSheetPath); //加载精灵图
    if (spriteSheet.isNull()) {
        qWarning() << "Failed to load sprite sheet:" << m_spriteSheetPath;
        return QPixmap();
    }

    const int cols = spriteSheet.width() / m_frameSize;
    int row = typeId / cols;
    int col = typeId % cols;

    QRect sourceRect(col * m_frameSize, row * m_frameSize, m_frameSize, m_frameSize);
    return spriteSheet.copy(sourceRect);
}

void Map::addToScene()
{
    const int spacing = m_frameSize + 15; // 格子间隔

    // 整个地图像素宽高
    int totalWidth  = (m_cols - 1) * spacing;
    int totalHeight = (m_rows - 1) * spacing;

    // 场景中心
    QRectF sceneRect = m_scene->sceneRect();
    QPointF sceneCenter = sceneRect.center();

    // 偏移量 = 场景中心 - 一半地图尺寸
    qreal offsetX = sceneCenter.x() - totalWidth  / 2.0;
    qreal offsetY = sceneCenter.y() - totalHeight / 2.0;

    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            int typeId = m_map[i][j];
            QPixmap sprite = getSpriteByType(typeId);
            if (sprite.isNull()) continue;

            QPointF pos(offsetX + j * spacing,
                        offsetY + i * spacing);

            Box *box = new Box(pos, m_spriteSheetPath, m_scene);
            box->setPixmap(sprite);
            box->setZValue(1);
            //保存box坐标
            box->row = i;
            box->col = j;
            m_boxes.append(box);
        }
    }
}

//canConnect()辅助函数
static bool straightConnect(int r1, int c1, int r2, int c2,
                            const QVector<QVector<int>>& grid) {
    if (r1 == r2) {
        for (int c = std::min(c1, c2) + 1; c < std::max(c1, c2); ++c) {
            if (grid[r1][c] != -1) return false;
        }
        return true;
    }
    if (c1 == c2) {
        for (int r = std::min(r1, r2) + 1; r < std::max(r1, r2); ++r) {
            if (grid[r][c1] != -1) return false;
        }
        return true;
    }
    return false;
}

static bool oneTurnConnect(int r1, int c1, int r2, int c2,
                           const QVector<QVector<int>>& grid) {
    // 拐点1 (r1, c2)
    if (grid[r1][c2] == -1 &&
        straightConnect(r1, c1, r1, c2, grid) &&
        straightConnect(r1, c2, r2, c2, grid))
        return true;

    // 拐点2 (r2, c1)
    if (grid[r2][c1] == -1 &&
        straightConnect(r1, c1, r2, c1, grid) &&
        straightConnect(r2, c1, r2, c2, grid))
        return true;

    return false;
}

static bool twoTurnConnect(int r1, int c1, int r2, int c2,
                           const QVector<QVector<int>>& grid,
                           int rows, int cols) {
    // 从起点沿四个方向延伸
    // 横向
    for (int c = c1 - 1; c >= 0; --c) {
        if (grid[r1][c] != -1) break; // 遇到障碍停
        if (oneTurnConnect(r1, c, r2, c2, grid)) return true;
    }
    for (int c = c1 + 1; c < cols; ++c) {
        if (grid[r1][c] != -1) break;
        if (oneTurnConnect(r1, c, r2, c2, grid)) return true;
    }
    // 纵向
    for (int r = r1 - 1; r >= 0; --r) {
        if (grid[r][c1] != -1) break;
        if (oneTurnConnect(r, c1, r2, c2, grid)) return true;
    }
    for (int r = r1 + 1; r < rows; ++r) {
        if (grid[r][c1] != -1) break;
        if (oneTurnConnect(r, c1, r2, c2, grid)) return true;
    }
    return false;
}

bool Map::canConnect(Box* a, Box* b) {
    if (!a || !b) return false;
    if (a == b) return false;
    if (m_map[a->row][a->col] != m_map[b->row][b->col]) return false;

    int rows = m_rows;
    int cols = m_cols;

    // 构造 padding 后的 grid
    QVector<QVector<int>> grid(rows + 2, QVector<int>(cols + 2, -1));   //初始化语法：grid(元素个数,元素值）
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            grid[i+1][j+1] = m_map[i][j];
        }
    }

    // 起点和终点的坐标需要 +1 偏移
    int r1 = a->row + 1, c1 = a->col + 1;
    int r2 = b->row + 1, c2 = b->col + 1;

    // 起点终点设为空，避免自己挡路
    grid[r1][c1] = -1;
    grid[r2][c2] = -1;

    // 0拐
    if (straightConnect(r1, c1, r2, c2, grid)) return true;
    // 1拐
    if (oneTurnConnect(r1, c1, r2, c2, grid)) return true;
    // 2拐
    if (twoTurnConnect(r1, c1, r2, c2, grid, rows+2, cols+2)) return true;

    return false;
}
