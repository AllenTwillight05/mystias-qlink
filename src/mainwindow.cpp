#include "mainwindow.h"
#include "startmenu.h"
#include "character.h"
#include "map.h"
#include "score.h"
#include "box.h"
#include "powerupmanager.h"
#include "savegamemanager.h"

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
    currentPathItem(nullptr),
    countdownText(nullptr),
    countdownTimer(new QTimer(this)),
    isPaused(false),
    score(nullptr),
    saveManager(this),
    powerUpManager(nullptr),
    powerUpSpawnTimer(nullptr)
{
    setWindowTitle("Mystia’s Ingredient Quest");
    resize(1200, 675);

    // 设置窗口图标
    // setWindowIcon(QIcon(":/assets/tpicon.ico"));
    setWindowIcon(QIcon(":/assets/tpicon.png"));

    // 初始显示主菜单
    setCentralWidget(startMenu);

    // 连接主菜单信号
    connect(startMenu, &StartMenu::startSinglePlayer, this, [this]() { startGame(1); });
    connect(startMenu, &StartMenu::startMultiPlayer, this, [this]() { startGame(2); });

    // 连接存档管理器的错误信号
    connect(&saveManager, &SaveGameManager::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("加载失败"), message);
    });
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow destructor called";
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
    qDebug() << "=== Starting game with" << playerCount << "players ===";

    // 如果已有游戏在运行，先清理
    cleanupGameResources();

    // 确保所有删除操作完成
    QCoreApplication::processEvents();

    // 新建 scene 和 view
    scene = new QGraphicsScene(this);
    setupSceneDefaults(scene);

    view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::TextAntialiasing);
    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 切换 central widget
    setCentralWidget(view);

    // 创建菜单
    createMenu();

    // 初始化道具管理器
    powerUpManager = new PowerUpManager(this);

    // 创建地图并加入场景
    gameMap = new Map(yNum, xNum, typeNum, ":/assets/ingredient.png", scene, 26);
    gameMap->addToScene();

    // 初始化道具管理器（依赖 map）
    powerUpManager->initialize(gameMap, scene);

    // 创建角色 - 确保完全清理旧角色
    characters.clear();
    if (playerCount >= 1) {
        Character* character1 = new Character(":/assets/sprites0.png", this);
        character1->setPos(mapWidth/5, mapHeight/5);
        character1->setControls({ Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D });
        character1->setGameMap(gameMap);
        scene->addItem(character1);
        connect(character1, &Character::collidedWithBox, this, &MainWindow::handleActivation);
        characters.append(character1);
        qDebug() << "Created player 1";
    }
    if (playerCount >= 2) {
        Character* character2 = new Character(":/assets/sprites1.png", this);
        character2->setPos(mapWidth/5*4, mapHeight/5*4);
        character2->setControls({ Qt::Key_I, Qt::Key_K, Qt::Key_J, Qt::Key_L });
        character2->setGameMap(gameMap);
        scene->addItem(character2);
        connect(character2, &Character::collidedWithBox, this, &MainWindow::handleActivation);
        characters.append(character2);
        qDebug() << "Created player 2";
    }

    // 道具生成定时器
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

    // 连接倒计时
    if (countdownTimer) {
        countdownTimer->stop();
        disconnect(countdownTimer, nullptr, this, nullptr);
    }
    connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    countdownTimer->start(1000);

    isPaused = false;
    qDebug() << "=== Game started successfully ===";
}

