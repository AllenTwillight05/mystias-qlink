#pragma once
#include <QWidget>

class QPushButton;
class QLabel;
class QPixmap;

class StartMenu : public QWidget
{
    Q_OBJECT
public:
    explicit StartMenu(QWidget *parent = nullptr);

    // 获取配置参数
    int getYNum() const { return m_yNum; }
    int getXNum() const { return m_xNum; }
    int getTypeNum() const { return m_typeNum; }
    int getInitialCountdownTime() const { return m_initialCountdownTime; }

signals:
    void startSinglePlayer();
    void startMultiPlayer();
    void configRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onConfigClicked();  // 配置按钮点击槽函数

private:
    QLabel *backgroundLabel;
    QPushButton *singleBtn;
    QPushButton *multiBtn;
    QPushButton *configBtn;
    QPixmap bgPixmap;

    // 配置参数
    int m_yNum = 4;
    int m_xNum = 6;
    int m_typeNum = 4;
    int m_initialCountdownTime = 120;
};
