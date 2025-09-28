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
    m_scene(scene), //这里mainwindow中传入box
    m_tools(),
    m_rows(rows),
    m_cols(cols),
    m_typeCount(typeCount),
    m_frameSize(frameSize),
    m_spriteSheetPath(spriteSheetPath),
    disOrder(nullptr)
{
    initMap();
}

// Map::~Map()
// {
//     for (Box* b : m_boxes) {
//         if (b) {
//             m_scene->removeItem(b);
//             delete b;
//         }
//     }
//     delete[] disOrder;
// }

Map::~Map()
{
    // 这里不要手动 delete b，让 scene->clear() 来做
    m_boxes.clear();   // 只是清空容器，不销毁 Box
    m_tools.clear();
    delete[] disOrder;
}

// ================= 工具函数 =================

//  把格子坐标 (r,c) 转为像素中心点
QPointF Map::cellCenterPx(int r, int c) const {

    int totalWidth  = (m_cols - 1) * spacing;
    int totalHeight = (m_rows - 1) * spacing;

    QRectF sceneRect = m_scene->sceneRect();
    QPointF sceneCenter = sceneRect.center();

    qreal offsetX = sceneCenter.x() - totalWidth  / 2.0;
    qreal offsetY = sceneCenter.y() - totalHeight / 2.0;

    return QPointF(offsetX + c * spacing, offsetY + r * spacing);
}
//  网格转化为像素坐标
QVector<QPointF> Map::cellsToScene(const QVector<QPoint>& cells) const {
    QVector<QPointF> result;
    result.reserve(cells.size());   //预先分配内存，避免频繁的重新分配和拷贝
    for (const QPoint& p : cells)
        result << cellCenterPx(p.y()-1, p.x()-1); // 注意QPoint(x,y)，这里 x=col, y=row，-1是为了从padding后的grid回到原map
    return result;
}

// ================= 初始化地图 =================

void Map::initMap()
{
    disOrder = new int[m_typeCount + 1];
    for (int i = 0; i < m_typeCount; ++i) {
        //disOrder[i] = QRandomGenerator::global()->bounded(164);    // 精灵图参数：recipe
        disOrder[i] = QRandomGenerator::global()->bounded(62);    // 精灵图参数：ingridient
    }
    disOrder[m_typeCount] = -1;

    m_map.resize(m_rows);
    for (int i = 0; i < m_rows; i++) {
        m_map[i].resize(m_cols);
        for (int j = 0; j < m_cols; j++) {
            int randomIndex = QRandomGenerator::global()->bounded(m_typeCount + 1);
            m_map[i][j] = disOrder[randomIndex];
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
    int totalWidth  = (m_cols - 1) * spacing;
    int totalHeight = (m_rows - 1) * spacing;

    QRectF sceneRect = m_scene->sceneRect();
    QPointF sceneCenter = sceneRect.center();

    qreal offsetX = sceneCenter.x() - totalWidth  / 2.0;
    qreal offsetY = sceneCenter.y() - totalHeight / 2.0;

    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            int typeId = m_map[i][j];
            if(typeId == -1) continue;
            QPixmap sprite = getSpriteByType(typeId);
            if(sprite.isNull()) continue;

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

// 读档设置地图数据
void Map::setMapData(const QVector<QVector<int>>& newMapData)
{
    // 首先清理现有的箱子对象，避免内存泄漏
    qDeleteAll(m_boxes);
    m_boxes.clear();
    qDeleteAll(m_tools);
    m_tools.clear();

    // 复制新的地图数据
    m_map = newMapData;
    if (m_map.isEmpty() || m_map[0].isEmpty()) {
        qWarning() << "地图数据未初始化，无法创建箱子";    // 确保地图数据已初始化
        return;
    }

    // 根据新的地图数据重新初始化箱子
    addToScene();
}


// ================= 路径判定 =================

bool Map::straightConnect(int r1, int c1, int r2, int c2,
                          const QVector<QVector<int>>& grid,
                          QVector<QPoint>& outPath) const
{
    if (r1 == r2) {
        for (int c = std::min(c1, c2) + 1; c < std::max(c1, c2); ++c)
            if (grid[r1][c] != -1) return false;
        outPath << QPoint(c1, r1) << QPoint(c2, r2);    //等价于outPath.append()
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
                         QVector<QPoint>& outPath) const
{
    // 拐点1 (r1, c2)
    if (grid[r1][c2] == -1) {
        QVector<QPoint> tmp;    //tmp没用，仅仅作为straightConnect()所需传入；函数执行完毕离开作用域时，会自动调用析构函数释放QVector对象内存
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
                         QVector<QPoint>& outPath) const
{
    // 从起点沿四个方向延伸
    for (int c = c1 - 1; c >= 0; --c) { //向左
        if (grid[r1][c] != -1) break;
        QVector<QPoint> tmp;    //此处tmp有用，因为第一个拐点已经记录并作为起点传入，故而tmp记录第一、二个拐点和终点
        if (oneTurnConnect(r1, c, r2, c2, grid, tmp)) {
            outPath << QPoint(c1, r1) << tmp;   //先传入起点，随后传入tmp
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
    for (int r = r1 - 1; r >= 0; --r) { //向上
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

    //padding矩阵初始化为全-1
    QVector<QVector<int>> grid(rows + 2, QVector<int>(cols + 2, -1));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            grid[i+1][j+1] = m_map[i][j];

    //在grid中坐标a(c1,r1),b(c2,r2) （对应(x,y)但不是实际坐标，是格子序号）
    int r1 = a->row + 1, c1 = a->col + 1;
    int r2 = b->row + 1, c2 = b->col + 1;

    grid[r1][c1] = -1;
    grid[r2][c2] = -1;

    QVector<QPoint> path;

    if (straightConnect(r1, c1, r2, c2, grid, path) ||  //此时传入path为空
        oneTurnConnect(r1, c1, r2, c2, grid, path) ||
        twoTurnConnect(r1, c1, r2, c2, grid, rows+2, cols+2, path)) {
        m_pathCells = path;
        m_pathPixels = cellsToScene(path);
        return true;
    }

    return false;
}


// 直接遍历 m_boxes，按 type 聚合 Box*存储在红黑树typeGroups中，然后两两调用 canConnect
bool Map::isSolvable()
{
    //键值对，eg.:{1: [(0,0), (1,1)], 2: [(2,2)]}
    QMap<int, QVector<Box*>> typeGroups;

    for (Box* box : m_boxes) {
        if (!box) continue;
        int r = box->row;
        int c = box->col;
        // 防护：坐标越界或已经被标记为 -1（空位）就跳过
        if (r < 0 || r >= m_rows || c < 0 || c >= m_cols) continue;
        int type = m_map[r][c];
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
                if (canConnect(a, b)) {
                    return true; // 找到一对可消，立即返回
                }
            }
        }
    }

    return false;
}

