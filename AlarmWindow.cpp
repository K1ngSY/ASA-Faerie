#include "AlarmWindow.h"
#include "ui_AlarmWindow.h"
#include "WindowSelectionDialog.h"
#include "GameMonitor.h"
#include "MotionSimulator.h"
#include "OverlayWindow.h"
// #include "MainWindowHelper.h"

#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QSettings>
#include <QDesktopServices>
#include <shellapi.h>
#include <QElapsedTimer>
#include <QThread>
#include <QCoreApplication>

AlarmWindow::AlarmWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AlarmWindow),
    m_gameWindowTitle("ArkAscended"),    // 默认ArkAscended，用户选择后重新赋值
    m_wechatWindowTitle(""),
    m_gameWindowHwnd(nullptr),
    m_wechatWindowHwnd(nullptr),
    m_ocrKeywords("摧毁,入侵"),
    m_callMember("1,2"),
    m_monitorTimer(nullptr),
    m_sliderUpdateTimer(nullptr),
    m_crashKeywords("crash, Crash, Fatal error!,Unhandled Exception,LowLevelFatalError,has stopped working"),
    m_overlayWindow(nullptr),
    m_expireTimer(new QTimer(this)),
    monitoringCycle(10000),
    weChatDotX(20),
    weChatDotY(20),
    rejoinButtonX(20),
    rejoinButtonY(20)
{
    ui->setupUi(this);

    // 设置默认 OCR 关键字文本
    ui->ocrKeywordsTextEdit->setPlainText(m_ocrKeywords);
    ui->callMemberTextEdit->setPlainText(m_callMember);

    ui->monitoringCycleSpinBox->setValue(monitoringCycle);

    m_sliderUpdateTimer = new QTimer(this);
    connect(m_sliderUpdateTimer, &QTimer::timeout, this, &AlarmWindow::updateSliderRanges);
    m_sliderUpdateTimer->start(100);
    ui->statusLabel->setStyleSheet("font-weight: bold; font-size: 10px; color:#ff0000;");
    m_expireTimer->setInterval(60 * 1000);
    connect(m_expireTimer, &QTimer::timeout, this, &AlarmWindow::checkExpiration);

    // 连接各按钮和控件
    connect(ui->selectButton, &QPushButton::clicked, this, &AlarmWindow::selectWindows);
    connect(ui->startButton, &QPushButton::clicked, this, &AlarmWindow::startMonitoring);
    connect(ui->closeMonitoringButton, &QPushButton::clicked, this, &AlarmWindow::closeMonitoring);
    connect(ui->updateKeywordsButton, &QPushButton::clicked, this, &AlarmWindow::updateOCRKeywords);
    connect(ui->updateCallMemberButton, &QPushButton::clicked, this, &AlarmWindow::updateCallMember);
    connect(ui->overlayVisibleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlayVisibilityChanged(int)));
    connect(ui->gameOverlayVisibleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(gameOverlayVisibilityChanged(int)));
    connect(ui->monitoringCycleSpinBox, &QSpinBox::valueChanged, this, &AlarmWindow::updateMonitoringCycle);
    connect(ui->testImagepushButton, &QPushButton::clicked, this, &AlarmWindow::testImagePaste);
    connect(ui->testAlarmpushButton, &QPushButton::clicked, this, &AlarmWindow::testAlarm);
    connect(ui->horizontalSlider_X, &QSlider::valueChanged, this, &AlarmWindow::updateOverlayPosition_Slider);
    connect(ui->horizontalSlider_Y, &QSlider::valueChanged, this, &AlarmWindow::updateOverlayPosition_Slider);
    connect(ui->oneKeyStartButton, &QPushButton::clicked, this, &AlarmWindow::onOneKeyStart);

    appendLog("Program started -- 程序已启动");
}

AlarmWindow::~AlarmWindow()
{
    if(m_monitorTimer) m_monitorTimer->stop();
    // if(m_overlayTimer) m_overlayTimer->stop();
    delete m_overlayWindow;
    delete ui;
}

void AlarmWindow::appendLog(const QString &message)
{
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logLine = QString("[%1] %2").arg(timeStamp).arg(message);
    qDebug() << logLine;
    if(ui->logTextEdit)
        ui->logTextEdit->append(logLine);
}

void AlarmWindow::saveConfig()
{
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("ocrKeywords", m_ocrKeywords);
    settings.setValue("callMember", m_callMember);
    settings.setValue("gameWindowTitle", m_gameWindowTitle);
    settings.setValue("wechatWindowTitle", m_wechatWindowTitle);
    settings.setValue("wechatDotX", weChatDotX);
    settings.setValue("wechatDotY", weChatDotY);
    settings.setValue("rejoinButtonX", rejoinButtonX);
    settings.setValue("rejoinButtonY", rejoinButtonY);
    settings.setValue("crashKeywords", m_crashKeywords);
    settings.endGroup();
    appendLog("配置已保存");
}

