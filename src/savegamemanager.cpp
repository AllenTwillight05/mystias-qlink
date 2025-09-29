#include "savegamemanager.h"
#include "map.h"
#include "character.h"
#include "score.h"
#include <QFile>
#include <QDataStream>
#include <QMessageBox>

SaveGameManager::SaveGameManager(QObject* parent) : QObject(parent) {}

// savegamemanager.cpp
bool SaveGameManager::saveGame(const QString &filename,
                               Map &gameMap,
                               QVector<Character*> &characters, // 改为接收角色列表
                               int countdownTime)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(tr("无法创建存档文件: %1").arg(file.errorString()));
        return false;
    }

    GameSaveData saveData;
    saveData.mapData = gameMap.getMapData();

    // 保存所有角色的位置和分数
    for (Character* character : characters) {
        saveData.characterPositions.append(character->getPosition());
        saveData.scores.append(character->getCharacterScore()->getScore());
    }

    saveData.countdownTime = countdownTime;

    QDataStream out(&file); //类似std::ofstream out("file.txt");out <<...;
    out.setVersion(QDataStream::Qt_5_15);

    out << QLINK_FILE_SIGNATURE;
    out << SAVE_FILE_VERSION;
    out << saveData;    //已经重载过<<运算符，故不用把saveData.mapData等一行行拆开写    //由于返回了重载时返回了out，这里也可以三行并一行写

    file.close();
    return true;
}

bool SaveGameManager::loadGame(const QString &filename,
                               Map &gameMap,
                               QVector<Character*> &characters,
                               int &countdownTime)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("无法打开存档文件: %1").arg(file.errorString()));
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);

    quint32 magic;
    in >> magic;
    if (magic != QLINK_FILE_SIGNATURE) {
        emit errorOccurred(tr("这不是有效的连连看存档文件"));
        file.close();
        return false;
    }

    qint32 version;
    in >> version;
    if (version != SAVE_FILE_VERSION) {
        emit errorOccurred(tr("不支持的存档版本: %1").arg(version));
        file.close();
        return false;
    }

    // 直接读取存档数据
    QVector<QVector<int>> mapData;
    QVector<QPointF> characterPositions;
    QVector<int> scores;
    int tempCountdownTime;

    in >> mapData;
    in >> characterPositions;
    in >> scores;
    in >> tempCountdownTime;

    file.close();

    // 检查角色数量是否匹配
    if (characters.size() != characterPositions.size()) {
        QString currentMode = (characters.size() == 1) ? tr("单人游戏") : tr("双人游戏");
        QString savedMode = (characterPositions.size() == 1) ? tr("单人游戏") : tr("双人游戏");

        emit errorOccurred(
            tr("存档模式不匹配！\n\n当前游戏模式：%1\n存档游戏模式：%2\n\n请开始%3游戏后再加载此存档。")
                .arg(currentMode)
                .arg(savedMode)
                .arg(savedMode)
            );
        return false;
    }

    // 恢复地图
    gameMap.setMapData(mapData);

    // 恢复角色状态
    for (int i = 0; i < characters.size(); ++i) {
        characters[i]->setPosition(characterPositions[i]);
        characters[i]->getCharacterScore()->setScore(scores[i]);
    }

    // 恢复倒计时
    countdownTime = tempCountdownTime;

    return true;
}
