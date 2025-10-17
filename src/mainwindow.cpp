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

// mainwindow类构造函数
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

// mainwindow类析构函数
MainWindow::~MainWindow()
{
    qDebug() << "MainWindow destructor called";
    // 清理游戏资源（StartMenu 由 parent 自动删除）
    cleanupGameResources();
}

// 新建场景辅助函数，在startGame()中传入空场景scene，为scene添加3层QPixmap贴图
void MainWindow::setupSceneDefaults(QGraphicsScene *s)
{
    if (!s) return;
    s->setSceneRect(0, 0, mapWidth, mapHeight);
    s->setBackgroundBrush(QColorConstants::Svg::darkolivegreen);

    QPixmap bgPixmap_bottom(":/assets/background_bottom.png");
    if (!bgPixmap_bottom.isNull()) {
        QGraphicsPixmapItem *bgItem = s->addPixmap(bgPixmap_bottom);

        // 计算居中位置
        qreal x = (mapWidth - bgPixmap_bottom.width()) / 2.0;  // 相当于(800-1200)/2 = -200
        qreal y = (mapHeight - bgPixmap_bottom.height()) / 2.0; // 相当于(600-675)/2 = -37.5

        bgItem->setPos(x, y);  // 背景图居中，部分超出场景边界
        bgItem->setZValue(-100);
    }

    QPixmap bgPixmap_mid(":/assets/background_mid.png");
    if (!bgPixmap_mid.isNull()) {
        QGraphicsPixmapItem *bgItem = s->addPixmap(bgPixmap_mid);

        qreal x = (mapWidth - bgPixmap_mid.width()) / 2.0;
        qreal y = (mapHeight - bgPixmap_mid.height()) / 2.0;

        bgItem->setPos(x, y);
        bgItem->setZValue(100);
    }

    QPixmap bgPixmap_top(":/assets/background_top.png");
    if (!bgPixmap_top.isNull()) {
        QGraphicsPixmapItem *bgItem = s->addPixmap(bgPixmap_top);

        qreal x = (mapWidth - bgPixmap_top.width()) / 2.0;
        qreal y = (mapHeight - bgPixmap_top.height()) / 2.0;

        bgItem->setPos(x, y);
        bgItem->setZValue(101);
    }
}

// 开始游戏：创建 scene/view/map/角色。作为startMenu发出信号的slot函数（lambda表达式作为slot）
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

    // 抗锯齿与性能
    view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::TextAntialiasing);
    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 切换central widget从startMenu到view
    setCentralWidget(view);

    // 创建菜单
    createMenu();

    // 初始化道具管理器
    powerUpManager = new PowerUpManager(this);  //传入this作为PowerUpManager的父类，便于析构时候的内存管理

    // 创建box地图并加入场景
    gameMap = new Map(yNum, xNum, typeNum, ":/assets/ingredient.png", scene, 26);

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
        character2->setControls({ Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right });
        character2->setGameMap(gameMap);
        scene->addItem(character2);
        connect(character2, &Character::collidedWithBox, this, &MainWindow::handleActivation);
        characters.append(character2);
        qDebug() << "Created player 2";
    }

    // 道具生成定时器（每15s生成1个）
    powerUpSpawnTimer = new QTimer(this);
    connect(powerUpSpawnTimer, &QTimer::timeout, this, [this]() {
        if (powerUpManager) powerUpManager->spawnPowerUp(QRandomGenerator::global()->bounded(3) + 1);
    });
    if (powerUpManager) powerUpManager->spawnPowerUp(QRandomGenerator::global()->bounded(3) + 1);
    powerUpSpawnTimer->start(15000);

    // 倒计时文本
    countdownTime = initialCountdownTime;
    countdownText = scene->addText(QString("Time：%1").arg(countdownTime));
    countdownText->setDefaultTextColor(QColorConstants::Svg::burlywood);
    countdownText->setFont(QFont("Consolas", 20, QFont::Bold));
    countdownText->setZValue(102);
    countdownText->setPos(20, 20);

    // 连接倒计时，如果已经存在先消除
    if (countdownTimer) {
        countdownTimer->stop();
        disconnect(countdownTimer, nullptr, this, nullptr);
    }
    connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    countdownTimer->start(1000);

    isPaused = false;
    qDebug() << "=== Game started successfully ===";
}

