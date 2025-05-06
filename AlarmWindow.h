#ifndef ALARMWINDOW_H
#define ALARMWINDOW_H

#include "qdatetime.h"
#include <QMainWindow>
#include <windows.h>
#include <QTimer>
#include <QDateTime>

class OverlayWindow;

namespace Ui {
class AlarmWindow;
}

class AlarmWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AlarmWindow(QWidget *parent = nullptr);
    ~AlarmWindow();
    void setExpireDate(const QString &dateString);
private slots:
    void selectWindows();
    void startMonitoring();
    void closeMonitoring();
    void updateOCRKeywords();
    void checkForAlarm();
    void updateOverlayPosition_Slider(int value);
    void overlayVisibilityChanged(int state);
    void gameOverlayVisibilityChanged(int state);
    void updateMonitoringCycle();
    void testImagePaste();
    void testAlarm();
    void updateSliderRanges();
    void updateCallMember();
    void onOneKeyStart();
    void checkExpiration();
private:
    void appendLog(const QString &message);
    void saveConfig();
    void loadConfig();
    static void setSliderMaxmum(int x, int y);
    Ui::AlarmWindow *ui;
    QString m_gameWindowTitle;
    QString m_wechatWindowTitle;
    // 当前选择的窗口句柄
    HWND m_gameWindowHwnd;
    HWND m_wechatWindowHwnd;
    // OCR关键字，多个关键字以逗号分隔
    QString m_ocrKeywords;
    QString m_callMember;
    // 定时器：监控周期
    QTimer *m_monitorTimer;
    // 刷新滑块最大值
    QTimer *m_sliderUpdateTimer;
    // 白框窗口标题关键词
    QString m_crashKeywords;
    // 覆盖层窗口，用于模拟拨打电话界面
    OverlayWindow *m_overlayWindow;
    QDateTime   m_expireDateTime;   // 从服务器返回并解析的到期时间
    QTimer     *m_expireTimer;
    int monitoringCycle;
    int weChatDotX;
    int weChatDotY;
    int rejoinButtonX;
    int rejoinButtonY;
    bool m_isMonitoring = false;          // guard against duplicate starts
    void closeCrashWindows();             // close any crash dialogs                // restart via Steam
    bool restartGame();
    void restartGame(std::function<void()> next);
    void clickGameCenterAndKeyL();       // click center + L key
    void findAndBindWindows();            // resolve titles to HWND
    bool checkWindowsExist() const;
    // 冷启动 设计为从游戏不存在的情况下启动并加入上个对局
    void coldRejoin();
    // 冷启动的辅助函数 监测发起游戏启动信号后开始游戏按钮的出现
    void waitForRejoin(std::function<void(bool)> onFinished);
    void waitForRejoin();
    // 冷启动辅助函数2 执行剩余步骤
    void coldRejoinHelper_2();
    // 副栉龙警报文本, 部落日志文本, 发微信消息选框值, 打微信电话选框值, 副栉龙是否监测到警报, 部落日志是否监测到警报, 窗口句柄, 微信电话坐标 x, y, 微信窗口是否为群聊, 呼叫群聊成员
    static bool sendAlarm(const QString &text1, const QString &text2, const bool &sendText, const bool &makeCall, const bool &detected_P, const bool &detected_L, HWND WCHwnd, HWND GAMEHwmd, int X, int Y, bool isGroup, QString callMember);
public slots:
    void setExpireDateLabel(const QString &date);

};

#endif // ALARMWINDOW_H
