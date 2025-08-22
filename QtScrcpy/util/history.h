#ifndef ALL_HISTORY_H
#define ALL_HISTORY_H

#include <QObject>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

class History : public QObject{
    Q_OBJECT

public:
    struct Address {
        QString ip;
        int port;
        static Address fromJson(const QJsonObject& obj) {
            Address addr;
            addr.ip = obj.value("ip").toString();
            addr.port = obj.value("port").toInt();
            return addr;
        }
    };

public:
    static History &getInstance();
    void loadHistoryAddress();
    QList<History::Address> getHistories();
    void appendHistory(const QString &ip, const int &port);

private:
    explicit History(QObject *parent = nullptr);

private:
    QList<Address> m_histories;

};

#endif //ALL_HISTORY_H