// 按顺序清理游戏资源
void MainWindow::cleanupGameResources()
{
    static bool isCleaningUp = false;
    if (isCleaningUp) return;
    isCleaningUp = true;

    qDebug() << "=== Starting cleanupGameResources ===";

    // 0. 消除 menuBar
    if (menuBar()) {
        menuBar()->clear();
    }

    // 1. 停止所有定时器
    if (countdownTimer) {
        countdownTimer->stop();
        disconnect(countdownTimer, nullptr, this, nullptr); // 断开 this 的所有连接（从任何发送者）
    }

    if (powerUpSpawnTimer) {
        powerUpSpawnTimer->stop();
        disconnect(powerUpSpawnTimer, nullptr, this, nullptr);
        powerUpSpawnTimer->deleteLater();
        powerUpSpawnTimer = nullptr;
    }

    // 2. 停用道具类powerUpManager
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

            // 清除预选箱子
            character->clearLastActivatedBox();

            // 断开所有连接
            disconnect(character, nullptr, this, nullptr);

            // 停止定时器
            character->stopTimers();

            // 从场景中移除
            if (scene && scene->items().contains(character)) {
                scene->removeItem(character);
            }

            // 使用 deleteLater() 而不是立即删除
            // 避免在事件处理中间删除对象。
            // deleteLater()将一个延迟删除事件（QDeferredDeleteEvent）放入事件队列中。当事件循环处理到这个事件时，它会安全地删除该对象。
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
    // 在 scene 清理之后，否则先清理导致 m_tools 和 m_boxes 内的对象丢失，内存泄漏
    if (gameMap) {
        delete gameMap; // 直接删除
        gameMap = nullptr;
    }

    // 8. 重置状态
    countdownTime = initialCountdownTime;
    isPaused = false;

    // 强制处理所有待删除对象
    QCoreApplication::processEvents();

    qDebug() << "=== Finished cleanupGameResources ===";
    isCleaningUp = false;
}

// 返回主菜单界面
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
    // 添加焦点设置
    if (!hasFocus()) {
        setFocus();
    }

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

// 暂停与继续切换
void MainWindow::togglePause()
{
    isPaused = !isPaused;
    for (Character* c : characters) c->isPaused = isPaused;
}

/* ---------------------- 角色与 box 交互 ---------------------- */
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
    // 获取角色自己的 lastActivatedBox
    Box* lastBox = sender->getLastActivatedBox();

    if (!lastBox) {
        sender->setLastActivatedBox(box);
        box->activate();
        return;
    }

    if (box == lastBox) return;

    if (gameMap->canConnect(lastBox, box)) {
        lastBox->deactivate();
        box->deactivate();
        gameMap->m_map[lastBox->row][lastBox->col] = -1;
        gameMap->m_map[box->row][box->col] = -1;
        scene->removeItem(lastBox);
        scene->removeItem(box);
        gameMap->m_boxes.removeOne(lastBox);
        gameMap->m_boxes.removeOne(box);
        sender->setLastActivatedBox(nullptr);

        sender->getCharacterScore()->increase(10);

        const QVector<QPointF>& pts = gameMap->m_pathPixels;
        if (pts.size() >= 2 && scene) {
            QPainterPath path(pts[0]);
            for (int i = 1; i < pts.size(); ++i) path.lineTo(pts[i]);
            QPen pen(QColor(155, 123, 128), 10);
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
        lastBox->deactivate();
        sender->setLastActivatedBox(box);
        box->activate();
    }

    if (gameMap && !gameMap->isSolvable()) {
        showGameOverDialog();
    }
}

// 游戏界面顶部menubar，通过connect实现逻辑功能
void MainWindow::createMenu()
{
    menuBar()->clear();

    QMenu *gameMenu = menuBar()->addMenu(tr("选项")); //tr()为翻译函数，后续可使用lupdate提取文本生成.ts并编写对照翻译

    QAction *saveAction = new QAction(tr("保存游戏"), this);    //QAction为动作类，此处通过gameMenu->addAction()添加到菜单项
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

// 存档，通过connect到菜单项由&QAction::triggered信号触发
void MainWindow::onSaveGame()
{
    // 存档操作时暂停
    isPaused = true;
    for (Character* c : characters) c->isPaused = true;

    if (characters.isEmpty()) {
        QMessageBox::warning(this, tr("保存游戏"), tr("没有可用的角色"));
        return;
    }

    // 存档弹窗，补全后缀，提示成功保存
    QString filename = QFileDialog::getSaveFileName(this, tr("保存游戏"), QDir::currentPath(), tr("连连看存档 (*.lksav)"));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(".lksav")) filename += ".lksav";
        if (saveManager.saveGame(filename, *gameMap, characters, countdownTime)) {
            QMessageBox::information(this, tr("保存游戏"), tr("游戏已成功保存!"));
        }
    }

    // 游戏继续
    isPaused = false;
    for (Character* c : characters) c->isPaused = false;
}

// 读档，通过connect到菜单项由&QAction::triggered信号触发
void MainWindow::onLoadGame()
{
    isPaused = true;
    for (Character* c : characters) c->isPaused = true;

    // getOpenFileName完整文件路径到filename
    QString filename = QFileDialog::getOpenFileName(this, tr("加载游戏"), QDir::currentPath(), tr("连连看存档 (*.lksav)"));

    if (!filename.isEmpty()) {
        qDebug() << "Selected file path:" << filename;
        QFileInfo fileInfo(filename);
        qDebug() << "Absolute path:" << fileInfo.absoluteFilePath();
        qDebug() << "Exists:" << fileInfo.exists();
    }

    if (!filename.isEmpty()) {
        // 调用现有的加载方法，并检查返回值
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

// Game Over弹窗
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
