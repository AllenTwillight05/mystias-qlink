#include "mainwindow.h"
#include "startmenu.h"
#include "character.h"
#include "map.h"
#include "score.h"
#include "box.h"
#include "powerupmanager.h"

#include <QTimer>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QDebug>
#include <QDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    startMenu(new StartMenu(this)),
    scene(nullptr),
    view(nullptr),
    gameMap(nullptr),
    countdownText(nullptr),
    countdownTimer(new QTimer(this)),
    isPaused(false),
    score(nullptr),
    saveManager(this),
    powerUpManager(nullptr),
    powerUpSpawnTimer(nullptr),
    currentPathItem(nullptr)
{
    setWindowTitle("Mystia’s Ingredient Quest");
    resize(1200, 675);

    // 初始显示主菜单（StartMenu 永远存在）
    setCentralWidget(startMenu);

    // 连接主菜单信号
    connect(startMenu, &StartMenu::startSinglePlayer, this, [this]() { startGame(1); });
    connect(startMenu, &StartMenu::startMultiPlayer, this, [this]() { startGame(2); });
}

MainWindow::~MainWindow()
{
    // 彻底清理游戏资源（StartMenu 由 parent 自动删除）
    cleanupGameResources();
}

/* ---------------------- 辅助：场景默认设置 ---------------------- */
void MainWindow::setupSceneDefaults(QGraphicsScene *s)
{
    if (!s) return;
    s->setSceneRect(0, 0, mapWidth, mapHeight);
    s->setBackgroundBrush(QColorConstants::Svg::antiquewhite);

    QPixmap bgPixmap(":/assets/background.jpg");
    if (!bgPixmap.isNull()) {
        QGraphicsPixmapItem *bgItem = s->addPixmap(bgPixmap);
        bgItem->setZValue(-100);
    }
}

/* ---------------------- 开始游戏：创建 scene/view/map/角色 ---------------------- */
void MainWindow::startGame(int playerCount)
{
    // 如果已有游戏在运行，先清理
    cleanupGameResources();

    // 新建 scene 和 view（把 scene 的父对象设为 this 便于管理）
    scene = new QGraphicsScene(this);
    setupSceneDefaults(scene);

    view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::TextAntialiasing);
    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 切换 central widget（替换 startMenu）
    setCentralWidget(view);

    // 创建菜单（会先清空旧菜单）
    createMenu();

    // 初始化道具管理器
    powerUpManager = new PowerUpManager(this);

    // 创建地图并加入场景
    gameMap = new Map(yNum, xNum, typeNum, ":/assets/ingredient.png", scene, 26);
    gameMap->addToScene();

    // 初始化道具管理器（依赖 map）
    powerUpManager->initialize(gameMap, scene);

    // 创建角色
    if (playerCount >= 1) {
        Character* character1 = new Character(":/assets/sprites0.png", this);
        character1->setPos(mapWidth/5, mapHeight/5);
        character1->setControls({ Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D });
        character1->setGameMap(gameMap);
        scene->addItem(character1);
        connect(character1, &Character::collidedWithBox, this, &MainWindow::handleActivation);
        characters.append(character1);
    }
    if (playerCount >= 2) {
        Character* character2 = new Character(":/assets/sprites1.png", this);
        character2->setPos(mapWidth/5*4, mapHeight/5*4);
        character2->setControls({ Qt::Key_I, Qt::Key_K, Qt::Key_J, Qt::Key_L });
        character2->setGameMap(gameMap);
        scene->addItem(character2);
        connect(character2, &Character::collidedWithBox, this, &MainWindow::handleActivation);
        characters.append(character2);
    }

    // 道具生成定时器（先生成一个）
    powerUpSpawnTimer = new QTimer(this);
    connect(powerUpSpawnTimer, &QTimer::timeout, this, [this]() {
        if (powerUpManager) powerUpManager->spawnPowerUp(QRandomGenerator::global()->bounded(3) + 1);
    });
    if (powerUpManager) powerUpManager->spawnPowerUp(QRandomGenerator::global()->bounded(3) + 1);
    powerUpSpawnTimer->start(15000);

    // 倒计时文本
    countdownTime = initialCountdownTime;
    countdownText = scene->addText(QString("Time：%1").arg(countdownTime));
    countdownText->setDefaultTextColor(QColorConstants::Svg::saddlebrown);
    countdownText->setFont(QFont("Consolas", 20, QFont::Bold));
    countdownText->setZValue(100);
    countdownText->setPos(20, 20);

    // 连接倒计时（防止重复连接）
    connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown, Qt::UniqueConnection);
    countdownTimer->start(1000);

    isPaused = false;
}

