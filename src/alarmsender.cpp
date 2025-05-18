#include "alarmsender.h"
#include "motionsimulator.h"
AlarmSender::AlarmSender(QObject *parent)
    : QObject{parent}
{}

void AlarmSender::sendText(const QString &msg, HWND widHwnd)
{
    MotionSimulator::sendTextAlarm(msg, widHwnd);
}
