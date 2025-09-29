#include "startmenu.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QFile>
#include <QDebug>

StartMenu::StartMenu(QWidget *parent)
    : QWidget(parent),
    backgroundLabel(new QLabel(this)),
    singleBtn(new QPushButton(tr("独自采集"), this)),
    multiBtn(new QPushButton(tr("招募伙伴"), this)),
    bgPixmap(":/assets/start_menu_bg.png")
{
    // 背景 label 在底层
    backgroundLabel->setScaledContents(true);
    backgroundLabel->lower();

    // 简单美化按钮
    singleBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 24px;"
        "  padding: 12px 30px;"
        "  border-radius: 12px;"
        "  background-color: rgba(50, 50, 50, 60);"  // 半透明白
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(125, 125, 125, 100);"
        "}"
        );

    multiBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 24px;"
        "  padding: 12px 30px;"
        "  border-radius: 12px;"
        "  background-color: rgba(50, 50, 50, 60);"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(125, 125, 125, 100);"
        "}"
        );

    // 创建垂直布局放置按钮
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(singleBtn, 0, Qt::AlignLeft);  // 左对齐
    buttonLayout->addSpacing(24);
    buttonLayout->addWidget(multiBtn, 0, Qt::AlignLeft);   // 左对齐
    buttonLayout->addStretch();
    buttonLayout->setContentsMargins(60, 40, 60, 60);


    // 使用网格布局，更精确控制列宽
    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setColumnStretch(0, 1); // 第一列占1份
    mainLayout->setColumnStretch(1, 1); // 第二列占1份
    mainLayout->setColumnStretch(2, 5); // 第三列占5份

    QWidget *leftContainer = new QWidget(this);
    leftContainer->setLayout(buttonLayout);

    mainLayout->addWidget(leftContainer, 0, 1);
    // 第1列留空，占第二列

    setLayout(mainLayout);

    setLayout(mainLayout);

    connect(singleBtn, &QPushButton::clicked, this, &StartMenu::startSinglePlayer);
    connect(multiBtn, &QPushButton::clicked, this, &StartMenu::startMultiPlayer);

    if (bgPixmap.isNull()) {
        qWarning() << "StartMenu: 背景图片未找到: :/assets/start_menu_bg.png";
    }
}

void StartMenu::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!bgPixmap.isNull()) {
        // 等比例放大并覆盖（会裁剪超出的部分），保证填满窗口
        QPixmap scaled = bgPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        backgroundLabel->setPixmap(scaled);
        backgroundLabel->resize(size());
    } else {
        backgroundLabel->clear();
    }
}
