#include "rejoinprocessor.h"
#include "motionsimulator.h"
#include <QDebug>
#include <QThread>
#include <windows.h>

RejoinProcessor::RejoinProcessor(QObject *parent, HWND gameHwnd)
    : QObject{parent}, m_gameHwnd(gameHwnd), m_serverCode("")
{}

void RejoinProcessor::setHwnd(HWND gameHwnd)
{
    m_gameHwnd = gameHwnd;
}

void RejoinProcessor::setServerCode(QString serverCode)
{
    m_serverCode = serverCode;
    emit logMessage(QString("已设置服务器代码为 %1").arg(m_serverCode));
}

void RejoinProcessor::startRejoin()
{
    if (!m_gameHwnd) {
        emit logMessage("rejoin: HWND is null");
        emit finished_1();
        return;
    }
    RECT rect;
    if (!GetWindowRect(m_gameHwnd, &rect)) {
        emit logMessage("rejoin: Failed to get window rect");
        emit finished_1();
        return;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    // 找到后执行点击
    MotionSimulator::LeftClick(m_gameHwnd, static_cast<int>(width / 2), static_cast<int>(height * 43 / 54));
    emit logMessage("rejoin: 已点击开始游戏按钮");
    QThread::msleep(2000);
    int X = width * 0.32;
    int Y = height / 2;
    MotionSimulator::LeftClick(m_gameHwnd, X, Y);
    emit logMessage("已点击加入游戏卡片，等待服务器列表加载片刻后点击加入上个对局");
    QThread::msleep(5000);

    /// 新方法使用搜索服务器查找加入上个服务器
    // 按照服务器代码搜索服务器
    X = width * 0.88;
    Y = height * 0.18;
    MotionSimulator::LeftClick(m_gameHwnd, X, Y);
    QThread::msleep(200);
    MotionSimulator::pasteText(m_serverCode);
    QThread::msleep(15000);

    // 点击列表第一个服务器
    X = width * 0.5;
    Y = height * 0.31;
    MotionSimulator::LeftClick(m_gameHwnd, X, Y);
    QThread::msleep(200);

    // 点击Join
    X = width * 0.88;
    Y = height * 0.88;
    MotionSimulator::LeftClick(m_gameHwnd, X, Y);
    emit logMessage("已点击重新加入上个对局，等待重新加入游戏，两分钟后重启监控进程！");
    QThread::msleep(120000);
    emit finished_0();
}
