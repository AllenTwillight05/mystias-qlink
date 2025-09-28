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
    singleBtn(new QPushButton(tr("单人游戏"), this)),
    multiBtn(new QPushButton(tr("多人游戏"), this)),
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
        "  background-color: rgba(25, 25, 25, 60);"  // 半透明白
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(25, 25, 25, 100);"
        "}"
        );

    multiBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 24px;"
        "  padding: 12px 30px;"
        "  border-radius: 12px;"
        "  background-color: rgba(25, 25, 25, 60);"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(25, 25, 25, 100);"
        "}"
        );


    // 布局：垂直居中
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(singleBtn, 0, Qt::AlignHCenter);
    layout->addSpacing(12);
    layout->addWidget(multiBtn, 0, Qt::AlignHCenter);
    layout->addStretch();
    layout->setContentsMargins(60, 40, 60, 60);
    setLayout(layout);

    connect(singleBtn, &QPushButton::clicked, this, &StartMenu::startSinglePlayer);
    connect(multiBtn, &QPushButton::clicked, this, &StartMenu::startMultiPlayer);

    // 如果背景资源不存在，打印提示
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
