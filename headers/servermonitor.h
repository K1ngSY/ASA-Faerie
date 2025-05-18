#ifndef SERVERMONITOR_H
#define SERVERMONITOR_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// 用于承载单个服务器信息
struct ServerInfo {
    QString sessionName;
    int numPlayers;
};
Q_DECLARE_METATYPE(QList<ServerInfo>)

class ServerMonitor : public QObject {
    Q_OBJECT
public:
    explicit ServerMonitor(QObject *parent = nullptr);
    ~ServerMonitor();

public slots:
    // intervalMs: 拉取间隔，单位毫秒
    void start();
    void stop();

signals:
    // 每次拉取到完整列表后发出
    void serversFetched(const QList<ServerInfo> &servers);
    void cleanTimers();

private slots:
    void fetch();
    void onReplyFinished(QNetworkReply *reply);
    void deleteTimer();

private:
    QTimer                *m_timer;
    QNetworkAccessManager *m_manager;
};

#endif // SERVERMONITOR_H
