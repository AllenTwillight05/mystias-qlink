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
    const int spacing = m_frameSize + 20; // 格子间隔

    // 整个地图像素宽高
    int totalWidth  = m_cols * spacing;
    int totalHeight = m_rows * spacing;

    // 场景中心
    QRectF sceneRect = m_scene->sceneRect();
    QPointF sceneCenter = sceneRect.center();

    // 偏移量 = 场景中心 - 一半地图尺寸
    qreal offsetX = sceneCenter.x() - totalWidth  / 2.0;
    qreal offsetY = sceneCenter.y() - totalHeight / 2.0;

    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            // 直接计算中心点坐标（无需补偿）
            QPointF centerPos(
                offsetX + j * spacing,
                offsetY + i * spacing
                );

            Box *box = new Box(centerPos, m_spriteSheetPath, m_scene);
            box->setPixmap(getSpriteByType(m_map[i][j]));
            m_boxes.append(box);
        }
    }
}
