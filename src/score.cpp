#include "score.h"
#include <QString>

Score::Score(QGraphicsItem* parent)
    : QGraphicsTextItem(parent), score(0)
{
    setDefaultTextColor(QColorConstants::Svg::saddlebrown);
    setFont(QFont("Consolas", 20));
    updateText();
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

int Score::getScore() const
{
    return score;
}

void Score::updateText()
{
    setPlainText(QString("Score：%1").arg(score));
}
