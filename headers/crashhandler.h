#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QTimer>

#include <windows.h>

class crashHandler : public QObject
{
    Q_OBJECT
public:
    explicit crashHandler(QObject *parent = nullptr);
    ~crashHandler();
public slots:
    void startRestartGame();

signals:
    void gameIsReady();
    void gameIsRunning();
    void restartFailed();
    void finished_0(const HWND gameHwnd);
    void finished_1();
    void logMessage(const QString &msg);
    void scanGWTimeOut();
    void scanSBTimeOut();

private:
    QElapsedTimer *m_ETimer;
    QTimer *m_Timer2;
    QTimer *m_Timer;
    int m_timeoutMs;
    int m_timeoutMsSB;
    HWND m_gameHwnd;
    QString m_crashKeywords;
    void startScanStartButton();
    void closeCrashWindows();


private slots:
    void scanGameWindow();
    void scanStartButton();
    void finishScanGameWindow();
    void finishScanStartButton();
    void exit_0();
    void exit_1();
};

#endif // CRASHHANDLER_H
