#include "savegamemanager.h"
#include "map.h"
#include "character.h"
#include "score.h"
#include <QFile>
#include <QDataStream>
#include <QMessageBox>

SaveGameManager::SaveGameManager(QObject* parent) : QObject(parent) {}

// 存档逻辑，传入存档文件名、地图指针、角色列表、剩余时间，存储成功则返回true
bool SaveGameManager::saveGame(const QString &filename,
                               Map &gameMap,
                               QVector<Character*> &characters,
                               int countdownTime)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(tr("无法创建存档文件: %1").arg(file.errorString()));
        return false;
    }

    // 结构体，包含二维int数组mapData（序号+空）、QPointF数组characterPositions、分数数组scores和剩余时间countdownTime
    GameSaveData saveData;
    saveData.mapData = gameMap.getMapData();

    // 保存所有角色的位置和分数
    for (Character* character : characters) {
        saveData.characterPositions.append(character->getPosition());
        saveData.scores.append(character->getCharacterScore()->getScore());
        // getCharacterScore()返回的是character类对象的成员：score指针，故需要再调用score类的getScore()返回分数值
    }

    saveData.countdownTime = countdownTime;

    QDataStream out(&file); //类似std::ofstream out("file.txt");  out <<...;
    out.setVersion(QDataStream::Qt_5_15);

    // 写入
    out << QLINK_FILE_SIGNATURE;    // 文件魔数
    out << SAVE_FILE_VERSION;       // 版本
    out << saveData;    // 已经重载过<<运算符，故不用把saveData.mapData等一行行拆开写    //由于返回了重载时返回了out，这里也可以三行并一行写

    file.close();
    return true;
}

// 存档逻辑，传入文件名、地图指针、角色列表、剩余时间，读档成功则返回true
bool SaveGameManager::loadGame(const QString &filename,
                               Map &gameMap,
                               QVector<Character*> &characters,
                               int &countdownTime)
{
    // 以只读模式打开
    QFile file(filename);   // 创建文件QFile类对象file，关联到传入的filename
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("无法打开存档文件: %1").arg(file.errorString())); //emit发射错误信号
        return false;
    }

    QDataStream in(&file);  // 创建二进制数据流QDataStream类对象in，关联到文件对象file
    in.setVersion(QDataStream::Qt_5_15);

    // 读取文件魔数和版本
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

    // 以结构体形式读取存档数据
    GameSaveData saveData;
    in >> saveData;

    file.close();

    // 检查角色数量是否匹配
    if (characters.size() != saveData.characterPositions.size()) {
        QString currentMode = (characters.size() == 1) ? tr("单人游戏") : tr("双人游戏");
        QString savedMode = (saveData.characterPositions.size() == 1) ? tr("单人游戏") : tr("双人游戏");

        emit errorOccurred(
            tr("存档模式不匹配！\n\n当前游戏模式：%1\n存档游戏模式：%2\n\n请开始%3游戏后再加载此存档。")
                .arg(currentMode)
                .arg(savedMode)
                .arg(savedMode)
            );
        return false;
    }

    // 恢复地图
    gameMap.setMapData(saveData.mapData);

    // 恢复角色状态
    for (int i = 0; i < characters.size(); ++i) {
        characters[i]->setPosition(saveData.characterPositions[i]);
        characters[i]->getCharacterScore()->setScore(saveData.scores[i]);
    }

    // 恢复倒计时
    countdownTime = saveData.countdownTime;

    return true;
}
