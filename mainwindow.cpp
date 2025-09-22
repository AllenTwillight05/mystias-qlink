#include "mainwindow.h"
#include "character.h"
#include "map.h"
#include "score.h"
#include "box.h"

#include <QTimer>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>       // 菜单栏
#include <QMenu>          // 菜单
#include <QAction>        // 菜单动作
#include <QFileDialog>    // 文件对话框
#include <QMessageBox>    // 消息框

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    scene(nullptr),
    view(nullptr),
    gameMap(nullptr),
    countdownTimer(new QTimer(this)),
    isPaused(false),
    saveManager(this)
{
    setupScene();
    createMenu();

    // 初始化地图
    gameMap = new Map(yNum, xNum, typeNum, ":/assets/ingredient.png", scene, 26);
    gameMap->addToScene();

    // === 创建角色1（WASD 控制） ===
    Character* character1 = new Character(":/assets/sprites0.png", this);
    character1->setPos(mapWidth/5, mapHeight/5);
    character1->setControls({ Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D });    //Qt::Key_W等本质是int
    character1->setGameMap(gameMap);
    scene->addItem(character1);
    connect(character1, &Character::collidedWithBox, this, &MainWindow::handleActivation);
    characters.append(character1);
    //Score* score1 = new Score(character1);

    // === 创建角色2（IJKL控制） ===
    Character* character2 = new Character(":/assets/sprites1.png", this);
    character2->setPos(mapWidth/5*4, mapHeight/5*4);
    character2->setControls({ Qt::Key_I, Qt::Key_K, Qt::Key_J, Qt::Key_L });
    character2->setGameMap(gameMap);
    scene->addItem(character2);
    connect(character2, &Character::collidedWithBox, this, &MainWindow::handleActivation);
    characters.append(character2);


    // 倒计时
    countdownTime = initialCountdownTime;
    countdownText = scene->addText(QString("Time：%1").arg(countdownTime));
    countdownText->setDefaultTextColor(QColorConstants::Svg::saddlebrown);
    countdownText->setFont(QFont("Consolas", 20, QFont::Bold));
    countdownText->setZValue(100);
    countdownText->setPos(20, 20);

    connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    countdownTimer->start(1000);

    // 单人模式右上角分数，现已弃用
    // score = new Score();
    // scene->addItem(score);
    // score->setPos(mapWidth - 160, 20);
    // score->setDefaultTextColor(QColorConstants::Svg::saddlebrown);
    // score->setFont(QFont("Consolas", 20, QFont::Bold));

    // 连接存档管理器的错误信号
    // 注意saveManager是对象实例，所以connect中需要取地址
    connect(&saveManager, &SaveGameManager::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("存档错误"), message);
    });

    setWindowTitle("Mystia’s Ingredient Quest");
    resize(1200, 675);
}

MainWindow::~MainWindow() {}

void MainWindow::setupScene()
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    // scene->setItemIndexMethod(QGraphicsScene::NoIndex); // 禁用BSP树，使用堆叠顺序
    scene->setSceneRect(0, 0, mapWidth, mapHeight);
    scene->setBackgroundBrush(QColorConstants::Svg::antiquewhite);

    QPixmap bgPixmap(":/assets/background.jpg");
    if (!bgPixmap.isNull()) {
        QGraphicsPixmapItem *bgItem = scene->addPixmap(bgPixmap);
        bgItem->setZValue(-100);
    }

    setCentralWidget(view);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 添加暂停快捷键（空格键）
    if (event->key() == Qt::Key_Space) {
        togglePause();
        return;  // 避免传递给角色
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
    countdownText->setPlainText(QString("Time：%1").arg(countdownTime));
}

void MainWindow::togglePause(){
    isPaused = !isPaused;
    if(isPaused){
        for(Character* c: characters){
            c->isPaused = true;
        }
    }else{
        for(Character* c: characters){
            c->isPaused = false;
        }
    }
}

