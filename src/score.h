#pragma once

#include <QGraphicsTextItem>
#include <QFont>

class Score : public QGraphicsTextItem
{
public:
    Score(QGraphicsItem* parent = nullptr);

    void increase(int delta = 10);  // 增加分数
    void reset();                   // 重置分数
    void setScore(int data);        // 设置分数
    int getScore() const;           // 获取当前分数
    void updateText();

private:
    int score;
};
