//
// Created by pixiu on 2025/8/22.
//


#include "history.h"
#include "config.h"

#define FILE_NAME "history.json"
#define HISTORY_KEY "histories"
#define HISTORY_MAX 5

History::History(QObject *parent) : QObject(parent)
{

}

History &History::getInstance()
{
    static History history;
    return history;
}

void History::loadHistoryAddress()
{
    QString fileName = FILE_NAME;
    QString filePath = Config::getInstance().getConfigPath() + "/" + fileName;

    if (!QFile::exists(filePath)) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "历史记录文件创建失败：" << file.errorString();
            return;
        }
        QJsonObject emptyObj;
        QJsonArray emptyArray;
        emptyObj.insert(HISTORY_KEY, emptyArray);
        file.write(QJsonDocument(emptyObj).toJson());
        file.close();
        return;
    }
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning() << "历史记录文件打开失败：" << loadFile.errorString();
        return;
    }

    QString json = loadFile.readAll();
    loadFile.close();

    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json.toUtf8(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        qWarning() << QString("json error: %1").arg(jsonError.errorString());
        return;
    }
    if (!jsonDoc.isObject()) {
        qDebug() << "JSON 文档根节点不是对象！";
        return;
    }
    QJsonObject rootObj = jsonDoc.object();
    if (!rootObj.contains(HISTORY_KEY) || !rootObj[HISTORY_KEY].isArray()) {
        qDebug() << HISTORY_KEY << " 不存在或非数组类型！";
        return;
    }
    QJsonArray historiesArray = rootObj[HISTORY_KEY].toArray();

    m_histories.clear();
    for (const QJsonValue& value : historiesArray) {
        if (value.isObject()) {
            QJsonObject addrObj = value.toObject();
            m_histories.append(Address::fromJson(addrObj));
        }
    }
}

void History::appendHistory(const QString &ip, const int &port)
{
    Address addr;
    addr.ip = ip;
    addr.port = port;

    if (m_histories.size()>= HISTORY_MAX) {
        // 如果超过，删除最旧的一条
        m_histories.removeLast();
    }

    m_histories.prepend(addr); // 插入到最前面

    // 保存到文件
    QString fileName = FILE_NAME;
    QString filePath = Config::getInstance().getConfigPath() + "/" + fileName;

    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "历史记录文件保存失败：" << saveFile.errorString();
        return;
    }

    QJsonObject rootObj;
    QJsonArray historiesArray;

    for (const Address& history : m_histories) {
        QJsonObject addrObj;
        addrObj.insert("ip", history.ip);
        addrObj.insert("port", history.port);
        historiesArray.append(addrObj);
    }

    rootObj.insert(HISTORY_KEY, historiesArray);

    saveFile.write(QJsonDocument(rootObj).toJson());
    saveFile.close();
}

QList<History::Address> History::getHistories() {
    return m_histories;
}