/* ---------------------- 清理游戏资源，保证顺序与安全 ---------------------- */
void MainWindow::cleanupGameResources()
{
    static bool isCleaningUp = false;
    if (isCleaningUp) return;
    isCleaningUp = true;

    qDebug() << "=== Starting cleanupGameResources ===";

    // 1. 停止所有定时器
    if (countdownTimer) {
        countdownTimer->stop();
        disconnect(countdownTimer, nullptr, this, nullptr);
    }

    if (powerUpSpawnTimer) {
        powerUpSpawnTimer->stop();
        disconnect(powerUpSpawnTimer, nullptr, this, nullptr);
        powerUpSpawnTimer->deleteLater();
        powerUpSpawnTimer = nullptr;
    }

    // 2. 停用道具管理器
    if (powerUpManager) {
        powerUpManager->deactivateHint();
        powerUpManager->deleteLater();
        powerUpManager = nullptr;
    }

    // 3. 安全清理角色 - 使用 deleteLater
    qDebug() << "Cleaning up" << characters.size() << "characters";
    for (Character* character : characters) {
        if (character) {
            qDebug() << "Safely removing character";

            // 断开所有连接
            disconnect(character, nullptr, this, nullptr);

            // 停止定时器
            character->stopTimers();

            // 从场景中移除
            if (scene && scene->items().contains(character)) {
                scene->removeItem(character);
            }

            // 使用 deleteLater 而不是立即删除
            character->deleteLater();
        }
    }
    characters.clear();

    // 4. 清理路径项 (QGraphicsPathItem 不是 QObject，需要直接删除)
    if (currentPathItem) {
        if (scene && scene->items().contains(currentPathItem)) {
            scene->removeItem(currentPathItem);
        }
        delete currentPathItem; // 直接删除
        currentPathItem = nullptr;
    }

    // 5. 清理倒计时文本 (QGraphicsTextItem 不是 QObject，需要直接删除)
    if (countdownText) {
        if (scene && scene->items().contains(countdownText)) {
            scene->removeItem(countdownText);
        }
        delete countdownText; // 直接删除
        countdownText = nullptr;
    }

    // 6. 清理 view 和 scene (QGraphicsView 是 QObject，可以使用 deleteLater)
    if (view) {
        setCentralWidget(nullptr);
        view->deleteLater();
        view = nullptr;
        scene = nullptr;
    }

    // 7. 清理地图 (Map 不是 QObject，需要直接删除)
    // 在 scene 清理之后，否则 m_tools 和 m_boxes 内的对象会内存泄漏
    if (gameMap) {
        delete gameMap; // 直接删除
        gameMap = nullptr;
    }

    // 8. 重置状态
    lastActivatedBox = nullptr;
    countdownTime = initialCountdownTime;
    isPaused = false;

    // 强制处理所有待删除对象
    QCoreApplication::processEvents();

    qDebug() << "=== Finished cleanupGameResources ===";
    isCleaningUp = false;
}

/* ---------------------- 回到主菜单 ---------------------- */
// void MainWindow::resetToTitleScreen()
// {
//     qDebug() << "=== Starting resetToTitleScreen ===";

//     // 暂停所有活动
//     isPaused = true;

//     // 清理游戏资源
//     cleanupGameResources();

//     // 确保处理完所有待处理的事件
//     QCoreApplication::processEvents();

//     // 重新创建 startMenu
//     if (startMenu) {
//         qDebug() << "Deleting existing startMenu";
//         startMenu->deleteLater();
//         startMenu = nullptr;
//     }

//     // 添加调试信息
//     qDebug() << "Creating new StartMenu";
//     startMenu = new StartMenu(this);

//     // 检查 StartMenu 是否创建成功
//     if (!startMenu) {
//         qDebug() << "Failed to create StartMenu!";
//         return;
//     }

//     // 重新连接信号
//     connect(startMenu, &StartMenu::startSinglePlayer, this, [this]() { startGame(1); });
//     connect(startMenu, &StartMenu::startMultiPlayer, this, [this]() { startGame(2); });

//     qDebug() << "Setting central widget to start menu";
//     setCentralWidget(startMenu);

//     // 重置状态
//     isPaused = false;

//     // 强制刷新界面
//     update();
//     repaint();

//     qDebug() << "=== Finished resetToTitleScreen ===";
// }

void MainWindow::resetToTitleScreen()
{
    qDebug() << "=== Starting resetToTitleScreen ===";

    // 立即暂停
    isPaused = true;

    // 清理游戏资源
    cleanupGameResources();

    // 确保所有删除操作完成
    QCoreApplication::processEvents();

    // 创建新的开始菜单
    startMenu = new StartMenu(this);
    connect(startMenu, &StartMenu::startSinglePlayer, this, [this]() { startGame(1); });
    connect(startMenu, &StartMenu::startMultiPlayer, this, [this]() { startGame(2); });

    setCentralWidget(startMenu);

    // 重置暂停状态
    isPaused = false;

    qDebug() << "=== Finished resetToTitleScreen ===";
}