/* ---------------------- 清理游戏资源，保证顺序与安全 ---------------------- */
void MainWindow::cleanupGameResources()
{
    // 停止倒计时
    if (countdownTimer) {
        countdownTimer->stop();
        QObject::disconnect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    }

    // 停止并删除道具生成定时器
    if (powerUpSpawnTimer) {
        powerUpSpawnTimer->stop();
        delete powerUpSpawnTimer;
        powerUpSpawnTimer = nullptr;
    }

    // 停停 hint 并删除道具管理器
    if (powerUpManager) {
        powerUpManager->deactivateHint();
        delete powerUpManager;
        powerUpManager = nullptr;
    }

    // 删除地图（Map 的析构应处理其 own items）
    if (gameMap) {
        delete gameMap;
        gameMap = nullptr;
    }

    // 删除并停止角色
    // for (Character* c : characters) {
    //     if (c) {
    //         c->stopTimers(); // 防止内部计时器还在回调
    //         if (scene && scene->items().contains(c)) scene->removeItem(c);
    //         delete c;
    //     }
    // }
    characters.clear();

    // 删除倒计时文本
    if (countdownText) {
        if (scene && scene->items().contains(countdownText)) scene->removeItem(countdownText);
        delete countdownText;
        countdownText = nullptr;
    }

    // 删除当前绘制路径
    if (currentPathItem) {
        if (scene && scene->items().contains(currentPathItem)) scene->removeItem(currentPathItem);
        delete currentPathItem;
        currentPathItem = nullptr;
    }

    // 删除 scene 中残留的 items（谨慎）
    // if (scene) {
    //     QList<QGraphicsItem*> items = scene->items();
    //     for (QGraphicsItem* it : items) {
    //         scene->removeItem(it);
    //         delete it;
    //     }
    // }

    // 从 central widget 中移除 view（如果存在）
    if (view && centralWidget() == view) {
        setCentralWidget(nullptr);
    }

    // 删除 view 与 scene（按先后顺序）
    if (view) {
        delete view;
        view = nullptr;
    }
    if (scene) {
        delete scene;
        scene = nullptr;
    }

    // 清空菜单，避免重复项
    menuBar()->clear();
}

/* ---------------------- 回到主菜单 ---------------------- */
void MainWindow::resetToTitleScreen()
{
    cleanupGameResources();

    // 切回 startMenu（它是 this 的子控件并一直存在）
    setCentralWidget(startMenu);
}

/* ---------------------- 输入事件转发 ---------------------- */
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        togglePause();
        return;
    }
    for (Character* c : characters) {
        c->handleKeyPress(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    for (Character* c : characters) {
        c->handleKeyRelease(event);
    }
}

/* ---------------------- 倒计时 ---------------------- */
void MainWindow::updateCountdown()
{
    if (isPaused) return;

    countdownTime--;
    if (countdownTime < 0) {
        countdownTime = 0;
        countdownTimer->stop();
        showGameOverDialog();
        return;
    }
    if (countdownText) countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
}

void MainWindow::addCountdownTime(int seconds)
{
    countdownTime += seconds;
    if (countdownText) countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
}

void MainWindow::togglePause()
{
    isPaused = !isPaused;
    for (Character* c : characters) c->isPaused = isPaused;
}

/* ---------------------- 角色与 box 交互（保留你的逻辑） ---------------------- */
void MainWindow::handleActivation(Box* box, Character* sender)
{
    if (!box) return;

    // 道具类型处理
    if (box->toolType >= 1) {
        switch (box->toolType) {
        case 1: {
            countdownTime += 30;
            if (countdownText) countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
            QGraphicsTextItem* feedback = scene->addText("+1s");
            feedback->setDefaultTextColor(Qt::green);
            feedback->setFont(QFont("Consolas", 16, QFont::Bold));
            feedback->setZValue(100);
            feedback->setPos(sender->getPosition());
            QTimer::singleShot(1000, [feedback]() {
                if (feedback->scene()) {
                    feedback->scene()->removeItem(feedback);
                    delete feedback;
                }
            });
            break;
        }
        case 2: {
            if (gameMap) gameMap->shuffleBoxes();
            QGraphicsTextItem* feedback = scene->addText("Shuffle!");
            feedback->setDefaultTextColor(Qt::blue);
            feedback->setFont(QFont("Consolas", 16, QFont::Bold));
            feedback->setZValue(100);
            feedback->setPos(sender->getPosition());
            QTimer::singleShot(1000, [feedback]() {
                if (feedback->scene()) {
                    feedback->scene()->removeItem(feedback);
                    delete feedback;
                }
            });
            break;
        }
        case 3: {
            if (powerUpManager) powerUpManager->activateHint();
            QGraphicsTextItem* feedback = scene->addText("Hint!");
            feedback->setDefaultTextColor(Qt::yellow);
            feedback->setFont(QFont("Consolas", 16, QFont::Bold));
            feedback->setZValue(100);
            feedback->setPos(sender->getPosition());
            QTimer::singleShot(1000, [feedback]() {
                if (feedback->scene()) {
                    feedback->scene()->removeItem(feedback);
                    delete feedback;
                }
            });
            break;
        }
        }

        if (gameMap) gameMap->m_tools.removeOne(box);
        if (scene) scene->removeItem(box);
        delete box;
        return;
    }

    // 消除逻辑
    if (!lastActivatedBox) {
        lastActivatedBox = box;
        box->activate();
        return;
    }

    if (box == lastActivatedBox) return;

    if (gameMap->canConnect(lastActivatedBox, box)) {
        lastActivatedBox->deactivate();
        box->deactivate();
        gameMap->m_map[lastActivatedBox->row][lastActivatedBox->col] = -1;
        gameMap->m_map[box->row][box->col] = -1;
        gameMap->getScene()->removeItem(lastActivatedBox);
        gameMap->getScene()->removeItem(box);
        gameMap->m_boxes.removeOne(lastActivatedBox);
        gameMap->m_boxes.removeOne(box);
        lastActivatedBox = nullptr;

        sender->getCharacterScore()->increase(10);

        const QVector<QPointF>& pts = gameMap->m_pathPixels;
        if (pts.size() >= 2 && scene) {
            QPainterPath path(pts[0]);
            for (int i = 1; i < pts.size(); ++i) path.lineTo(pts[i]);
            QPen pen(QColor(255, 223, 128), 10);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCapStyle(Qt::RoundCap);

            QGraphicsPathItem* lineItem = scene->addPath(path, pen);
            lineItem->setZValue(-1);
            currentPathItem = lineItem;

            QTimer::singleShot(500, [this, lineItem]() {
                if (lineItem && lineItem->scene()) {
                    lineItem->scene()->removeItem(lineItem);
                    delete lineItem;
                    if (currentPathItem == lineItem) currentPathItem = nullptr;
                }
            });
        }
    } else {
        lastActivatedBox->deactivate();
        lastActivatedBox = box;
        box->activate();
    }

    if (gameMap && !gameMap->isSolvable()) {
        showGameOverDialog();
    }
}

/* ---------------------- 菜单（返回主菜单 / 退出程序 / 保存/加载 等） ---------------------- */
void MainWindow::createMenu()
{
    menuBar()->clear();

    QMenu *gameMenu = menuBar()->addMenu(tr("选项"));

    QAction *saveAction = new QAction(tr("保存游戏"), this);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveGame);
    gameMenu->addAction(saveAction);

    QAction *loadAction = new QAction(tr("加载游戏"), this);
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoadGame);
    gameMenu->addAction(loadAction);

    gameMenu->addSeparator();

    QAction *togglePause = new QAction(tr("暂停/继续"), this);
    connect(togglePause, &QAction::triggered, this, &MainWindow::togglePause);
    gameMenu->addAction(togglePause);

    gameMenu->addSeparator();

    QAction *exitAction = new QAction(tr("返回主菜单"), this);
    connect(exitAction, &QAction::triggered, this, [this]() { resetToTitleScreen(); });
    gameMenu->addAction(exitAction);

    QAction *quitAction = new QAction(tr("退出程序"), this);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
    gameMenu->addAction(quitAction);
}

