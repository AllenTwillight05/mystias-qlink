#include "savegamemanager.h"
#include "map.h"
#include "character.h"
#include "score.h"
#include <QFile>
#include <QDataStream>
#include <QMessageBox>

// 在cpp文件中定义静态常量
const quint32 SaveGameManager::SAVE_FILE_MAGIC = 0x4C4B5341; // "LKSA", QLinkSaveArchive
const qint32 SaveGameManager::SAVE_FILE_VERSION = 1;

SaveGameManager::SaveGameManager(QObject* parent) : QObject(parent) {}

bool SaveGameManager::saveGame(const QString &filename,
                               Map &gameMap,
                               Character &character,
                               Score &score,
                               int countdownTime)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(tr("无法创建存档文件: %1").arg(file.errorString()));
        return false;
    }

    GameSaveData saveData;
    saveData.mapData = gameMap.getMapData();
    saveData.characterPos = character.getPosition();
    saveData.score = score.getScore();
    saveData.countdownTime = countdownTime;

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);

    out << SAVE_FILE_MAGIC;
    out << SAVE_FILE_VERSION;
    out << saveData; // 使用重载的运算符

    file.close();
    return true;
}

bool SaveGameManager::loadGame(const QString &filename,
                               Map &gameMap,
                               Character &character,
                               Score &score,
                               int& countdownTime)
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
    if (magic != SAVE_FILE_MAGIC) {
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

    GameSaveData saveData;
    in >> saveData; // 使用重载的运算符

    file.close();

    gameMap.setMapData(saveData.mapData);
    character.setPosition(saveData.characterPos);
    score.setScore(saveData.score);
    countdownTime = saveData.countdownTime;

    return true;
}
