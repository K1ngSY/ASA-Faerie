#include "expirationdetector.h"
#include <QDebug>


ExpirationDetector::ExpirationDetector(QObject *parent, QDateTime expDT) :
    QObject{parent},
    m_expDateTime(expDT),
    m_expireTimer(nullptr)
{}

void ExpirationDetector::start()
{
    m_expireTimer = new QTimer(this);
    connect(m_expireTimer, &QTimer::timeout, this, &ExpirationDetector::checkExpiration);
    m_expireTimer->start(5000);
    qDebug() << "过期检查器启动！";
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

    delete m_expireTimer;
}

void ExpirationDetector::checkExpiration()
{
    QDateTime now = QDateTime::currentDateTime();
    if (!m_expDateTime.isValid()) {
        return;
    }
    if (now >= m_expDateTime) {
        qDebug() << "检测到卡密过期！";
        emit expirated();
        // 停掉定时器
        if (m_expireTimer)    m_expireTimer->stop();
    }
}
