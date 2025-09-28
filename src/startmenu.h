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

signals:
    void startSinglePlayer();
    void startMultiPlayer();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *backgroundLabel;
    QPushButton *singleBtn;
    QPushButton *multiBtn;
    QPixmap bgPixmap;
};
