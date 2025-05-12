#include "alarm.h"
#include "gamehandler.h"
#include "visualprocessor.h"
#include "motionsimulator.h"
#include <windows.h>
#include <QImage>
#include <QDate>
#include <QThread>
#include <QTimer>
#include <QSettings>

Alarm::Alarm(QObject *parent, QString APTitle, HWND APHwnd)
    : QObject{parent},
    m_gameWindowTitle("ArkAscended"),
    m_alarmPlatformTitle(APTitle),
    m_ocrKeywords("摧毁,击杀"),
    m_callMember("1"),
    m_gameWindowHwnd(nullptr),
    m_alarmPlatformHwnd(APHwnd),
    m_isGroup(false),
    m_needText(false),
    m_needCall(false),
    m_CONTINUE(true),
    m_callButton_X(20),
    m_callButton_Y(20),
    m_timer(nullptr),
    m_keywords(QStringList()),
    m_errorkeys("disconnected,timeout"),
    m_errorKeywords(QStringList())
{
    m_timer = new QTimer(this);
    m_timer->setInterval(10000);
    connect(m_timer, &QTimer::timeout, this, &Alarm::doOneRound);
    connect(this, &Alarm::finished, m_timer, &QTimer::stop);
    connect(this, &Alarm::finished, m_timer, &QTimer::deleteLater);

    m_keywords = m_ocrKeywords.split(",", Qt::SkipEmptyParts);
    // 拆分关键字到字符串列表
    for (int i = 0; i < m_keywords.size(); ++i) {
        m_keywords[i] = m_keywords[i].trimmed();
    }

    m_errorKeywords = m_errorkeys.split(",", Qt::SkipEmptyParts);
    for (int i = 0; i < m_errorKeywords.size(); ++i) {
        m_errorKeywords[i] = m_errorKeywords[i].trimmed();
    }
}

void Alarm::setAPTitle(QString APTitle)
{
    m_alarmPlatformTitle = APTitle;
}

void Alarm::setAPHwnd(HWND APHwnd)
{
    m_alarmPlatformHwnd = APHwnd;
}

void Alarm::startMonitoring()
{
    loadSettings();
    m_timer->start();
    if (m_timer->isActive()) {
        emit logMessage("监控循环启动！");
    }
}

void Alarm::sendAlarm(const QString &text1,
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
                      QString callMember) {
    if(detected_P && detected_L){
        if(sendText) {
            MotionSimulator::sendTextAlarm(text1, APHwnd);
            MotionSimulator::sendTextAlarm(text2, APHwnd);
            MotionSimulator::sendImageAlarm(fullScreen, APHwnd);
        }
        if(makeCall) {
            if(isGroup) {
                MotionSimulator::LeftClick(APHwnd, X, Y);
                MotionSimulator::sendGroupCall(callMember);
            }
            else {
                MotionSimulator::LeftClick(APHwnd, X, Y);
            }
        }
        return;
    }
    else if(detected_P){
        if(sendText) {
            MotionSimulator::sendTextAlarm(text1, APHwnd);
            MotionSimulator::sendImageAlarm(fullScreen, APHwnd);
        }
        if(makeCall) {
            if(isGroup) {
                MotionSimulator::LeftClick(APHwnd, X, Y);
                MotionSimulator::sendGroupCall(callMember);
            }
            else {
                MotionSimulator::LeftClick(APHwnd, X, Y);
            }
        }
        return;
    }
    else if(detected_L){
        if(sendText) {
            MotionSimulator::sendTextAlarm(text2, APHwnd);
            MotionSimulator::sendImageAlarm(fullScreen, APHwnd);
        }
        if(makeCall) {
            if(isGroup) {
                MotionSimulator::LeftClick(APHwnd, X, Y);
                MotionSimulator::sendGroupCall(callMember);
            }
            else {
                MotionSimulator::LeftClick(APHwnd, X, Y);
            }
        }
        return;
    }
    emit logMessage("发送警报失败：未知错误");
}

void Alarm::stop(){
    m_CONTINUE = false;
    emit finished();
}

