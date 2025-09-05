#include "map.h"
#include <QPixmap>
#include <QRandomGenerator>
#include <QDebug>

// ================= 构造 / 析构 =================

Map::Map(int rows, int cols, int typeCount,
         const QString &spriteSheetPath,
         QGraphicsScene *scene, int frameSize)
    : m_boxes(),
    m_map(),
    m_scene(scene),
    m_rows(rows),
    m_cols(cols),
    m_typeCount(typeCount),
    m_frameSize(frameSize),
    m_spriteSheetPath(spriteSheetPath),
    disOrder(nullptr)
{
    initMap();
}

Map::~Map()
{
    for (Box* b : m_boxes) {
        if (b) {
            m_scene->removeItem(b);
            delete b;
        }
    }
    delete[] disOrder;
}

// ================= 工具函数 =================

// 把格子坐标 (r,c) 转为像素中心点
QPointF Map::cellCenterPx(int r, int c) const {
    const int spacing = m_frameSize + 15;

    int totalWidth  = (m_cols - 1) * spacing;
    int totalHeight = (m_rows - 1) * spacing;

    QRectF sceneRect = m_scene->sceneRect();
    QPointF sceneCenter = sceneRect.center();

    qreal offsetX = sceneCenter.x() - totalWidth  / 2.0;
    qreal offsetY = sceneCenter.y() - totalHeight / 2.0;

    return QPointF(offsetX + c * spacing, offsetY + r * spacing);
}

QVector<QPointF> Map::cellsToScene(const QVector<QPoint>& cells) const {
    QVector<QPointF> result;
    result.reserve(cells.size());
    for (const QPoint& p : cells)
        result << cellCenterPx(p.y()-1, p.x()-1); // 注意QPoint(x,y)，这里 x=col, y=row
    return result;
}

// ================= 初始化地图 =================

void Map::initMap()
{
    disOrder = new int[m_typeCount];
    for (int i = 0; i < m_typeCount; ++i) {
        disOrder[i] = QRandomGenerator::global()->bounded(164);
    }

    m_map.resize(m_rows);
    for (int i = 0; i < m_rows; i++) {
        m_map[i].resize(m_cols);
        for (int j = 0; j < m_cols; j++) {
            m_map[i][j] = disOrder[QRandomGenerator::global()->bounded(m_typeCount)];
        }
    }
}

QPixmap Map::getSpriteByType(int typeId)
{
    QPixmap spriteSheet(m_spriteSheetPath);
    if (spriteSheet.isNull()) {
        qWarning() << "Failed to load sprite sheet:" << m_spriteSheetPath;
        return QPixmap();
    }

    const int cols = spriteSheet.width() / m_frameSize;
    int row = typeId / cols;
    int col = typeId % cols;

    QRect sourceRect(col * m_frameSize, row * m_frameSize,
                     m_frameSize, m_frameSize);
    return spriteSheet.copy(sourceRect);
}

void Map::addToScene()
{
    const int spacing = m_frameSize + 15;
    int totalWidth  = (m_cols - 1) * spacing;
    int totalHeight = (m_rows - 1) * spacing;

    QRectF sceneRect = m_scene->sceneRect();
    QPointF sceneCenter = sceneRect.center();

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
            box->row = i;
            box->col = j;
            m_boxes.append(box);
        }
    }
}

// ================= 路径判定 =================

bool Map::straightConnect(int r1, int c1, int r2, int c2,
                          const QVector<QVector<int>>& grid,
                          QVector<QPoint>& outPath)
{
    if (r1 == r2) {
        for (int c = std::min(c1, c2) + 1; c < std::max(c1, c2); ++c)
            if (grid[r1][c] != -1) return false;
        outPath << QPoint(c1, r1) << QPoint(c2, r2);
        return true;
    }
    if (c1 == c2) {
        for (int r = std::min(r1, r2) + 1; r < std::max(r1, r2); ++r)
            if (grid[r][c1] != -1) return false;
        outPath << QPoint(c1, r1) << QPoint(c2, r2);
        return true;
    }
    return false;
}

bool Map::oneTurnConnect(int r1, int c1, int r2, int c2,
                         const QVector<QVector<int>>& grid,
                         QVector<QPoint>& outPath)
{
    // 拐点1 (r1, c2)
    if (grid[r1][c2] == -1) {
        QVector<QPoint> tmp;
        if (straightConnect(r1, c1, r1, c2, grid, tmp) &&
            straightConnect(r1, c2, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << QPoint(c2, r1) << QPoint(c2, r2);
            return true;
        }
    }
    // 拐点2 (r2, c1)
    if (grid[r2][c1] == -1) {
        QVector<QPoint> tmp;
        if (straightConnect(r1, c1, r2, c1, grid, tmp) &&
            straightConnect(r2, c1, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << QPoint(c1, r2) << QPoint(c2, r2);
            return true;
        }
    }
    return false;
}

bool Map::twoTurnConnect(int r1, int c1, int r2, int c2,
                         const QVector<QVector<int>>& grid,
                         int rows, int cols,
                         QVector<QPoint>& outPath)
{
    // 从起点沿四个方向延伸
    for (int c = c1 - 1; c >= 0; --c) {
        if (grid[r1][c] != -1) break;
        QVector<QPoint> tmp;
        if (oneTurnConnect(r1, c, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << tmp;
            return true;
        }
    }
    for (int c = c1 + 1; c < cols; ++c) {
        if (grid[r1][c] != -1) break;
        QVector<QPoint> tmp;
        if (oneTurnConnect(r1, c, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << tmp;
            return true;
        }
    }
    for (int r = r1 - 1; r >= 0; --r) {
        if (grid[r][c1] != -1) break;
        QVector<QPoint> tmp;
        if (oneTurnConnect(r, c1, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << tmp;
            return true;
        }
    }
    for (int r = r1 + 1; r < rows; ++r) {
        if (grid[r][c1] != -1) break;
        QVector<QPoint> tmp;
        if (oneTurnConnect(r, c1, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << tmp;
            return true;
        }
    }
    return false;
}

bool Map::canConnect(Box* a, Box* b)
{
    if (!a || !b) return false;
    if (a == b) return false;
    if (m_map[a->row][a->col] != m_map[b->row][b->col]) return false;

    int rows = m_rows;
    int cols = m_cols;

    QVector<QVector<int>> grid(rows + 2, QVector<int>(cols + 2, -1));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            grid[i+1][j+1] = m_map[i][j];

    int r1 = a->row + 1, c1 = a->col + 1;
    int r2 = b->row + 1, c2 = b->col + 1;

    grid[r1][c1] = -1;
    grid[r2][c2] = -1;

    QVector<QPoint> path;

    if (straightConnect(r1, c1, r2, c2, grid, path) ||
        oneTurnConnect(r1, c1, r2, c2, grid, path) ||
        twoTurnConnect(r1, c1, r2, c2, grid, rows+2, cols+2, path)) {
        m_pathCells = path;
        m_pathPixels = cellsToScene(path);
        return true;
    }

    return false;
}
