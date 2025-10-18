#include "startmenu.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

// 构造函数
StartMenu::StartMenu(QWidget *parent)
    : QWidget(parent),
    backgroundLabel(new QLabel(this)),
    singleBtn(new QPushButton(tr("独自采集"), this)),
    multiBtn(new QPushButton(tr("招募伙伴"), this)),
    configBtn(new QPushButton(tr("食材配置"), this)),
    bgPixmap(":/assets/start_menu_bg.png")
{
    // 背景 label 在底层
    backgroundLabel->setScaledContents(true);
    backgroundLabel->lower();

    // 设置按钮样式
    QString buttonStyle =
        "QPushButton {"
        "  font-size: 24px;"
        "  padding: 12px 30px;"
        "  border-radius: 12px;"
        "  background-color: rgba(50, 50, 50, 60);"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(125, 125, 125, 100);"
        "}";

    singleBtn->setStyleSheet(buttonStyle);
    multiBtn->setStyleSheet(buttonStyle);
    configBtn->setStyleSheet(buttonStyle);

    // 创建垂直布局放置按钮
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(singleBtn, 0, Qt::AlignLeft);  // 左对齐
    buttonLayout->addSpacing(24);
    buttonLayout->addWidget(multiBtn, 0, Qt::AlignLeft);
    buttonLayout->addSpacing(24);
    buttonLayout->addWidget(configBtn, 0, Qt::AlignLeft);
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
    connect(configBtn, &QPushButton::clicked, this, &StartMenu::onConfigClicked);

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

void StartMenu::onConfigClicked()
{
    // 显示配置对话框
    bool ok;

    // 设置行数
    int yNum = QInputDialog::getInt(this, tr("配置"),
                                    tr("行数 (yNum):"), m_yNum, 2, 10, 1, &ok);
    if (!ok) return;

    // 设置列数
    int xNum = QInputDialog::getInt(this, tr("配置"),
                                    tr("列数 (xNum):"), m_xNum, 3, 15, 1, &ok);
    if (!ok) return;

    // 设置类型数
    int typeNum = QInputDialog::getInt(this, tr("配置"),
                                       tr("食材类型数:"), m_typeNum, 2, 8, 1, &ok);
    if (!ok) return;

    // 验证参数合理性（行数×列数应该是偶数，因为连连看需要成对消除）
    if ((yNum * xNum) % 2 != 0) {
        QMessageBox::warning(this, tr("配置错误"),
                             tr("行数 × 列数必须是偶数，当前为 %1").arg(yNum * xNum));
        return;
    }

    // 验证类型数是否足够
    if (typeNum * 2 > yNum * xNum) {
        QMessageBox::warning(this, tr("配置错误"),
                             tr("类型数过多，无法生成有效地图"));
        return;
    }

    // 更新配置
    m_yNum = yNum;
    m_xNum = xNum;
    m_typeNum = typeNum;

    QMessageBox::information(this, tr("配置成功"),
                             tr("地图配置已更新:\n行数: %1\n列数: %2\n类型数: %3")
                                 .arg(m_yNum).arg(m_xNum).arg(m_typeNum));

    // 发射配置请求信号（如果需要通知MainWindow）
    emit configRequested();
}
