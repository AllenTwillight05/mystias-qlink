#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QPoint>

class Map;
class Character;
class Score;

class SaveGameManager : public QObject
{
    Q_OBJECT
public:
    explicit SaveGameManager(QObject *parent = nullptr);

    bool saveGame(const QString &filename,
                  Map &gameMap,
                  QVector<Character*> &characters, // 改为接收角色列表
                  int countdownTime);

    bool loadGame(const QString &filename,
                  Map &gameMap,
                  QVector<Character*> &characters, // 改为接收角色列表
                  int &countdownTime);

signals:
    void errorOccurred(const QString &message);

private:
    // 文件标识和版本常量
    static const quint32 QLINK_FILE_SIGNATURE = 0x514C5341; // "QLSA",QLinkSaveArchive   // 文件魔数
    static const qint32 SAVE_FILE_VERSION = 1;  // 版本号

    struct GameSaveData
    {
        QVector<QVector<int>> mapData;
        QVector<QPointF> characterPositions;  // 所有角色的位置
        QVector<int> scores;                  // 所有角色的分数;
        int countdownTime;

        // 序列化操作，重载入和出运算符
        friend QDataStream &operator<<(QDataStream &out, const GameSaveData &data) {    //QDataStream类似iostream但处理Qt对象，二进制格式
            out << data.mapData;
            out << data.characterPositions;
            out << data.scores;
            out << data.countdownTime;
            return out;     //返回流对象，用于链式操作
        }

        friend QDataStream &operator>>(QDataStream &in, GameSaveData &data) {
            in >> data.mapData;
            in >> data.characterPositions;
            in >> data.scores;
            in >> data.countdownTime;
            return in;
        }
    };
};