void AlarmWindow::loadConfig()
{
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    m_ocrKeywords = settings.value("ocrKeywords", "摧毁,入侵,报警").toString();
    m_callMember = settings.value("callMember", "1,2,3").toString();
    m_gameWindowTitle = settings.value("gameWindowTitle", "ArkAscended").toString();
    m_wechatWindowTitle = settings.value("wechatWindowTitle", "").toString();
    int x = settings.value("wechatDotX", 20).toInt();
    int y = settings.value("wechatDotY", 20).toInt();
    weChatDotX = x;
    weChatDotY = y;
    x = settings.value("rejoinButtonX", 20).toInt();
    y = settings.value("rejoinButtonY", 20).toInt();
    rejoinButtonX = x;
    rejoinButtonY = y;
    m_crashKeywords = settings.value("crashKeywords", "crash, Crash, Fatal error!,Unhandled Exception,LowLevelFatalError,has stopped working").toString();
    settings.endGroup();
    ui->ocrKeywordsTextEdit->setPlainText(m_ocrKeywords);
    appendLog("配置已加载");

    // 根据保存的窗口标题重新获取窗口句柄
    if (!m_gameWindowTitle.isEmpty()) {
        // 使用 FindWindowW 查找窗口，注意转换为宽字符串
        std::wstring gameTitle = m_gameWindowTitle.toStdWString();
        m_gameWindowHwnd = FindWindowW(nullptr, gameTitle.c_str());
        if (!m_gameWindowHwnd)
            appendLog("loadConfig: 未能找到游戏窗口，检查窗口标题是否正确且是否已打开");
        else
            appendLog("loadConfig: 成功获取游戏窗口句柄");
    }
    if (!m_wechatWindowTitle.isEmpty()) {
        std::wstring wechatTitle = m_wechatWindowTitle.toStdWString();
        m_wechatWindowHwnd = FindWindowW(nullptr, wechatTitle.c_str());
        if (!m_wechatWindowHwnd)
            appendLog("loadConfig: 未能找到微信窗口，检查窗口标题是否正确且是否已打开");
        else
            appendLog("loadConfig: 成功获取微信窗口句柄");
    }
}

void AlarmWindow::onOneKeyStart()
{   ui->statusLabel->setText("尝试使用配置文件一键启动……");
    if (m_isMonitoring) {
        ui->statusLabel->setText("监控已在运行中");
        return;
    }
    loadConfig();
    findAndBindWindows();
    if (!m_gameWindowHwnd || !m_wechatWindowHwnd) {
        appendLog("一键启动：WeChat或ASA窗口句柄加载失败！");
        ui->statusLabel->setText("一键启动：WeChat或ASA窗口句柄加载失败！");
        QMessageBox::warning(this, "错误", "未设置正确的游戏窗口或微信窗口标题！");
        return;
    }
    startMonitoring();
}

void AlarmWindow::selectWindows()
{
    appendLog("开始选择窗口……");
    ui->statusLabel->setText("正在执行窗口选择……");
    // 选择游戏窗口
    WindowSelectionDialog gameDialog(this);
    gameDialog.setWindowTitle("选择游戏窗口");
    gameDialog.setLabel1Text("选择游戏窗口");
    if (gameDialog.exec() == QDialog::Accepted) {
        auto gameWin = gameDialog.selectedWindow();
        m_gameWindowHwnd = gameWin.hwnd;
        m_gameWindowTitle = gameWin.title;  // 保存窗口标题
        appendLog(QString("选定游戏窗口：%1 句柄：%2").arg(gameWin.title).arg((qulonglong)m_gameWindowHwnd));
    } else {
        appendLog("未选择游戏窗口");
        QMessageBox::warning(this, "提示", "未选择游戏窗口");
        return;
    }
    // 选择微信窗口
    WindowSelectionDialog wechatDialog(this);
    wechatDialog.setWindowTitle("选择微信窗口");
    wechatDialog.setLabel1Text("选择微信窗口");
    if (wechatDialog.exec() == QDialog::Accepted) {
        auto wechatWin = wechatDialog.selectedWindow();
        m_wechatWindowHwnd = wechatWin.hwnd;
        m_wechatWindowTitle = wechatWin.title;  // 保存窗口标题
        appendLog(QString("选定微信窗口：%1 句柄：%2").arg(wechatWin.title).arg((qulonglong)m_wechatWindowHwnd));
    } else {
        appendLog("未选择微信窗口");
        QMessageBox::warning(this, "提示", "未选择微信窗口");
        return;
    }
    appendLog("窗口选择完成");
    ui->statusLabel->setText("窗口选择完成！");
    QMessageBox::information(this, "提示", "窗口选择完成");
}

