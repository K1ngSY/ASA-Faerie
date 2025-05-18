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
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>

Alarm::Alarm(QObject *parent, QString APTitle, HWND APHwnd)
    : QObject{parent},
    m_gameWindowTitle("ArkAscended"),
    m_alarmPlatformTitle(APTitle),
    m_ocrKeywords("dino,发现,detected,an,enemy"),
    m_callMember("1"),
    m_gameWindowHwnd(nullptr),
    m_alarmPlatformHwnd(APHwnd),
    m_isGroup(false),
    m_needText(false),
    m_needCall(false),
    m_callButton_X(20),
    m_callButton_Y(20),
    m_timer(nullptr),
    m_keywords(QStringList()),
    m_errorkeys("HOST,CONNECTION,TIMEOUT,主机连接超时"),
    m_errorKeywords(QStringList())
{
    m_keywords = m_ocrKeywords.split(",", Qt::SkipEmptyParts);
    // 拆分关键字到字符串列表
    for (int i = 0; i < m_keywords.size(); ++i) {
        m_keywords[i] = m_keywords[i].trimmed();
    }

    m_errorKeywords = m_errorkeys.split(",", Qt::SkipEmptyParts);
    for (int i = 0; i < m_errorKeywords.size(); ++i) {
        m_errorKeywords[i] = m_errorKeywords[i].trimmed();
    }

    // 1) 设置持久化文件存放路径（程序所在目录下）
    m_logFilePath = QCoreApplication::applicationDirPath() + "/tribe_logs.txt";

    // 2) 初始化关键词列表
    m_seriousKeywords    << "摧毁" << "击杀" << "was destroyed" << "was killed by";
    m_nonSeriousKeywords << "饿死" << "已死亡" << "starved"   << "died" << "promoted" << "added" << "demolished" << "was killed" << "was removed" << "claimed" << "unclaimed";
}

Alarm::~Alarm()
{
    // 如果当前析构不在 Alarm 所在线程，就先同步调用 stop()
    if (thread() && thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(
            this,
            "stop",
            Qt::BlockingQueuedConnection
            );
    } else {
        // 如果已经在正确线程，直接停止并删除定时器
        if (m_timer) {
            if (m_timer->isActive())
                m_timer->stop();
            m_timer->disconnect(this);
            delete m_timer;
            m_timer = nullptr;
        }
    }

    // QObject 父子机制会自动删除子对象，无需再手动 delete
    // 但为了安全，确保指针置空
    m_timer = nullptr;
}

void Alarm::setAPTitle(QString APTitle)
{
    m_alarmPlatformTitle = APTitle;
    emit logMessage("alarmworker: setAPTitle: 已经设置新的微信窗口标题为" + m_alarmPlatformTitle);
}

void Alarm::setAPHwnd(HWND APHwnd)
{
    m_alarmPlatformHwnd = APHwnd;
    emit logMessage(QString("alarmworker: setAPTitle: 已经设置新的微信窗口句柄为 %1").arg((qulonglong)m_alarmPlatformHwnd));
}

void Alarm::startMonitoring()
{
    if (!m_timer) {
        m_timer = new QTimer(this);
        // ensure timer lives in this object’s thread
        m_timer->moveToThread(thread());
        m_timer->setInterval(5000);
        loadSettings();
        connect(m_timer, &QTimer::timeout, this, &Alarm::doOneRound);
    }
    if (!m_timer->isActive())
        m_timer->start();
    if (m_timer->isActive())
        emit logMessage("监控循环启动！");
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

void Alarm::stopMonitoring(){
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        emit logMessage("AlarmWorker: 已暂停TImer!");
    } else {
        emit logMessage("AlarmWorker: stopMonitoring: Timer不存在或不在运行！");
    }
    emit finished();
}

