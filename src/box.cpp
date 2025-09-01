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
    setOffset(-frameSize/2, -frameSize/2); // 中心对齐

}

bool Box::preAct(){
    this->setScale(2);
    return 0;
}

void Box::activate(){
    boxCond = 1;
    auto* glow = new QGraphicsDropShadowEffect;
    glow->setColor(Qt::yellow);    // 金色发光
    glow->setBlurRadius(20);        // 光晕大小
    glow->setOffset(0);             // 居中发光
    this->setGraphicsEffect(glow);  // 应用特效
}