void AlarmWindow::startMonitoring()
{
    if (m_isMonitoring) {
        qDebug() << "Monitoring already running, ignoring.";
        ui->statusLabel->setText("监控已在运行中！");
        return;
    }
    if (!m_gameWindowHwnd || !m_wechatWindowHwnd) {
        appendLog("错误：未选择游戏或微信窗口");
        ui->statusLabel->setText("错误：未选择游戏或微信窗口");
        QMessageBox::warning(this, "错误", "请先选择游戏和微信窗口");
        return;
    }
    appendLog("启动监控……");
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &AlarmWindow::checkForAlarm);
    m_monitorTimer->start(monitoringCycle); // 5s一次
    // 立即检查一次
    checkForAlarm();
    appendLog("监控已启动");
    ui->statusLabel->setText("监控正在运行中！");
    saveConfig();
}

void AlarmWindow::closeMonitoring()
{
    if(m_isMonitoring && m_monitorTimer) {
        m_isMonitoring = false;
        m_monitorTimer->stop();
        appendLog("close:监控已关闭");
        ui->statusLabel->setText("close:监控已被手动关闭！");
    } else {
        appendLog("close:监控未在正常运行，尝试关闭");
        m_isMonitoring = false;
        if (m_monitorTimer) {
            m_monitorTimer->stop();
            appendLog("close:监控timer已关闭");
        } else {
            appendLog("close:监控未正常运行 timer不存在");
        }

        ui->statusLabel->setText("close:监控在异常状态下被手动关闭！");
    }
}

void AlarmWindow::updateOCRKeywords()
{
    m_ocrKeywords = ui->ocrKeywordsTextEdit->toPlainText().trimmed();
    appendLog("更新 OCR 关键字为: " + m_ocrKeywords);
    ui->ocrKeywordsTextEdit->setText(m_ocrKeywords);
}

void AlarmWindow::checkForAlarm()
{
    appendLog("Checking For Alarm......\n发起监测......");
    // MainWindowHelper::h_closeCrashWindows(m_crashKeywords); // 已实现在MWH
    closeCrashWindows();
    restartGame();
    if (!checkWindowsExist()) {
        appendLog("微信或游戏窗口不存在，终止本轮监测！");
        ui->statusLabel->setText("微信或游戏窗口不存在，终止本轮监测！");
        return;
    }
    appendLog("Trying to capture the game......\n(发起截图操作)......");

    QString ocrResult, ocrResult2;
    QImage pic1, pic2, screenShot;
    QString sentText1, sentText2, detectedKeywords1, detectedKeywords2;

    if (!GameMonitor::analyzeGameWindow(m_gameWindowHwnd, ocrResult, ocrResult2, pic1, pic2, screenShot)) {
        appendLog("Fail to capture or recognize!\n截图或 OCR 失败!");
        return;
    }
    ui->image1Label->setPixmap(QPixmap::fromImage(pic1));
    ui->image2Label->setPixmap(QPixmap::fromImage(pic2));

    appendLog("OCR result (OCR识别到文字): " + ocrResult + " " + ocrResult2);
    int tries = 0;
    while (ocrResult2.isEmpty() && tries < 10) {
        clickGameCenterAndKeyL();
        appendLog("侦测到部落日志未打开，已执行自动打开部落日志");
        if (!GameMonitor::analyzeGameWindow(m_gameWindowHwnd, ocrResult, ocrResult2, pic1, pic2, screenShot)) {
            appendLog("Fail to capture or recognize!\n截图或 OCR 失败!");
            return;
        }
        ui->image1Label->setPixmap(QPixmap::fromImage(pic1));
        ui->image2Label->setPixmap(QPixmap::fromImage(pic2));
        if (!ocrResult2.isEmpty()) break;
        tries++;
        if (tries == 10) {
            appendLog("checkForAlarm: 无法打开部落日志");
            return;
        }
    }
    if (!ocrResult2.isEmpty()) {
        QStringList keywords = m_ocrKeywords.split(",", Qt::SkipEmptyParts);
        for (int i = 0; i < keywords.size(); ++i)
            keywords[i] = keywords[i].trimmed();

        bool alarmTriggered_P = false;
        bool alarmTriggered_L = false;

        if (!ocrResult.isEmpty()) {
            for (const QString &keyword : keywords) {
                if (!keyword.isEmpty() && ocrResult.contains(keyword)) {
                    alarmTriggered_P = true;
                    detectedKeywords1 = keyword;
                    appendLog("Detected keywords in Parasaurolophus Alert(检测到副栉龙警报中存在关键字): " + keyword);
                    break;
                }
            }
        } else {ocrResult = "";}
        for (const QString &keyword : keywords) {
            if (!keyword.isEmpty() && ocrResult2.contains(keyword)) {
                alarmTriggered_L = true;
                detectedKeywords2 = keyword;
                appendLog("Detected keywords in Tribe Log (检测到部落日志中存在关键字): " + keyword);
                break;
            }
        }
        if (alarmTriggered_P || alarmTriggered_L) {
            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            sentText1 = "";
            sentText2 = "";
            if (alarmTriggered_P)
                sentText1 = QString("[%1] \n(Key: %2)\n----------------------------------\nParasaurolophus has detected the enemy!\n您的副栉龙已发现敌人，请尽快上线查看！\n[%3]").arg(timeStamp).arg(detectedKeywords1).arg(ocrResult);
            if (alarmTriggered_L)
                sentText2 = QString("[%1] \n(Key: %2)\n----------------------------------\nKeywords in the tribe log: %2 \n侦测到部落日志存在关键词：%2 请您尽快上线查看！\n[%3]").arg(timeStamp).arg(detectedKeywords2).arg(ocrResult2);
            sendAlarm(sentText1, sentText2, ui->sendMessageCheckBox->isChecked(), ui->makeCallCheckBox->isChecked(), alarmTriggered_P, alarmTriggered_L, m_wechatWindowHwnd, m_gameWindowHwnd, weChatDotX, weChatDotY, ui->isGroupCheckBox->isChecked(), m_callMember);
        }
    }
}

