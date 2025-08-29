#pragma once
#include <QVector>
#include <QGraphicsScene>
#include <QString>
#include "box.h"

// Map 类：管理 m*n 的 Box 矩阵
class Map {
public:
    // 构造函数
    Map(int rows, int cols, int typeCount,
        const QString &spriteSheetPath,
        QGraphicsScene *scene, int frameSize = 26);

    // 析构函数，清理内存
    ~Map();

    // 将地图添加到场景
    void addToScene();

    // 获取地图行列数
    int rowCount() const { return m_rows; }
    int colCount() const { return m_cols; }

private:
    int m_rows;             // 行数
    int m_cols;             // 列数
    int m_typeCount;        // 可用的类型数量
    int m_frameSize;        // 精灵图小块大小（正方形）
    QString m_spriteSheetPath;

    QGraphicsScene *m_scene;
    QVector<QVector<int>> m_map;      // 存储类型编号矩阵
    QVector<Box*> m_boxes;            // 存储生成的 Box 实例

    // 初始化随机地图
    void initMap();

    // 根据类型编号生成 QPixmap
    QPixmap getSpriteByType(int typeId);
};
