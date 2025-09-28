#pragma once

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVector>
#include "savegamemanager.h"

class Character;
class Box;
class Map;
class Score;
class PowerUpManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void addCountdownTime(int seconds);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updateCountdown();
    void handleActivation(Box* box, Character* sender);   // 角色碰到 Box 后调用

    void onSaveGame();
    void onLoadGame();
    void togglePause();

private:
    void setupScene();
    void showGameOverDialog();
    void resetToTitleScreen();
    void createMenu();

private:
    // 图形相关
    QGraphicsScene *scene;
    QGraphicsView *view;

    QVector<Character*> characters;  // 所有角色数组

    // 地图相关
    const qreal mapWidth = 800;
    const qreal mapHeight = 600;
    int yNum = 4, xNum = 3, typeNum = 3;
    Map* gameMap;

    // 消除相关
    Box* lastActivatedBox = nullptr;
    QGraphicsPathItem* currentPathItem = nullptr;

    // 倒计时相关
    int initialCountdownTime = 60;     // 初始倒计时
    int countdownTime;
    QGraphicsTextItem* countdownText;
    QTimer* countdownTimer;
    bool isPaused;

    // 分数相关
    Score* score;

    // 存档管理
    SaveGameManager saveManager;
    // 道具
    PowerUpManager* powerUpManager;
};
