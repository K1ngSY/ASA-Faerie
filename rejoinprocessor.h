#ifndef REJOINPROCESSOR_H
#define REJOINPROCESSOR_H

#include <QObject>
#include <QString>
#include <windows.h>

class RejoinProcessor : public QObject
{
    Q_OBJECT
public:
    explicit RejoinProcessor(QObject *parent = nullptr, HWND gameHwnd = nullptr);
    void setHwnd(HWND gameHwnd);
    void setServerCode(QString serverCode);
public slots:
    void startRejoin();

signals:
    // 信号 加入到上个对局成功
    void finished_0();
    // 信号 加入上个对局失败
    void finished_1();
    // 信号 给主窗追加日志
    void logMessage(const QString &msg);
private:
    HWND m_gameHwnd;
    QString m_serverCode;
};

#endif // REJOINPROCESSOR_H
