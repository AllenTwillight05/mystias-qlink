#include "box.h"
#include <QPixmap>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QGraphicsDropShadowEffect>

Box::Box(const QPointF &pos, const QString &imagePath, QGraphicsScene *scene)
{
    setupSprite(imagePath);
    setOffset(-pixmap().width()/2, -pixmap().height()/2); // 中心对齐
    setPos(pos);

    // 在构造函数中添加（需要#include <QGraphicsEllipseItem>）
    QGraphicsEllipseItem* debugMarker = new QGraphicsEllipseItem(-3, -3, 6, 6, this); // 创建坐标小圆点
    debugMarker->setBrush(Qt::green); // 设置为绿色
    debugMarker->setZValue(100);
    debugMarker->setPos(0, 0); // 相对于父项的位置（中心点）

    this->setScale(1.5);
    this->setZValue(1);
    scene->addItem(this);
}

Box::Box(const QString &imagePath, QGraphicsScene *scene, const QPointF& characterPos):
    Box(generateRandomPosition(scene->sceneRect(), characterPos), imagePath, scene)// 委托构造：调用上面的第一个构造函数
{
    setupSprite(imagePath);
}

// 辅助随机构造函数
QPointF Box::generateRandomPosition(const QRectF &sceneRect,const QPointF &characterPos) {
    QPointF boxPos;
    do{
        boxPos.setX(static_cast<int>(sceneRect.left()) + QRandomGenerator::global()->bounded(static_cast<int>(sceneRect.width())));
        boxPos.setY(static_cast<int>(sceneRect.top()) + QRandomGenerator::global()->bounded(static_cast<int>(sceneRect.height())));
    }while(QLineF(characterPos, boxPos).length() < boxSize * 4);
    return QPointF(boxPos);
}

void Box::setupSprite(const QPixmap &imagePath, int frameSize){
    // 精灵图参数
    const int cols = 10;   // 横向10个小图
    const int rows = 17;   // 纵向17个小图
    const int skipLast = 6; // 右下角6个为空

    // !!!生成随机帧索引 [0, totalFrames-1]，int转换为整数，不包括最后六个空位!!!
    int totalFrames = cols * rows - skipLast;
    int randomFrame = QRandomGenerator::global()->bounded(totalFrames);

    int row = randomFrame / cols;
    int col = randomFrame % cols;

    QRect sourceRect(col * frameSize, row * frameSize, frameSize, frameSize);

    setPixmap(imagePath.copy(sourceRect));
    setOffset(-frameSize/2, -frameSize/2); // 中心对齐（构造函数中已经实现，可选择性删去）

}

void Box::preAct() {
    // 如果已有遮罩则先删除
    if (m_overlay) delete m_overlay;
    // 创建新遮罩
    m_overlay = new QGraphicsRectItem(this->boundingRect(), this);
    m_overlay->setBrush(QColor(0, 0, 0, 120));  // 调整颜色和透明度
    m_overlay->setPen(Qt::NoPen);  // 去掉边框
    m_overlay->setZValue(-1);
    m_overlay->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

}

void Box::npreAct() {
    if (m_overlay) {
        // 保险起见先从父子关系里摘掉（不是必须，但能少点脏区关联）
        m_overlay->setParentItem(nullptr);
        delete m_overlay;
        m_overlay = nullptr;

        // 关键：无效化 + 重绘
        if (auto eff = graphicsEffect()) {      // 若有发光效果，先让它重新抓取源图
            eff->setEnabled(false);
            eff->setEnabled(true);
        }
        update();                                // 申请重绘当前 Item
        if (scene())                             // 再保险一点：让场景也重绘此区域
            scene()->invalidate(sceneBoundingRect(), QGraphicsScene::AllLayers);
    }
}


void Box::activate(){
    this->setScale(2);
    auto* glow = new QGraphicsDropShadowEffect;
    glow->setColor(Qt::yellow);    // 金色发光
    glow->setBlurRadius(20);        // 光晕大小
    glow->setOffset(0);             // 居中发光
    this->setGraphicsEffect(glow);  // 应用特效
}
void Box::deactivate(){
    this->setScale(1.5);
    // 移除发光效果
    this->setGraphicsEffect(nullptr);
}