void AlarmWindow::updateOverlayPosition_Slider(int value)
{
    if (!m_overlayWindow || !m_wechatWindowHwnd){

        appendLog("修改无效：覆盖层未显示或尚未选择微信窗口！");
        ui->horizontalSlider_X->setValue(weChatDotX);
        ui->horizontalSlider_Y->setValue(weChatDotY);
        return;
    }
    RECT rect;
    if(GetWindowRect(m_wechatWindowHwnd, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        m_overlayWindow->setGeometry(rect.left, rect.top, width, height);
        // 更新红点位置由用户通过 spinbox 设置
        weChatDotX = ui->horizontalSlider_X->value();
        weChatDotY = ui->horizontalSlider_Y->value();
        m_overlayWindow->setRedDotPosition(weChatDotX, weChatDotY);
    }
    qDebug() << "侦测到滑块变动：" << value;
    appendLog("微信窗口坐标已变更");
}

void AlarmWindow::overlayVisibilityChanged(int state)
{
    if (state == Qt::Checked) {
        if (!m_overlayWindow) {
            m_overlayWindow = new OverlayWindow(m_wechatWindowHwnd, this);
        }
        m_overlayWindow->show();
        appendLog("显示微信覆盖层");
    }
    else {
        if(m_overlayWindow) {
            m_overlayWindow->hide();
            delete m_overlayWindow;
            m_overlayWindow = nullptr;
            appendLog("隐藏微信覆盖层");
        }
    }
}

void AlarmWindow::gameOverlayVisibilityChanged(int state)
{
    if (state == Qt::Checked) {
        if (!m_overlayWindow) {
            m_overlayWindow = new OverlayWindow(m_gameWindowHwnd, this);
        }
        m_overlayWindow->show();
        appendLog("显示游戏覆盖层");
    }
    else {
        if(m_overlayWindow) {
            m_overlayWindow->hide();
            delete m_overlayWindow;
            m_overlayWindow = nullptr;
            appendLog("隐藏游戏覆盖层");
        }
    }
}

void AlarmWindow::updateMonitoringCycle()
{
    int time = ui->monitoringCycleSpinBox->value();
    if(m_monitorTimer) {
        if (m_monitorTimer->isActive()) {
            m_monitorTimer->stop();
            monitoringCycle = time;
            m_monitorTimer->start(monitoringCycle);
            appendLog(QString("监测周期已更新为 %1 ms").arg(monitoringCycle));
        }
        else {
            appendLog("监测未启动 无法修改");
        }
        appendLog(QString("Timer已初始化，已修改周期为 %1 ms并重启Timer").arg(monitoringCycle));
    }
    else {
        monitoringCycle = time;
        appendLog(QString("Timer未初始化，仅修改周期为 %1 ms").arg(monitoringCycle));
    }
}

void AlarmWindow::testImagePaste(){
    QImage weChatSC;
    if (!GameMonitor::CaptureWindowWithPrintWindow(m_wechatWindowHwnd, weChatSC))
        qDebug() << "测试发送微信图片时截图失败！";
    MotionSimulator::sendTextAlarm("测试文本", m_wechatWindowHwnd);

    MotionSimulator::sendImageAlarm(weChatSC, m_wechatWindowHwnd);
    ui->image2Label->setPixmap(QPixmap::fromImage(weChatSC));
    qDebug() << "测试发送微信图片成功！";
}


bool AlarmWindow::sendAlarm(const QString &text1, const QString &text2, const bool &sendText, const bool &makeCall, const bool &detected_P, const bool &detected_L, HWND WCHwnd, HWND GAMEHwmd, int X, int Y, bool isGroup ,QString callMember) {
    QImage screenShot;
    if(detected_P && detected_L){
        if(sendText) {
            MotionSimulator::sendTextAlarm(text1, WCHwnd);
            MotionSimulator::sendTextAlarm(text2, WCHwnd);
            GameMonitor::CaptureWindowWithPrintWindow(GAMEHwmd, screenShot);
            MotionSimulator::sendImageAlarm(screenShot, WCHwnd);
        }
        if(makeCall) {
            if(isGroup) {
                MotionSimulator::SimulateLeftClick(WCHwnd, X, Y);
                MotionSimulator::sendGroupCall(callMember);
            }
            else {
                MotionSimulator::SimulateLeftClick(WCHwnd, X, Y);
            }
        }
        return true;
    }
    else if(detected_P){
        if(sendText) {
            MotionSimulator::sendTextAlarm(text1, WCHwnd);
            GameMonitor::CaptureWindowWithPrintWindow(GAMEHwmd, screenShot);
            MotionSimulator::sendImageAlarm(screenShot, WCHwnd);
        }
        if(makeCall) {
            if(isGroup) {
                MotionSimulator::SimulateLeftClick(WCHwnd, X, Y);
                MotionSimulator::sendGroupCall(callMember);
            }
            else {
                MotionSimulator::SimulateLeftClick(WCHwnd, X, Y);
            }
        }
        return true;
    }
    else if(detected_L){
        if(sendText) {
            MotionSimulator::sendTextAlarm(text2, WCHwnd);
            GameMonitor::CaptureWindowWithPrintWindow(GAMEHwmd, screenShot);
            MotionSimulator::sendImageAlarm(screenShot, WCHwnd);
        }
        if(makeCall) {
            if(isGroup) {
                MotionSimulator::SimulateLeftClick(WCHwnd, X, Y);
                MotionSimulator::sendGroupCall(callMember);
            }
            else {
                MotionSimulator::SimulateLeftClick(WCHwnd, X, Y);
            }
        }
        return true;
    }
    return false;
}

void AlarmWindow::testAlarm() {
    appendLog("正在发起Alarm测试");

    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    appendLog(QString("时间戳已生成: %1 !").arg(timeStamp));
    QString sentText1 = QString("[%1] \n(Key: %2)\n----------------------------------\nParasaurolophus has detected the enemy!\n您的副栉龙已发现敌人，请尽快上线查看！\n[%3]").arg(timeStamp).arg("[副栉龙Key]").arg("副栉龙TEXT");
    appendLog(QString("副栉龙警报信息已生成: %1 !").arg(sentText1));
    QString sentText2 = QString("[%1] \n(Key: %2)\n----------------------------------\nKeywords in the tribe log: %2 \n侦测到部落日志存在关键词：%2 请您尽快上线查看！\n[%3]").arg(timeStamp).arg("[部落日志Key]").arg("部落日志TEXT");
    appendLog(QString("部落日志警报信息已生成: %1 !").arg(sentText2));
    sendAlarm(sentText1, sentText2, ui->sendMessageCheckBox->isChecked(), ui->makeCallCheckBox->isChecked(), true, true, m_wechatWindowHwnd, m_gameWindowHwnd, weChatDotX, weChatDotY, ui->isGroupCheckBox->isChecked(), m_callMember);
    appendLog("已调用sendAlarm函数！");
}

void AlarmWindow::updateSliderRanges()
{
    // 如果微信窗口句柄有效，则更新滑块范围
    if (!m_wechatWindowHwnd || !m_gameWindowHwnd) {
        // qDebug() << "微信或ASA窗口句柄无效";
        return;
    }

    RECT rect, rect2;
    if (GetWindowRect(m_wechatWindowHwnd, &rect)) {
        int winWidth = rect.right - rect.left;
        int winHeight = rect.bottom - rect.top;

        // 对于滑块，假设X滑块控制水平方向的坐标，最大值设为窗口宽度，
        // Y滑块控制垂直方向的坐标，最大值设为窗口高度

        ui->horizontalSlider_X->setMaximum(winWidth - 20);
        ui->horizontalSlider_Y->setMaximum(winHeight - 20);

        // qDebug() << QString("更新微信按钮坐标范围: 宽=%1, 高=%2").arg(winWidth).arg(winHeight);
    }
    if (GetWindowRect(m_gameWindowHwnd, &rect2)) {
        int winWidth = rect2.right - rect2.left;
        int winHeight = rect2.bottom - rect2.top;

        // 对于滑块，假设X滑块控制水平方向的坐标，最大值设为窗口宽度，
        // Y滑块控制垂直方向的坐标，最大值设为窗口高度

        ui->horizontalSlider_X_2->setMaximum(winWidth - 20);
        ui->horizontalSlider_Y_2->setMaximum(winHeight - 20);

        // qDebug() << QString("更新游戏按钮坐标范围: 宽=%1, 高=%2").arg(winWidth).arg(winHeight);
    }
}

void AlarmWindow::updateCallMember() {
    m_callMember = ui->callMemberTextEdit->toPlainText().trimmed();
    appendLog(QString("Update CallMember as: %1 \n 更新CallMember为: %1").arg(m_callMember));
}

void AlarmWindow::findAndBindWindows()
{
    std::wstring g = m_gameWindowTitle.toStdWString();
    std::wstring w = m_wechatWindowTitle.toStdWString();
    m_gameWindowHwnd   = ::FindWindowW(nullptr, g.c_str());
    m_wechatWindowHwnd = ::FindWindowW(nullptr, w.c_str());
}

void AlarmWindow::clickGameCenterAndKeyL() // 用MotionSimulator中的函数代替掉
{
    RECT r;
    ::GetWindowRect(m_gameWindowHwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    // 点击
    MotionSimulator::SimulateLeftClick(m_gameWindowHwnd, cx, cy);
    // 按键 L
    WORD vk = 'L';
    MotionSimulator::simulateKey(vk);
}

bool AlarmWindow::restartGame() // 已弃用
{
    // 如果游戏窗口已存在，则不重启
    if (checkWindowsExist()) {
        appendLog("restartGame: 游戏正在运行，无法被重启！");
        return false;
    }

    // 尝试通过 Steam 协议启动游戏
    appendLog("restartGame: 游戏未检测到，正在尝试启动……");
    QDesktopServices::openUrl(QUrl("steam://rungameid/2399830"));

    // 启动后等待窗口出现，最长等待 120 秒
    const int timeoutMs = 120000;
    QElapsedTimer timer;
    timer.start();
    appendLog("restartGame: 等待游戏窗口出现（最多 120 秒）……");


    while (timer.elapsed() < timeoutMs) {
        QThread::msleep(20000);              // 每20 s 检查一次
        QCoreApplication::processEvents(); // 保证 UI 不假死
        if (checkWindowsExist()) {
            // 找到后绑定句柄并返回成功
            findAndBindWindows();
            appendLog("restartGame: 侦测到游戏已启动并绑定句柄，重启成功！");
            return true;
        }
    }

    // 超时未出现
    appendLog("restartGame: 等待超时，未检测到游戏窗口，重启失败！");
    return false;
}

void AlarmWindow::restartGame(std::function<void()> next) {
    // 如果游戏窗口已存在，则不重启
    if (checkWindowsExist()) {
        appendLog("restartGame: 游戏正在运行，无法被重启！");
        return;
    }

    // 尝试通过 Steam 协议启动游戏
    appendLog("restartGame: 游戏未检测到，正在尝试启动……");
    QDesktopServices::openUrl(QUrl("steam://rungameid/2399830"));

    // 启动后等待窗口出现，最长等待 120 秒
    const int timeoutMs = 120000;
    appendLog("rejoin: 等待窗口出现，最长等待 120 秒…");

    // 创建一个堆上的定时器和计时器
    auto *poller = new QTimer(this);
    auto *timer  = new QElapsedTimer();
    timer->start();

    // 每 20 秒触发一次
    poller->setInterval(20000);
    connect(poller, &QTimer::timeout, this, [this, timer, poller, next]() {

        bool found = checkWindowsExist();
        if (found || timer->elapsed() >= timeoutMs) {
            poller->stop();
            appendLog(QString("rejoin: %1")
                          .arg(found ? "游戏窗口已找到" : "等待超时, 未找到游戏窗口, 重启失败！"));
            poller->deleteLater();
            delete timer;

            // 若找到则回调后续逻辑
            if(found) {
                findAndBindWindows(); // 绑定句柄
                next();
            }
        }
    });

    poller->start();
}

void AlarmWindow::closeCrashWindows()
{
    // 1. 优先检查 Shooter Crash Reporter 窗口
    HWND shooterReporter = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
    if (shooterReporter) {
        appendLog("检测到 Shooter Crash Reporter 崩溃窗口，准备点击 “send and close” 按钮…");
        // TODO：将下面坐标替换为实际的按钮坐标
        RECT r;
        ::GetWindowRect(shooterReporter, &r);
        int x = (r.right - r.left);
        int y = (r.bottom - r.top);
        int sendAndCloseX = static_cast<int>(x * 0.74);  // 按钮相对于窗口左上角的 X 偏移
        int sendAndCloseY = static_cast<int>(y * 0.96);  // 按钮相对于窗口左上角的 Y 偏移
        MotionSimulator::SimulateLeftClick(shooterReporter, sendAndCloseX, sendAndCloseY);
        QTimer::singleShot(500, this, [this]() {
            HWND gameHwnd     = ::FindWindowW(nullptr, L"ArkAscended");
            HWND reporterHwnd = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
            // 如果游戏窗口和报告器都不在了，就结束；否则继续尝试关闭
            if (gameHwnd || reporterHwnd) {
                this->closeCrashWindows();
            }
        });
    }

    // 2. 如果没有 Shooter Crash Reporter，再用原有关键词逻辑关闭其它崩溃框

    // 拆分关键字串
    QStringList phrases = m_crashKeywords.split(",", Qt::SkipEmptyParts);
    QStringList tokens;
    for (auto ph : phrases) {
        ph = ph.trimmed();
        if (ph.isEmpty()) continue;
        tokens << ph;
    }
    tokens.removeDuplicates();

    // 枚举所有顶层窗口，找标题包含任一 token
    struct EnumData { const QStringList *tokens; HWND found; };
    EnumData data{&tokens, nullptr};
    ::EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
        auto &d = *reinterpret_cast<EnumData*>(lparam);
        if (!::IsWindowVisible(hwnd)) return TRUE;
        wchar_t buf[512] = {0};
        ::GetWindowTextW(hwnd, buf, _countof(buf));
        QString title = QString::fromWCharArray(buf).trimmed();
        if (title.isEmpty()) return TRUE;
        for (const QString &t : *d.tokens) {
            if (!t.isEmpty() && title.contains(t, Qt::CaseInsensitive)) {
                d.found = hwnd;
                return FALSE; // 停止枚举
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    // 关闭匹配到的窗口
    if (data.found) {
        wchar_t buf[512] = {0};
        ::GetWindowTextW(data.found, buf, _countof(buf));
        QString winTitle = QString::fromWCharArray(buf);
        appendLog(QString("检测到崩溃弹窗“%1”，准备关闭…").arg(winTitle));
        ::SendMessageW(data.found, WM_CLOSE, 0, 0);
        QTimer::singleShot(500, this, [this]() {
            this->closeCrashWindows();
        });
    }
    else {
        appendLog("未检测到崩溃弹窗");
    }
}

bool AlarmWindow::checkWindowsExist() const
{
    // 转换成宽字符
    std::wstring gameTitleW   = m_gameWindowTitle.toStdWString();
    std::wstring wechatTitleW = m_wechatWindowTitle.toStdWString();

    // 尝试查找两个窗口句柄
    HWND gameHwnd   = ::FindWindowW(nullptr, gameTitleW.c_str());
    HWND wechatHwnd = ::FindWindowW(nullptr, wechatTitleW.c_str());

    // 如果都找到了就返回 true，否则 false
    return (gameHwnd != nullptr) && (wechatHwnd != nullptr);
}

void AlarmWindow::coldRejoin()
{
    // 如果游戏窗口不存在，先尝试重启
    if (!checkWindowsExist()) {
        restartGame([this](){
            const int timeoutMs = 60000;
            appendLog("rejoin: 等待游戏窗口出现（最多 60 秒）…");

            // 创建一个堆上的定时器和计时器
            auto *poller = new QTimer(this);
            auto *timer  = new QElapsedTimer();
            timer->start();

            // 每 9.9 秒触发一次
            poller->setInterval(9900);
            connect(poller, &QTimer::timeout, this, [this, timer, poller]() {
                bool found = GameMonitor::checkForRejoin(m_gameWindowHwnd);
                if (found || timer->elapsed() >= timeoutMs) {
                    poller->stop();
                    appendLog(QString("rejoin: %1")
                                  .arg(found ? "找到重新加入按钮" : "等待超时，未找到重新加入按钮"));
                    poller->deleteLater();
                    delete timer;

                    // 若找到则回调后续逻辑
                    if(found)
                        coldRejoinHelper_2();
                }
            });

            poller->start();
        });
    }
    else {
        const int timeoutMs = 60000;
        appendLog("rejoin: 等待进入游戏按钮出现（最多 60 秒）…");

        // 创建一个堆上的定时器和计时器
        auto *poller = new QTimer(this);
        auto *timer  = new QElapsedTimer();
        timer->start();

        // 每 9.9 秒触发一次
        poller->setInterval(9900);
        connect(poller, &QTimer::timeout, this, [this, timer, poller]() {
            bool found = GameMonitor::checkForRejoin(m_gameWindowHwnd);
            if (found || timer->elapsed() >= timeoutMs) {
                poller->stop();
                appendLog(QString("rejoin: %1")
                              .arg(found ? "找到重新加入按钮" : "等待超时，未找到重新加入按钮"));
                poller->deleteLater();
                delete timer;

                // 若找到则回调后续逻辑
                if(found)
                    coldRejoinHelper_2();
            }
        });

        poller->start();
    }
}

void AlarmWindow::waitForRejoin(std::function<void(bool)> onFinished) // 弃用版本
{
    // 等待“重新加入对局”按钮出现，最多 60 秒
    const int timeoutMs = 60000;
    appendLog("rejoin: 等待游戏窗口出现（最多 60 秒）…");

    // 创建一个堆上的定时器和计时器
    auto *poller = new QTimer(this);
    auto *timer  = new QElapsedTimer();
    timer->start();

    // 每 9.9 秒触发一次
    poller->setInterval(9900);
    connect(poller, &QTimer::timeout, this, [this, timer, poller, onFinished]() {
        bool found = GameMonitor::checkForRejoin(m_gameWindowHwnd);
        if (found || timer->elapsed() >= timeoutMs) {
            poller->stop();
            bool success = found;
            appendLog(QString("rejoin: %1")
                          .arg(success ? "找到重新加入按钮" : "等待超时，未找到重新加入按钮"));
            poller->deleteLater();
            delete timer;

            // 回调后续逻辑
            onFinished(success);
        }
    });

    poller->start();
}

void AlarmWindow::waitForRejoin()
{
    // 等待“重新加入对局”按钮出现，最多 60 秒
    const int timeoutMs = 60000;
    appendLog("rejoin: 等待游戏窗口出现（最多 60 秒）…");

    // 创建一个堆上的定时器和计时器
    auto *poller = new QTimer(this);
    auto *timer  = new QElapsedTimer();
    timer->start();

    // 每 9.9 秒触发一次
    poller->setInterval(9900);
    connect(poller, &QTimer::timeout, this, [this, timer, poller]() {
        bool found = GameMonitor::checkForRejoin(m_gameWindowHwnd);
        if (found || timer->elapsed() >= timeoutMs) {
            poller->stop();
            appendLog(QString("rejoin: %1")
                          .arg(found ? "找到重新加入按钮" : "等待超时，未找到重新加入按钮"));
            poller->deleteLater();
            delete timer;

            // 若找到则回调后续逻辑
            if(found)
                coldRejoinHelper_2();
        }
    });

    poller->start();
}

void AlarmWindow::coldRejoinHelper_2() {
    // 如果找到“重新加入”按钮
    appendLog("rejoin: 找到重新加入按钮，执行后续逻辑…");
    RECT rect;
    if (!GetWindowRect(m_gameWindowHwnd, &rect)) {
        qDebug() << "rejoin: Failed to get window rect";
        return;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    // 找到后执行点击
    MotionSimulator::SimulateLeftClick(m_gameWindowHwnd, static_cast<int>(width / 2), static_cast<int>(height * 43 / 54));
    appendLog("rejoin: 已点击开始游戏按钮");
    QTimer::singleShot(1000, this, [this, width, height]() {
        int rjcX, rjcY;
        if(GameMonitor::checkJoinCard(m_gameWindowHwnd, rjcX, rjcY)) {
            MotionSimulator::SimulateLeftClick(m_gameWindowHwnd, rjcX, rjcY);
            appendLog("已点击加入游戏二级菜单卡片！");
        } else {
            appendLog("未找到加入游戏二级菜单卡片，退出重新加入游戏进程！");
        }
        MotionSimulator::SimulateLeftClick(m_gameWindowHwnd, static_cast<int>(width * 0.9), static_cast<int>(height * 0.83));
        appendLog("已点击重新加入上个对局，等待重新加入游戏，在调用本函数的地方在调用之后等游戏启动（1.5分钟）");
        return;
    });

}

void AlarmWindow::setExpireDateLabel(const QString &date){
    ui->expireDateLabel->setText(QString("到期时间：%1").arg(date));
}

void AlarmWindow::setExpireDate(const QString &dateString)
{
    // 1) 显示在界面上
    ui->expireDateLabel->setText(dateString);

    // 2) 解析成 QDateTime —— 和后端约定格式 “yyyy-MM-dd hh:mm” 或 “yyyy-MM-dd”
    //    这里只做到天级检查：
    m_expireDateTime = QDateTime::fromString(dateString, "yyyy-MM-dd");
    if (!m_expireDateTime.isValid()) {
        // 如果包含时间部分，可以尝试另一种格式：
        m_expireDateTime = QDateTime::fromString(dateString, "yyyy-MM-dd hh:mm");
    }

    // 3) 启动定时器并立即检查一次
    if (m_expireDateTime.isValid()) {
        m_expireTimer->start();
        checkExpiration();
    } else {
        appendLog("setExpireDate: 无法解析到期时间：" + dateString);
    }
}

void AlarmWindow::checkExpiration()
{
    QDateTime now = QDateTime::currentDateTime();
    if (!m_expireDateTime.isValid()) {
        return;
    }
    if (now >= m_expireDateTime) {
        appendLog("检测到到期时间已到或已过，程序即将退出");
        // 停掉所有定时器
        if (m_monitorTimer)   m_monitorTimer->stop();
        if (m_expireTimer)    m_expireTimer->stop();
        // 退出应用
        qApp->quit();
    }
}