void Alarm::doOneRound(){
    if (gameHandler::bindWindows(m_gameWindowTitle, m_gameWindowHwnd)) {
        emit warning("doOneRound: 无法绑定游戏窗口！");
        emit findWindowsFailed('G');
        return;
    }
    else if (!m_alarmPlatformHwnd) {
        emit warning("doOneRound: 通讯平台窗口句柄不存在！");
        emit findWindowsFailed('A');
        return;
    }
    else {
        if (m_CONTINUE) {
            if (!gameHandler::checkCrash())
            {
                if (!gameHandler::checkWindowExist(m_gameWindowHwnd)){
                    emit findWindowsFailed('G');
                }
                else if (!gameHandler::checkWindowExist(m_alarmPlatformHwnd)) {
                    emit findWindowsFailed('A');
                }
                else{
                    emit logMessage("已通过所有验证，开始分析本轮游戏窗口");
                    saveSettings();
                    QString ocrResult1, ocrResult2;
                    QImage pic1, pic2, screenShot;
                    QString sentText1, sentText2, detectedKeywords1, detectedKeywords2;
                    bool alarmTriggered_P = false;
                    bool alarmTriggered_L = false;

                    if (!VisualProcessor::analyzeGameWindow(m_gameWindowHwnd, ocrResult1, ocrResult2, pic1, pic2, screenShot)) {
                        emit logMessage("Alarm:analyzeGameWindow:Failed!");
                        return;
                    }

                    /*
                    ui->image1Label->setPixmap(QPixmap::fromImage(pic1));
                    ui->image2Label->setPixmap(QPixmap::fromImage(pic2));
                    */
                    emit gotPic_1(pic1);
                    emit gotPic_2(pic2);

                    emit logMessage("OCR result (OCR识别到文字): " + ocrResult1 + " " + ocrResult2);
                    int tries = 0;
                    while (ocrResult2.isEmpty() && tries < 10) {
                        MotionSimulator::clickGameCenterAndKeyL(m_gameWindowHwnd);
                        emit logMessage("侦测到部落日志未打开，尝试自动打开部落日志");
                        if (!VisualProcessor::analyzeGameWindow(m_gameWindowHwnd, ocrResult1, ocrResult2, pic1, pic2, screenShot)) {
                            emit logMessage("Fail to capture or recognize!\n截图或 OCR 失败!");
                            return;
                        }
                        if (!ocrResult2.isEmpty()) {
                            // ui->image1Label->setPixmap(QPixmap::fromImage(pic1));
                            // ui->image2Label->setPixmap(QPixmap::fromImage(pic2));
                            emit gotPic_1(pic1);
                            emit gotPic_2(pic2);
                            break;
                        }
                        tries++;
                    }
                    if (tries >= 10) {
                        emit logMessage("无法打开部落日志");
                        return;
                    }
                    if (!ocrResult2.isEmpty()) {
                        if (!ocrResult1.isEmpty()) {
                            for (const QString &keyword : m_keywords) {
                                if (!keyword.isEmpty() && ocrResult1.contains(keyword)) {
                                    alarmTriggered_P = true;
                                    detectedKeywords1 = keyword;
                                    emit logMessage("Detected keywords in Parasaurolophus Alert(检测到副栉龙警报中存在关键字): " + keyword);
                                    break;
                                }
                            }
                        }
                        // 监测游戏对话是否超时或丢失连接
                        for (const QString &keyword : m_errorKeywords) {
                            if (!keyword.isEmpty() && ocrResult2.contains(keyword)) {
                                emit logMessage("检测到游戏对话丢失连接或超时: " + keyword);
                                emit gameTimeout();
                                break;
                            }
                        }
                        for (const QString &keyword : m_keywords) {
                            if (!keyword.isEmpty() && ocrResult2.contains(keyword)) {
                                alarmTriggered_L = true;
                                detectedKeywords2 = keyword;
                                emit logMessage("Detected keywords in Tribe Log (检测到部落日志中存在关键字): " + keyword);
                                break;
                            }
                        }
                        if (alarmTriggered_P || alarmTriggered_L) {
                            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                            sentText1 = "";
                            sentText2 = "";
                            if (alarmTriggered_P)
                                sentText1 = QString("[%1] \n(Key: %2)\n----------------------------------\nParasaurolophus has detected the enemy!\n您的副栉龙已发现敌人，请尽快上线查看！\n[%3]").arg(timeStamp).arg(detectedKeywords1).arg(ocrResult1);
                            if (alarmTriggered_L)
                                sentText2 = QString("[%1] \n(Key: %2)\n----------------------------------\nKeywords in the tribe log: %2 \n侦测到部落日志存在关键词：%2 请您尽快上线查看！\n[%3]").arg(timeStamp).arg(detectedKeywords2).arg(ocrResult2);
                            sendAlarm(sentText1,
                                      sentText2,
                                      screenShot,
                                      m_needText,
                                      m_needCall,
                                      alarmTriggered_P,
                                      alarmTriggered_L,
                                      m_alarmPlatformHwnd,
                                      m_callButton_X,
                                      m_callButton_Y,
                                      m_isGroup,
                                      m_callMember);
                            emit alarm(ocrResult1, ocrResult2, detectedKeywords1, detectedKeywords2);
                        }
                    }
                }
            } else {
                emit gameCrashed();
                return;
            }
        }
    }
}

void Alarm::setAP(QString title, HWND APHwnd) {
    m_alarmPlatformTitle = title;
    m_alarmPlatformHwnd = APHwnd;
}

void Alarm::updateGroupStatus(Qt::CheckState state)
{
    m_isGroup = (state == Qt::Checked);
    if(m_isGroup) {
        emit logMessage("alarm: 配置更新，启用群聊电话功能");
    } else {
        emit logMessage("alarm: 配置更新，关闭群聊电话功能");
    }
}

void Alarm::updateTextAlarmStatus(Qt::CheckState state)
{
    m_needText = (state == Qt::Checked);
    if(m_needText) {
        emit logMessage("alarm: 配置更新，启用文本警报功能");
    } else {
        emit logMessage("alarm: 配置更新，关闭文本警报功能");
    }
}

void Alarm::updateCallAlarmStatus(Qt::CheckState state)
{
    m_needCall = (state == Qt::Checked);
    if(m_needCall) {
        emit logMessage("alarm: 配置更新，启用微信电话功能");
    } else {
        emit logMessage("alarm: 配置更新，关闭微信电话功能");
    }
}

void Alarm::updateCallMember(QString callMember)
{
    m_callMember = callMember;
    emit logMessage("alarm: 微信电话成员已更新为: " + m_callMember);
}

void Alarm::saveSettings() {
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("callMember", m_callMember);
    settings.setValue("APDotX", m_callButton_X);
    settings.setValue("APDotY", m_callButton_Y);
    settings.endGroup();
    emit logMessage("配置已保存");
}

void Alarm::loadSettings()
{
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    m_callMember = settings.value("callMember", "1").toString();
    int x = settings.value("APDotX", 20).toInt();
    int y = settings.value("APDotY", 20).toInt();
    m_callButton_X = x;
    m_callButton_Y = y;
    settings.endGroup();
    emit logMessage("配置已加载");
}


