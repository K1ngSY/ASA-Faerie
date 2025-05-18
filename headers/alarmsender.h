#ifndef ALARMSENDER_H
#define ALARMSENDER_H

#include <QObject>
#include <QString>
#include <windows.h>

class AlarmSender : public QObject
{
    Q_OBJECT
public:
    explicit AlarmSender(QObject *parent = nullptr);

public slots:
    void sendText(const QString &msg, HWND widHwnd);
signals:
};

#endif // ALARMSENDER_H
