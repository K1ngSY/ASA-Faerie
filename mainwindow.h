#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>
#include "overlaywindow.h"
#include "crashhandler.h"
#include "rejoinprocessor.h"
#include "expirationdetector.h"
#include <alarm.h>
#include <QThread>
#include <QDateTime>

class OverlayWindow;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setExpireDate(const QString &dateString);

private:
    Ui::MainWindow *ui;
    OverlayWindow *m_overlayWindow;
    HWND m_alarmPlatformHwnd;
    HWND m_gameHwnd;
    QString m_alarmPlatformTitle;
    Alarm *m_alarmWorker;
    QThread *m_alarmThread;
    QTimer *m_sliderUpdateTimer;
    crashHandler *m_CrashHandler;
    QThread *m_crashHandlerThread;
    RejoinProcessor *m_RejoinProcessor;
    QThread *m_rejoinProcessorThread;
    QDateTime m_expireDateTime;
    ExpirationDetector *m_ExpirationDetector;
    QThread *m_expirationDetectorThread;
    int m_dot_X;
    int m_dot_Y;
    void appendLog(const QString &message);
    // 传入从崩溃处理器拿到的游戏窗口句柄 开始加入上个对局
    void startRejoin();



private slots:
    void overlayVisibilityChanged(int state);
    void selectWindow();
    void startMonitoring();
    // 槽函数 停止监控并安全退出监控进程
    void stopMonitoring();
    void transferLog(const QString &message);
    void onWorkerFinished();
    void updateOverlayPosition_Slider(int value);
    void updateSliderRanges();
    void updateCallMember(QString callMember);
    void updateImage1(const QImage &pic);
    void updateImage2(const QImage &pic);
    void onAlarmSent(const QString &p_ocrResult_1, const QString &p_ocrResult_2, const QString &p_key_1, const QString &p_key_2);
    void restartGame();
    void windowFailurehandler(char w);
    void warningHandler(const QString &msg);
    void setGameHwnd(::HWND gameHwnd);
    // 槽函数 处理崩溃成功后 保存游戏窗口句柄到成员变量 安全退出崩溃处理器 且开始加入上个对局
    void onCHFinished_0(::HWND gameHwnd);
    // 槽函数 处理崩溃失败后安全退出崩溃处理器
    void onCHFinished_1();
    // 槽函数 初始化加入上一个游戏
    void starRejoinGame();

    void finishRejoinGame();
    // 槽函数 初始化崩溃处理
    void onGameCrashed();
    // 槽函数 成功加入上个对局 发起监控
    void onRPFinished_0();
    // 槽函数 加入上个对局失败 (引导手动修复或者尝试再次重启游戏)
    void onRPFinished_1();
    // 槽函数处理局内掉线游戏
    void handleGameSessionDisconnected();
signals:
};
#endif // MAINWINDOW_H
