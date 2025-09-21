#include "score.h"
#include <QString>
#include <QGraphicsDropShadowEffect>

Score::Score(QGraphicsItem* parent)
    : QGraphicsTextItem(parent), score(0)
{
    setDefaultTextColor(QColorConstants::Svg::antiquewhite);
    setFont(QFont("Consolas", 16, QFont::Bold));
    setZValue(150);
    updateText();

    //描边
    QGraphicsDropShadowEffect* outline = new QGraphicsDropShadowEffect(this);
    outline->setColor(Qt::black);       // 描边颜色
    outline->setBlurRadius(8);          // 控制描边粗细（3-5px最佳）
    outline->setOffset(0, 0);           // 必须设为0才能居中描边
    setGraphicsEffect(outline);
}

void Score::increase(int delta)
{
    score += delta;
    updateText();
}

void Score::reset()
{
    score = 0;
    updateText();
}

void Score::setScore(int data){
    score = data;
    updateText();
};

int Score::getScore() const
{
    return score;
}

void Score::updateText()
{
    setPlainText(QString("Score：%1").arg(score));
}
