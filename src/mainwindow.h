#pragma once

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVector>
#include <QTimer>
#include "savegamemanager.h"

class Character;
class Box;
class Map;
class Score;
class PowerUpManager;
class StartMenu;
class QGraphicsTextItem;
class QGraphicsPathItem;

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
    void handleActivation(Box* box, Character* sender);

    void onSaveGame();
    void onLoadGame();
    void togglePause();

private:
    // helper functions
    void setupSceneDefaults(QGraphicsScene *s);
    void createMenu();
    void startGame(int playerCount);
    void cleanupGameResources();
    void resetToTitleScreen();
    void showGameOverDialog();

    // 道具处理函数
    void handleToolActivation(Box* box, Character* sender);
    void handleAddTimeTool(Character* sender);
    void handleShuffleTool( Character* sender);
    void handleHintTool(Character* sender);

    // 方块连接处理函数
    void handleBoxConnection(Box* box, Character* sender);
    void handleSuccessfulConnection(Box* box1, Box* box2, Character* sender);
    void handleFailedConnection(Box* lastBox, Box* newBox, Character* sender);

    // 通用辅助函数
    void showFeedbackText(const QString& text, const QColor& color, const QPointF& position);
    void showConnectionPath();

private:
    // 主菜单（作为 MainWindow 的子控件并一直保留）
    StartMenu* startMenu;

    // 懒创建的图形元素
    QGraphicsScene *scene = nullptr;
    QGraphicsView *view = nullptr;

    QVector<Character*> characters;

    // 地图与网格
    const qreal mapWidth = 800;
    const qreal mapHeight = 600;
    const QPointF mapPixSize = QPointF(mapWidth, mapHeight);
    int yNum = 4, xNum = 6, typeNum = 4;
    Map* gameMap = nullptr;

    // 交互相关
    QGraphicsPathItem* currentPathItem = nullptr;

    // 倒计时
    int initialCountdownTime = 120;
    int countdownTime = 0;
    QGraphicsTextItem* countdownText = nullptr;
    QTimer* countdownTimer = nullptr;
    bool isPaused = false;

    // 分数
    Score* score = nullptr;

    // 存档管理
    SaveGameManager saveManager;

    // 道具管理
    PowerUpManager* powerUpManager = nullptr;
    QTimer* powerUpSpawnTimer = nullptr;
};
