#include "expirationdetector.h"
#include <QDebug>


ExpirationDetector::ExpirationDetector(QObject *parent, QDateTime expDT) :
    QObject{parent},
    m_expDateTime(expDT),
    m_expireTimer(nullptr)
{}

void ExpirationDetector::start()
{
    if (!m_expireTimer) {
        m_expireTimer = new QTimer(this);
        connect(m_expireTimer, &QTimer::timeout, this, &ExpirationDetector::checkExpiration);
    }
    if (!m_expireTimer->isActive()) {
        m_expireTimer->start(5000);
        // emit logMessage("ExpirationDetector: 过期检查器启动！");
        qDebug() << "ExpirationDetector: 过期检查器启动！";
    } else {
        // emit logMessage("ExpirationDetector: 过期检查器已在运行不要重复启动！");
        qDebug() << "ExpirationDetector: 过期检查器已在运行不要重复启动！";
    }
}

void ExpirationDetector::stop()
{
    m_expireTimer->stop();
    emit logMessage("ExpirationDetector: 已暂停Timer");
}

void ExpirationDetector::setExpDateTime(QDateTime expDT)
{
    m_expDateTime = expDT;
}

ExpirationDetector::~ExpirationDetector()
{
    if(m_expireTimer->isActive()) {
        m_expireTimer->stop();
    }
    m_expireTimer->deleteLater();
}

void ExpirationDetector::checkExpiration()
{
    QDateTime now = QDateTime::currentDateTime();
    if (!m_expDateTime.isValid()) {
        return;
    }
    if (now >= m_expDateTime) {
        emit logMessage("检测到卡密过期！即将退出程序");
        qDebug() << "检测到卡密过期！";
        emit expirated();
        // 停掉定时器
        if (m_expireTimer && m_expireTimer->isActive())    m_expireTimer->stop();
    }
}
