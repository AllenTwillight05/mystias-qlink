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
    // 保存游戏
    bool saveGame(const QString &filename, Map &gameMap, Character &character, Score &score, int countdownTime);
    // 加载游戏
    bool loadGame(const QString &filename, Map &gameMap, Character &character, Score &score, int& countdownTime);

signals:
    void errorOccurred(const QString &message);

private:
    // 文件标识和版本常量
    static const quint32 SAVE_FILE_MAGIC;   // 文件魔数
    static const qint32 SAVE_FILE_VERSION;  // 版本号

    struct GameSaveData
    {
        QVector<QVector<int>> mapData;
        QPointF characterPos;
        int score;
        int countdownTime;

        // 序列化操作，重载入和出运算符
        friend QDataStream &operator<<(QDataStream &out, const GameSaveData &data) {
            out << data.mapData;
            out << data.characterPos;
            out << data.score;
            out << data.countdownTime;
            return out;
        }

        friend QDataStream &operator>>(QDataStream &in, GameSaveData &data) {
            in >> data.mapData;
            in >> data.characterPos;
            in >> data.score;
            in >> data.countdownTime;
            return in;
        }
    };
};