void MainWindow::handleActivation(Box* box, Character* sender)
{
    if (!box) return;

    if (!lastActivatedBox) {
        lastActivatedBox = box;
        box->activate();
        return;
    }

    if (box == lastActivatedBox) return;

    if (gameMap->canConnect(lastActivatedBox, box)) {
        // 消除逻辑
        lastActivatedBox->deactivate();
        box->deactivate();
        gameMap->m_map[lastActivatedBox->row][lastActivatedBox->col] = -1;
        gameMap->m_map[box->row][box->col] = -1;
        gameMap->getScene()->removeItem(lastActivatedBox);
        gameMap->getScene()->removeItem(box);
        gameMap->m_boxes.removeOne(lastActivatedBox);
        gameMap->m_boxes.removeOne(box);
        lastActivatedBox = nullptr;

        sender->getCharacterScore()->increase(10); // 调用Character内部的分数对象

        // 绘制路径
        const QVector<QPointF>& pts = gameMap->m_pathPixels;
        if (pts.size() >= 2) {
            QPainterPath path(pts[0]);
            for (int i = 1; i < pts.size(); ++i) {
                path.lineTo(pts[i]);
            }
            QPen pen(QColor(255, 223, 128), 10);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCapStyle(Qt::RoundCap);

            QGraphicsPathItem* lineItem =
                gameMap->getScene()->addPath(path, pen);
            lineItem->setZValue(-1);

            QTimer::singleShot(500, [scene = gameMap->getScene(), lineItem]() {
                scene->removeItem(lineItem);
                delete lineItem;
            });
        }
    } else {
        lastActivatedBox->deactivate();
        lastActivatedBox = box;
        box->activate();
    }

    if (!gameMap->isSolvable()) {
        showGameOverDialog();
    }
}


// 创建菜单
void MainWindow::createMenu()
{
    QMenu *gameMenu = menuBar()->addMenu(tr("选项"));

    // 保存游戏动作
    QAction *saveAction = new QAction(tr("保存游戏"), this);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveGame);
    gameMenu->addAction(saveAction);

    // 加载游戏动作
    QAction *loadAction = new QAction(tr("加载游戏"), this);
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoadGame);
    gameMenu->addAction(loadAction);

    // 添加分隔线
    gameMenu->addSeparator();

    // 暂停
    QAction *togglePause = new QAction(tr("暂停/继续"), this);
    connect(togglePause, &QAction::triggered, this, &MainWindow::togglePause);
    gameMenu->addAction(togglePause);

    gameMenu->addSeparator();

    // 退出游戏动作
    QAction *exitAction = new QAction(tr("退出"), this);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    gameMenu->addAction(exitAction);
}

// 保存游戏
void MainWindow::onSaveGame()
{
    //先暂停
    isPaused = true;
    for(Character* c: characters){
        c->isPaused = true;
    }

    //安全检查
    if (characters.isEmpty()) {
        QMessageBox::warning(this, tr("保存游戏"), tr("没有可用的角色"));  //warning,系统警告对话框
        return;
    }

    //获取文件名(文件对话框，输入保存位置和文件名)
    QString filename = QFileDialog::getSaveFileName(this,                        //父窗口
                                                    tr("保存游戏"),               //tr()国际化翻译函数，保持和系统语言一致，但暂时未编写.ts翻译文件；此行为对话框标题
                                                    QDir::currentPath(),         //默认目录
                                                    tr("连连看存档 (*.lksav)"));  //文件过滤器，格式：描述 (*.扩展名)
    //写入文件
    if (!filename.isEmpty()) {
        //文件名自动补齐
        if (!filename.endsWith(".lksav")) {
            filename += ".lksav";
        }

        // 直接传递角色列表
        if (saveManager.saveGame(filename, *gameMap, characters, countdownTime)) {
            QMessageBox::information(this, tr("保存游戏"), tr("游戏已成功保存!")); //information,系统信息对话框
        }
    }

    //解除暂停
    isPaused = false;
    for(Character* c: characters){
        c->isPaused = false;
    }
}

// 加载游戏
void MainWindow::onLoadGame()
{
    isPaused = true;
    for(Character* c: characters){
        c->isPaused = true;
    }

    if (characters.isEmpty()) {
        QMessageBox::warning(this, tr("加载游戏"), tr("没有可用的角色"));
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("加载游戏"),
                                                    QDir::currentPath(),
                                                    tr("连连看存档 (*.lksav)"));
    if (!filename.isEmpty()) {
        // 直接传递角色列表
        if (saveManager.loadGame(filename, *gameMap, characters, countdownTime)) {
            QMessageBox::information(this, tr("加载游戏"), tr("游戏已成功加载!"));

            // 更新倒计时显示
            countdownText->setPlainText(QString("Time：%1").arg(countdownTime));

            // 更新每个角色的分数显示
            for (Character* character : characters) {
                character->getCharacterScore()->updateText();
            }
        }
    }

    isPaused = false;
    for(Character* c: characters){
        c->isPaused = false;
    }
}


void MainWindow::showGameOverDialog() {
    for (Character* c : characters) {
        c->stopTimers();
    }
    countdownTimer->stop();

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

void MainWindow::resetToTitleScreen() {
    if (gameMap) {
        delete gameMap;
        gameMap = nullptr;
    }

    for (Character* c : characters) {
        scene->removeItem(c);
        delete c;
    }
    characters.clear();

    scene->clear();

    QGraphicsTextItem* titleText = scene->addText("Is the Order an Anago?");
    titleText->setDefaultTextColor(Qt::black);
    titleText->setScale(2.0);
    titleText->setPos(200, 150);
}
