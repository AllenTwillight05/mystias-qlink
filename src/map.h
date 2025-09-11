#pragma once

#include <QVector>
#include <QGraphicsScene>
#include <QString>
#include <QPoint>
#include <QPointF>
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
    QGraphicsScene* getScene() const { return m_scene; }

    // 判定两 Box 是否可连接
    bool canConnect(Box* a, Box* b);
    bool isSolvable();

    QVector<Box*> m_boxes;            // 存储生成的 Box 实例
    QVector<QVector<int>> m_map;      // 存储类型编号矩阵（二维数组）
    QGraphicsScene *m_scene;          // map 场景

    // 存储最近一次判定成功的路径（节点坐标）（网格/像素）
    QVector<QPoint>  m_pathCells;
    QVector<QPointF> m_pathPixels;

private:
    int m_rows;             // 行数
    int m_cols;             // 列数
    int m_typeCount;        // 可用的类型数量
    int m_frameSize;        // 精灵图小块大小（正方形）
    QString m_spriteSheetPath;
    int *disOrder;          // 打乱用数组

    // 初始化随机地图
    void initMap();

    // 根据类型编号生成 QPixmap
    QPixmap getSpriteByType(int typeId);

    // 工具函数：坐标换算
    QPointF cellCenterPx(int r, int c) const;
    QVector<QPointF> cellsToScene(const QVector<QPoint>& cells) const;

    // 直连、一拐、二拐路径判定（返回路径点）
    bool straightConnect(int r1, int c1, int r2, int c2,
                         const QVector<QVector<int>>& grid,
                         QVector<QPoint>& outPath) const;

    bool oneTurnConnect(int r1, int c1, int r2, int c2,
                        const QVector<QVector<int>>& grid,
                        QVector<QPoint>& outPath) const;

    bool twoTurnConnect(int r1, int c1, int r2, int c2,
                        const QVector<QVector<int>>& grid,
                        int rows, int cols,
                        QVector<QPoint>& outPath) const;

};
