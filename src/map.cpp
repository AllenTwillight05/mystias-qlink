#include "map.h"
#include <QPixmap>
#include <QRandomGenerator>
#include <QDebug>
#include <random>
#include <algorithm>

// 构造，传入行、列、方块种类、spritesheet贴图、所在场景、单帧方形贴图边长（pix)
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
    addToScene();
}

// 析构
Map::~Map()
{
    qDebug() << "Map destructor called";

    // 安全清理 boxes - 让 scene 管理它们的生命周期
    // 不直接删除 boxes，因为它们已经被 scene 管理，仅仅移除
    // 此行只清空了QList<Box*>指针容器，Box对象本身仍然存在于scene中，当scene被删除时，它仍然会删除这些 Box对象
    // 因此在mainWindow中先清理scene再把m_boxes和m_tools数组清空（此时内部对象已经析构）
    m_boxes.clear();
    m_tools.clear();

    delete[] disOrder;
    disOrder = nullptr;
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

// 辅助构造函数1：初始化地图，对spritesheet随机选择typecount帧编号，并与空格编号一起随机生成在地图二维数组m_map中
void Map::initMap()
{
    disOrder = new int[m_typeCount + 1];    //乱序数组，存储随机到的box编号以及空格多（赋-1）
    for (int i = 0; i < m_typeCount; ++i) {
        //disOrder[i] = QRandomGenerator::global()->bounded(164);    // 精灵图recipe：164帧
        disOrder[i] = QRandomGenerator::global()->bounded(62);    // 精灵图参数ingridient：62帧
    }
    disOrder[m_typeCount] = -1;

    m_map.resize(m_rows);   // 二维数组m_map设置行数
    for (int i = 0; i < m_rows; i++) {
        m_map[i].resize(m_cols);
        for (int j = 0; j < m_cols; j++) {
            int randomIndex = QRandomGenerator::global()->bounded(m_typeCount + 1);
            m_map[i][j] = disOrder[randomIndex];
        }
    }
}

// 辅助构造函数2： 按照m_map在scene中添加实体贴图
void Map::addToScene()
{
    // 计算scene中地图大小（pix）（“网格”中“结点”处放置box，“格子”为spacing占空）
    int totalWidth  = (m_cols - 1) * spacing;
    int totalHeight = (m_rows - 1) * spacing;

    // 设置 offset确保地图在scene中居中
    QRectF sceneRect = m_scene->sceneRect();    // 浮点数矩形，包括左上坐标与长宽
    QPointF sceneCenter = sceneRect.center();
    qreal offsetX = sceneCenter.x() - totalWidth  / 2.0;
    qreal offsetY = sceneCenter.y() - totalHeight / 2.0;

    // 在“网格”结点上放置裁切后的spritesheet帧
    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            int typeId = m_map[i][j];
            if(typeId == -1) continue;
            QPixmap sprite = getSpriteByType(typeId);   // 裁切
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

// spritesheet裁切，传入帧序号，返回相应帧（只要传入的m_frameSize与传入的spritesheet相匹配，可动态适应行列帧数）
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

    // 裁切
    QRect sourceRect(col * m_frameSize, row * m_frameSize,
                     m_frameSize, m_frameSize);
    return spriteSheet.copy(sourceRect);
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

void Map::shuffleBoxes()
{
    if (m_boxes.isEmpty()) return;

    // 1. 收集所有普通方块的类型（排除道具方块）
    QVector<int> boxTypes;
    for (Box* box : m_boxes) {
        boxTypes.append(m_map[box->row][box->col]);
    }

    // 2. 获取所有可用位置（排除道具方块占据的位置）
    QVector<QPoint> availablePositions;
    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            // 检查该位置是否被道具占据
            bool occupiedByTool = false;
            for (Box* tool : m_tools) {
                if (tool->row == i && tool->col == j) {
                    occupiedByTool = true;
                    break;
                }
            }
            if (!occupiedByTool) {
                availablePositions.append(QPoint(j, i)); // QPoint(x,y) 对应 (col,row)
            }
        }
    }

    // 3. 随机打乱类型和位置
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(boxTypes.begin(), boxTypes.end(), g);
    std::shuffle(availablePositions.begin(), availablePositions.end(), g);

    // 4. 更新地图数据和方块位置
    // 先清空地图（保留道具位置）
    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            // 如果不是道具位置，就设为-1
            bool isToolPos = false;
            for (Box* tool : m_tools) {
                if (tool->row == i && tool->col == j) {
                    isToolPos = true;
                    break;
                }
            }
            if (!isToolPos) {
                m_map[i][j] = -1;
            }
        }
    }

    // 5. 重新分配方块位置和更新场景显示
    for (int i = 0; i < m_boxes.size() && i < availablePositions.size(); i++) {
        Box* box = m_boxes[i];
        QPoint newPos = availablePositions[i];
        int newType = boxTypes[i];

        // 更新地图数据
        m_map[newPos.y()][newPos.x()] = newType;

        // 更新方块属性
        box->row = newPos.y();
        box->col = newPos.x();
        box->boxType = newType;

        // 更新精灵图
        QPixmap newSprite = getSpriteByType(newType);
        if (!newSprite.isNull()) {
            box->setPixmap(newSprite);
        }

        // 更新场景位置
        QPointF scenePos = cellCenterPx(newPos.y(), newPos.x());
        box->setPos(scenePos);
    }

    qDebug() << "Shuffle completed:" << m_boxes.size() << "boxes rearranged";
}

