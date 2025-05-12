#include "rejoinprocessor.h"
#include "motionsimulator.h"
#include "visualprocessor.h"

#include <QDebug>
#include <QThread>

#include <windows.h>

RejoinProcessor::RejoinProcessor(QObject *parent, HWND gameHwnd)
    : QObject{parent}, m_gameHwnd(gameHwnd)
{}

void RejoinProcessor::setHwnd(HWND gameHwnd)
{
    m_gameHwnd=gameHwnd;
}

void RejoinProcessor::startRejoin()
{
    RECT rect;
    if (!GetWindowRect(m_gameHwnd, &rect)) {
        emit logMessage("rejoin: Failed to get window rect");
        emit finished_1();
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    // 找到后执行点击
    MotionSimulator::LeftClick(m_gameHwnd, static_cast<int>(width / 2), static_cast<int>(height * 43 / 54));
    emit logMessage("rejoin: 已点击开始游戏按钮");
    QThread::msleep(2000);
    int X, Y;
    if(VisualProcessor::checkJoinCard(m_gameHwnd, X, Y)) {
        MotionSimulator::LeftClick(m_gameHwnd, X, Y);
    }
    else {
        emit logMessage("未找到加入游戏卡片");
        emit finished_1();
    }
    QThread::msleep(15000);
    MotionSimulator::LeftClick(m_gameHwnd, static_cast<int>(width * 0.9), static_cast<int>(height * 0.83));
    emit logMessage("已点击重新加入上个对局，等待重新加入游戏，两分钟后重启监控进程！");
    QThread::msleep(120000);
    emit finished_0();
}