/* ---------------------- 保存 / 加载（保留你原来的逻辑） ---------------------- */
void MainWindow::onSaveGame()
{
    isPaused = true;
    for (Character* c : characters) c->isPaused = true;

    if (characters.isEmpty()) {
        QMessageBox::warning(this, tr("保存游戏"), tr("没有可用的角色"));
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this, tr("保存游戏"), QDir::currentPath(), tr("连连看存档 (*.lksav)"));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(".lksav")) filename += ".lksav";
        if (saveManager.saveGame(filename, *gameMap, characters, countdownTime)) {
            QMessageBox::information(this, tr("保存游戏"), tr("游戏已成功保存!"));
        }
    }

    isPaused = false;
    for (Character* c : characters) c->isPaused = false;
}

void MainWindow::onLoadGame()
{
    isPaused = true;
    for (Character* c : characters) c->isPaused = true;

    if (characters.isEmpty()) {
        QMessageBox::warning(this, tr("加载游戏"), tr("没有可用的角色"));
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this, tr("加载游戏"), QDir::currentPath(), tr("连连看存档 (*.lksav)"));
    if (!filename.isEmpty()) {
        if (saveManager.loadGame(filename, *gameMap, characters, countdownTime)) {
            QMessageBox::information(this, tr("加载游戏"), tr("游戏已成功加载!"));
            if (countdownText) countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
            for (Character* character : characters) {
                character->getCharacterScore()->updateText();
            }
        }
    }

    isPaused = false;
    for (Character* c : characters) c->isPaused = false;
}

/* ---------------------- Game Over 弹窗 ---------------------- */
void MainWindow::showGameOverDialog()
{
    for (Character* c : characters) {
        c->stopTimers();
    }
    if (countdownTimer) countdownTimer->stop();

    if (powerUpManager) powerUpManager->deactivateHint();

    QDialog dlg(this);
    dlg.setWindowTitle("Game Over");
    dlg.resize(300, 150);
    dlg.setStyleSheet("background-color: AntiqueWhite;");

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    QLabel* label = new QLabel("GAME OVER");
    label->setStyleSheet("color: SaddleBrown; font-size: 20px;");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    QPushButton* okButton = new QPushButton("Return to Menu");
    okButton->setStyleSheet("background-color: SaddleBrown; color: OldLace;");
    layout->addWidget(okButton);
    connect(okButton, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();

    resetToTitleScreen();
}