void Alarm::doOneRound(){
    if (!bindWindows(m_gameWindowTitle)) {
        emit logMessage("doOneRound: 无法绑定游戏窗口！");
        emit findWindowsFailed('G');
        return;
    }
    else if (!m_alarmPlatformHwnd) {
        emit warning("doOneRound: 通讯平台窗口句柄不存在！");
        emit findWindowsFailed('A');
        return;
    }
    else {
        if (!gameHandler::checkCrash())
        {
            emit logMessage("通过监测 游戏未崩溃");
            if (!gameHandler::checkWindowExist(m_gameWindowHwnd)){
                emit findWindowsFailed('G');
                return;
            }
            else if (!gameHandler::checkWindowExist(m_alarmPlatformHwnd)) {
                emit findWindowsFailed('A');
                return;
            }
            else{
                emit logMessage("已通过所有异常验证，开始分析本轮游戏窗口");
                saveSettings();
                QString ocrResult1, ocrResult2;
                QImage pic1, pic2, screenShot;
                QString sentText1, sentText2, detectedKeywords1, detectedKeywords2;
                bool alarmTriggered_P = false;

                RECT rect;
                if (!GetWindowRect(m_gameWindowHwnd, &rect)) {
                    emit logMessage("alarm: Failed to get window rect");
                    return;
                }
                // int width = rect.right - rect.left;
                // int height = rect.bottom - rect.top;
                // MotionSimulator::LeftClick(m_gameWindowHwnd, width / 2, height / 2);
                // MotionSimulator::WheelScroll(-1000);
                // QThread::msleep(100);
                // MotionSimulator::WheelScroll(-1000);
                // QThread::msleep(100);
                // MotionSimulator::WheelScroll(-1000);
                // QThread::msleep(100);
                // MotionSimulator::WheelScroll(-1000);
                // QThread::msleep(100);
                // MotionSimulator::WheelScroll(-1000);
                // QThread::msleep(100);
                // MotionSimulator::WheelScroll(-1000);
                // QThread::msleep(100);
                if (!VisualProcessor::analyzeGameWindow(m_gameWindowHwnd, ocrResult1, ocrResult2, pic1, pic2, screenShot)) {
                    emit logMessage("Alarm:analyzeGameWindow:Failed!");
                    return;
                }
                emit gotPic_1(pic1);
                emit gotPic_2(pic2);
                emit logMessage("OCR result (OCR识别到文字): " + ocrResult1 + " " + ocrResult2);
                int tries = 0;
                while (ocrResult2.isEmpty() && tries < 10) {
                    emit logMessage("侦测到部落日志未打开，尝试自动打开部落日志");
                    MotionSimulator::clickGameCenterAndKeyL(m_gameWindowHwnd);
                    QThread::msleep(500);
                    if (!VisualProcessor::analyzeGameWindow(m_gameWindowHwnd, ocrResult1, ocrResult2, pic1, pic2, screenShot)) {
                        emit logMessage("Fail to capture or recognize!\n截图或 OCR 失败!");
                        return;
                    }
                    if (!ocrResult2.isEmpty()) {
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
                    // 冒泡排序
                    auto entries = splitTribeLogEntries(ocrResult2);
                    int size = entries.size();
                    for (int i = 0; i < size - 1; i++) {
                        bool swapped = false;
                        for (int f = 0; f < size - i - 1; f++) {
                            if(entries[f].first < entries[f + 1].first) {
                                entries.swapItemsAt(f, f + 1);
                                swapped = true;
                            }
                        }
                        if (!swapped) {
                            break;
                        }
                    }
                    for (auto &p : entries) {
                        const QString &ts      = p.first;
                        const QString &content = p.second;

                        // 1) 持久化文件去重：只有 appendIfNewLogEntry 返回 true 的才继续
                        if (!appendIfNewLogEntry(ts, content)) {
                            emit logMessage("已存在日志，跳过: " + ts);
                            continue;
                        }

                        // 2) 分类检测：先严重，再非严重
                        bool isSerious = false, isNonSerious = false;
                        QString matchedKw;
                        for (auto &kw : m_seriousKeywords) {
                            if (content.contains(kw, Qt::CaseInsensitive)) {
                                isSerious = true;
                                matchedKw = kw;
                                break;
                            }
                        }
                        if (!isSerious) {
                            for (auto &kw : m_nonSeriousKeywords) {
                                if (content.contains(kw, Qt::CaseInsensitive)) {
                                    isNonSerious = true;
                                    matchedKw = kw;
                                    break;
                                }
                            }
                        }

                        // 3) 如果匹配到任一关键词，则按需报警
                        if (isSerious || isNonSerious || alarmTriggered_P) {
                            emit logMessage(QString("新部落日志事件 [%1] %2").arg(ts).arg(content));

                            // 构造文本
                            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                            bool callFlag = (isSerious && m_needCall);

                            if (alarmTriggered_P && (isSerious || isNonSerious)) {
                                QString pachyText = QString("[%1] \n(Key: %2)\n----------------------------------\nParasaurolophus has detected the enemy!\n您的副栉龙已发现敌人，请尽快上线查看！\n[%3]").arg(timeStamp).arg(detectedKeywords1).arg(ocrResult1);
                                QString tribeText = QString("[%1] \n(Key: %2)\n----------------------------------\nKeywords in the tribe log: %2 \n侦测到部落日志存在关键词：%2 请您尽快上线查看！\n[%3]").arg(ts).arg(matchedKw).arg(content);
                                sendAlarm(pachyText,
                                          tribeText,
                                          screenShot,
                                          m_needText,
                                          callFlag,
                                          true,   // isPlat=false
                                          true,    // isTribe=true
                                          m_alarmPlatformHwnd,
                                          m_callButton_X, m_callButton_Y,
                                          m_isGroup, m_callMember);
                                emit alarm(ocrResult1, ocrResult2, detectedKeywords1, detectedKeywords2);
                            } else if (isSerious || isNonSerious) {
                                QString tribeText = QString("[%1] \n(Key: %2)\n----------------------------------\nKeywords in the tribe log: %2 \n侦测到部落日志存在关键词：%2 请您尽快上线查看！\n[%3]").arg(ts).arg(matchedKw).arg(content);
                                sendAlarm(QString(),
                                          tribeText,
                                          screenShot,
                                          m_needText,
                                          callFlag,
                                          false,   // isPlat=false
                                          true,    // isTribe=true
                                          m_alarmPlatformHwnd,
                                          m_callButton_X, m_callButton_Y,
                                          m_isGroup, m_callMember);
                                emit alarm(QString(), ocrResult2, QString(), detectedKeywords2);
                            } else {
                                QString pachyText = QString("[%1] \n(Key: %2)\n----------------------------------\nParasaurolophus has detected the enemy!\n您的副栉龙已发现敌人，请尽快上线查看！\n[%3]").arg(timeStamp).arg(detectedKeywords1).arg(ocrResult1);
                                sendAlarm(pachyText,
                                          QString(),
                                          screenShot,
                                          m_needText,
                                          m_needCall,
                                          true,   // isPlat=false
                                          false,    // isTribe=true
                                          m_alarmPlatformHwnd,
                                          m_callButton_X, m_callButton_Y,
                                          m_isGroup, m_callMember);
                                emit alarm(ocrResult1, QString(), detectedKeywords1, QString());
                            }
                        }
                    }
                }
            }
        } else {
            emit gameCrashed();
            return;
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

void Alarm::setCallButtonPosition(int x, int y)
{
    m_callButton_X = x;
    m_callButton_Y = y;
    emit logMessage(QString("已更新微信电话坐标为 X: %1 Y: %2").arg(m_callButton_X).arg(m_callButton_Y));
}

void Alarm::testFunc()
{
    QImage fullSC;
    if (m_alarmPlatformHwnd){
        if (VisualProcessor::printWindow(m_alarmPlatformHwnd, fullSC)) {
        QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString sentText1 = QString("[%1] \n(Key: %2)\n----------------------------------\nParasaurolophus has detected the enemy!\n您的副栉龙已发现敌人，请尽快上线查看！\n[%3]").arg(timeStamp).arg("detectedKeywords1").arg("ocrResult1");
        QString sentText2 = QString("[%1] \n(Key: %2)\n----------------------------------\nKeywords in the tribe log: %2 \n侦测到部落日志存在关键词：%2 请您尽快上线查看！\n[%3]").arg(timeStamp).arg("detectedKeywords2").arg("ocrResult2");
        sendAlarm(sentText1,
                  sentText2,
                  fullSC,
                  m_needText,
                  m_needCall,
                  true,
                  true,
                  m_alarmPlatformHwnd,
                  m_callButton_X,
                  m_callButton_Y,
                  m_isGroup,
                  m_callMember);
        } else {
            emit logMessage("testFunc: 截图失败！");
        }
    } else {
        emit logMessage("testFunc: 微信窗口句柄不存在！");
    }
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

bool Alarm::bindWindows(const QString &title)
{
    std::wstring w = title.toStdWString();
    m_gameWindowHwnd = ::FindWindowW(nullptr, w.c_str());
    if (m_gameWindowHwnd) return true;
    else return false;
}

////////////////////////////////////////////////////////////////////////////////
// 把 OCR 识别出的 rawTribeLog 按 “Day X,HH:MM:SS:” 切成多条 (时间戳, 内容)
QList<QPair<QString, QString> > Alarm::splitTribeLogEntries(const QString &rawTribeLog)
{
    QList<QPair<QString, QString>> entries;
    QRegularExpression rx(R"(Day\s*\d+[,，]\s*\d{1,2}:\d{2}:\d{2}:)");
    auto it = rx.globalMatch(rawTribeLog);

    // 记录每个匹配的起始位置和文本
    QVector<int> positions;
    QVector<QString> stamps;
    while (it.hasNext()) {
        auto m = it.next();
        positions.append(m.capturedStart());
        stamps.append(m.captured(0));
    }
    // 加上末尾，便于切最后一段
    positions.append(rawTribeLog.length());

    // 逐段切片：去掉时间戳前缀，只留后续内容
    for (int i = 0; i < stamps.size(); ++i) {
        int start = positions[i], end = positions[i+1];
        QString segment = rawTribeLog.mid(start, end - start).trimmed();
        QString content = segment.mid(stamps[i].length()).trimmed();
        entries.append(qMakePair(stamps[i], content));
    }
    return entries;
}

////////////////////////////////////////////////////////////////////////////////
// 如文件中无该时间戳，则追加一行 "时间戳\t内容"，并返回 true；否则返回 false
bool Alarm::appendIfNewLogEntry(const QString &ts, const QString &content)
{
    QFile file(m_logFilePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        emit logMessage("无法打开持久化日志文件：" + m_logFilePath);
        return false;
    }

    // 1) 先检查文件中是否已有该时间戳
    QTextStream in(&file);
    bool exists = false;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith(ts + "\t")) {
            exists = true;
            break;
        }
    }

    // 2) 如果不存在，则移动到末尾，写入新条目
    if (!exists) {
        QTextStream out(&file);
        file.seek(file.size());
        out << ts << "\t" << content << "\n";
        return true;
    }
    return false;
}
