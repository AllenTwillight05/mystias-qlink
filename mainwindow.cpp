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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    scene(nullptr),
    view(nullptr),
    gameMap(nullptr),
    countdownTimer(new QTimer(this)),
    isPaused(false)
{
    setupScene();

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

    // 分数
    score = new Score();
    scene->addItem(score);
    score->setPos(mapWidth - 160, 20);
    score->setDefaultTextColor(QColorConstants::Svg::saddlebrown);
    score->setFont(QFont("Consolas", 20, QFont::Bold));

    setWindowTitle("Mystia’s Ingredient Quest");
    resize(1200, 675);
}

MainWindow::~MainWindow() {}

void MainWindow::setupScene()
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
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

void MainWindow::handleActivation(Box* box)
{
    if (!box) return;

    // 保留你原来的逻辑
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

        score->increase(10);

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
            lineItem->setZValue(0);

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
