#ifndef ALARM_H
#define ALARM_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QTimer>
#include <QStringList>
#include <QSet>
#include <windows.h>

class Alarm : public QObject
{
    Q_OBJECT
public:
    explicit Alarm(QObject *parent = nullptr, QString APTitle = "", HWND APHwnd = nullptr);
    ~Alarm();
    void setAPTitle(QString APTitle);
    void setAPHwnd(HWND APHwnd);
    void updateCallMember(QString callMember);
    void setCallButtonPosition(int x, int y);

public slots:
    // 启动监控循环
    void startMonitoring();
    void stopMonitoring();
    // void updateGroupCallSettings(const QString &targetIndices);
    // void updateTextStatus(bool need);
    // void updateCallStatus(bool need);
    void setAP(QString title, HWND APHwnd);
    void updateGroupStatus(Qt::CheckState state);
    void updateTextAlarmStatus(Qt::CheckState state);
    void updateCallAlarmStatus(Qt::CheckState state);
    void testFunc();

signals:
    // 信号：给主窗追加日志
    void logMessage(const QString &msg);
    // 信号：发起警报
    void alarm(const QString &p_ocrResult_1 = "", const QString &p_ocrResult_2 = "", const QString &p_key_1 = "", const QString &p_key_2 = "");
    // 信号：游戏监控进程结束
    void finished();
    // 信号：找不到窗口 G = Game A = AlarmPlatform
    void findWindowsFailed(char w);

    void gameCrashed();

    void gotPic_1(const QImage &pic_1);

    void gotPic_2(const QImage &pic_2);

    void warning(const QString &msg);

    void gameTimeout();

private:
    QString m_gameWindowTitle;
    QString m_alarmPlatformTitle;
    QString m_ocrKeywords;
    QString m_callMember;
    HWND m_gameWindowHwnd;
    HWND m_alarmPlatformHwnd;
    bool m_isGroup;
    bool m_needText;
    bool m_needCall;
    int m_callButton_X;
    int m_callButton_Y;
    QTimer *m_timer;
    QStringList m_keywords;
    QString m_errorkeys;
    QStringList m_errorKeywords;
    void saveSettings();
    void loadSettings();
    bool bindWindows(const QString &title);

    // 持久化日志文件路径
    QString m_logFilePath;

    // 严重/非严重关键词列表
    QStringList m_seriousKeywords;
    QStringList m_nonSeriousKeywords;

    // 读取并按时间戳分割 OCR 一大段部落日志
    QList<QPair<QString, QString>> splitTribeLogEntries(const QString &rawTribeLog);

    // 如果日志文件中不存在该时间戳，则把“时间戳\t内容”写入文件，返回 true；否则返回 false
    bool appendIfNewLogEntry(const QString &ts, const QString &content);

    // 副栉龙警报文本, 部落日志文本, 游戏截图，发微信消息选框值, 打微信电话选框值, 副栉龙是否监测到警报, 部落日志是否监测到警报, 窗口句柄, 微信电话坐标 x, y, 微信窗口是否为群聊, 呼叫群聊成员
    void sendAlarm(const QString &text1,
                   const QString &text2,
                   const QImage &fullScreen,
                   const bool &sendText,
                   const bool &makeCall,
                   const bool &detected_P,
                   const bool &detected_L,
                   HWND APHwnd,
                   int X,
                   int Y,
                   bool isGroup,
                   QString callMember);

private slots:
    void doOneRound();
};

#endif // ALARM_H
