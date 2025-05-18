#include "servermonitor.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QThread>

ServerMonitor::ServerMonitor(QObject *parent)
    : QObject(parent),
    m_timer(nullptr),
    m_manager(nullptr)
{
    connect(this, &ServerMonitor::cleanTimers, this, &ServerMonitor::deleteTimer);
}

ServerMonitor::~ServerMonitor()
{
    // 如果当前析构不在 Alarm 所在线程，就先同步调用 stop()
    if (thread() && thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(
            this,
            "stop",
            Qt::BlockingQueuedConnection
            );
    } else {
        // 如果已经在正确线程，直接停止并删除定时器
        if (m_timer) {
            emit cleanTimers();
            // m_timer->disconnect(this);
            delete m_timer;
            m_timer = nullptr;
        }
        if (m_manager) {
            m_manager->deleteLater();
            m_manager = nullptr;
        }
    }

    // QObject 父子机制会自动删除子对象，无需再手动 delete
    // 但为了安全，确保指针置空
    m_timer = nullptr;
    stop();
}

void ServerMonitor::start()
{
    if(!m_timer) {
        m_timer = new QTimer(this);
        m_manager = new QNetworkAccessManager(this);
        connect(m_timer, &QTimer::timeout, this, &ServerMonitor::fetch);
        connect(m_manager, &QNetworkAccessManager::finished, this, &ServerMonitor::onReplyFinished);
    }
    if(!m_timer->isActive())
        m_timer->start(5000);
    fetch();
}

void ServerMonitor::stop()
{
    if (m_timer->isActive())
        m_timer->stop();
}

void ServerMonitor::fetch()
{
    QNetworkRequest req(QUrl("https://cdn2.arkdedicated.com/servers/asa/officialserverlist.json"));
    m_manager->get(req);
}

void ServerMonitor::onReplyFinished(QNetworkReply *reply)
{
    QList<ServerInfo> list;
    if (!reply->error()) {
        auto json = QJsonDocument::fromJson(reply->readAll());
        if (json.isArray()) {
            for (auto v : json.array()) {
                auto obj = v.toObject();
                ServerInfo info;
                info.sessionName = obj.value("SessionName").toString();
                info.numPlayers  = obj.value("NumPlayers").toInt();
                list.append(info);
            }
        }
    }
    reply->deleteLater();
    emit serversFetched(list);
}

void ServerMonitor::deleteTimer()
{
    if (m_timer->isActive())
        m_timer->stop();
}