/* ---------------------- 输入事件转发 ---------------------- */
/* ---------------------- 安全的输入事件处理 ---------------------- */
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        togglePause();
        return;
    }

    // 更严格的安全检查
    if (characters.isEmpty() || !scene || !gameMap) {
        qDebug() << "Key press ignored - game not ready";
        return;
    }

    for (int i = 0; i < characters.size(); ++i) {
        Character* c = characters[i];
        if (c && c->scene() == scene) { // 确保角色仍在scene中
            c->handleKeyPress(event);
        } else if (c == nullptr) {
            qDebug() << "Warning: null character at index" << i;
            characters.removeAt(i);
            --i; // 调整索引
        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (characters.isEmpty() || !scene || !gameMap) {
        qDebug() << "Key release ignored - game not ready";
        return;
    }

    for (int i = 0; i < characters.size(); ++i) {
        Character* c = characters[i];
        if (c && c->scene() == scene) { // 确保角色仍在scene中
            c->handleKeyRelease(event);
        } else if (c == nullptr) {
            qDebug() << "Warning: null character at index" << i;
            characters.removeAt(i);
            --i; // 调整索引
        }
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

// void MainWindow::onLoadGame()
// {
//     isPaused = true;
//     for (Character* c : characters) c->isPaused = true;

//     if (characters.isEmpty()) {
//         QMessageBox::warning(this, tr("加载游戏"), tr("没有可用的角色"));
//         return;
//     }

//     QString filename = QFileDialog::getOpenFileName(this, tr("加载游戏"), QDir::currentPath(), tr("连连看存档 (*.lksav)"));
//     if (!filename.isEmpty()) {
//         if (saveManager.loadGame(filename, *gameMap, characters, countdownTime)) {
//             QMessageBox::information(this, tr("加载游戏"), tr("游戏已成功加载!"));
//             if (countdownText) countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
//             for (Character* character : characters) {
//                 character->getCharacterScore()->updateText();
//             }
//         }
//     }

//     isPaused = false;
//     for (Character* c : characters) c->isPaused = false;
// }

void MainWindow::onLoadGame()
{
    isPaused = true;
    for (Character* c : characters) c->isPaused = true;

    QString filename = QFileDialog::getOpenFileName(this, tr("加载游戏"), QDir::currentPath(), tr("连连看存档 (*.lksav)"));
    if (!filename.isEmpty()) {
        // 直接调用现有的加载方法，但检查返回值
        if (saveManager.loadGame(filename, *gameMap, characters, countdownTime)) {
            QMessageBox::information(this, tr("加载游戏"), tr("游戏已成功加载!"));
            if (countdownText) countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
            for (Character* character : characters) {
                character->getCharacterScore()->updateText();
            }
        } else {
            // 加载失败，已经通过errorOccurred信号显示了错误信息
        }
    }

    isPaused = false;
    for (Character* c : characters) c->isPaused = false;
}

/* ---------------------- Game Over 弹窗 ---------------------- */
void MainWindow::showGameOverDialog()
{
    qDebug() << "=== Starting showGameOverDialog ===";

    // 1. 先停止所有活动，但不要立即清理
    isPaused = true;

    if (countdownTimer) {
        countdownTimer->stop();
        disconnect(countdownTimer, nullptr, this, nullptr);
    }

    if (powerUpSpawnTimer) {
        powerUpSpawnTimer->stop();
        disconnect(powerUpSpawnTimer, nullptr, this, nullptr);
    }

    // 2. 显示游戏结束对话框
    QMessageBox::information(this, "Game Over", "采集完毕~\n今天也是干劲十足");

    // 3. 使用单次定时器延迟重置，确保当前调用栈完成
    QTimer::singleShot(100, this, [this]() {
        qDebug() << "Delayed reset to title screen";
        resetToTitleScreen();
    });

    qDebug() << "=== Finished showGameOverDialog ===";
}
