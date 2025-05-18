#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overlaywindow.h"
#include "WindowSelectionDialog.h"
#include "alarm.h"
#include "gamehandler.h"
#include "motionsimulator.h"
#include "alarmsender.h"
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_overlayWindow(nullptr),
    m_alarmPlatformHwnd(nullptr),
    m_gameHwnd(nullptr),
    m_alarmPlatformTitle(""),
    m_alarmWorker(nullptr),
    m_alarmThread(nullptr),
    m_sliderUpdateTimer(nullptr),
    m_CrashHandler(nullptr),
    m_crashHandlerThread(nullptr),
    m_RejoinProcessor(nullptr),
    m_rejoinProcessorThread(nullptr),
    m_expireDateTime(QDateTime()),
    m_ExpirationDetector(nullptr),
    m_expirationDetectorThread(nullptr),
    m_dot_X(20),
    m_dot_Y(20),
    m_serverMonitor(nullptr),
    m_serverMonitorThread(nullptr),
    m_alarmSender(nullptr),
    m_alarmSenderThread(nullptr),
    m_serverComboBoxUpdated(false),
    m_playerNumberAlarmSent(false)
{
    ui->setupUi(this);
    // 注意 本线程启动前必须赋值两个参数 apTitle, apHwnd
    m_alarmWorker = new Alarm;
    m_alarmThread = new QThread(this);
    m_alarmWorker->moveToThread(m_alarmThread);
    // 本线程启动不需要任何参数
    m_CrashHandler = new crashHandler;
    m_crashHandlerThread = new QThread(this);
    m_CrashHandler->moveToThread(m_crashHandlerThread);
    // 注意 本线程启动需要一个参数 gameHwnd
    m_RejoinProcessor = new RejoinProcessor;
    m_rejoinProcessorThread = new QThread(this);
    m_RejoinProcessor->moveToThread(m_rejoinProcessorThread);
    // 注意 本线程启动需要一个参数 QDateTimer
    m_ExpirationDetector = new ExpirationDetector;
    m_expirationDetectorThread = new QThread(this);
    m_ExpirationDetector->moveToThread(m_expirationDetectorThread);

    // —— 初始化 ServerMonitor 线程 ——
    m_serverMonitor = new ServerMonitor;
    m_serverMonitorThread = new QThread(this);
    m_serverMonitor->moveToThread(m_serverMonitorThread);

    m_alarmSender = new AlarmSender;
    m_alarmSenderThread = new QThread(this);
    m_alarmSender->moveToThread(m_alarmSenderThread);

    // 滑块范围刷新timer
    m_sliderUpdateTimer = new QTimer(this);
    ui->image1Label->setScaledContents(true);
    ui->image2Label->setScaledContents(true);

    // Buttons
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startMonitoring);
    connect(ui->selectButton, &QPushButton::clicked, this, &MainWindow::selectWindow);
    connect(ui->closeMonitoringButton, &QPushButton::clicked, this, &MainWindow::stopMonitoring);
    connect(ui->updateCallMemberButton, &QPushButton::clicked, this, &MainWindow::updateCallMember);
    connect(ui->severCodepushButton, &QPushButton::clicked, this, &MainWindow::updateServerCode);

    // Overlay visibility
    connect(ui->overlayVisibleCheckBox, &QCheckBox::checkStateChanged, this, &MainWindow::overlayVisibilityChanged);

    // Alarm options
    connect(ui->isGroupCheckBox,    &QCheckBox::checkStateChanged, m_alarmWorker, &Alarm::updateGroupStatus);
    connect(ui->sendMessageCheckBox,&QCheckBox::checkStateChanged, m_alarmWorker, &Alarm::updateTextAlarmStatus);
    connect(ui->makeCallCheckBox,   &QCheckBox::checkStateChanged, m_alarmWorker, &Alarm::updateCallAlarmStatus);

    // Alarm worker signals
    connect(m_alarmWorker, &Alarm::gotPic_1,       this, &MainWindow::updateImage1);
    connect(m_alarmWorker, &Alarm::gotPic_2,       this, &MainWindow::updateImage2);
    connect(m_alarmWorker, &Alarm::alarm,          this, &MainWindow::onAlarmSent);
    connect(m_alarmWorker, &Alarm::gameCrashed,    this, &MainWindow::onGameCrashed);
    connect(m_alarmWorker, &Alarm::findWindowsFailed, this, &MainWindow::windowFailurehandler);
    connect(m_alarmWorker, &Alarm::warning,        this, &MainWindow::warningHandler);
    connect(m_alarmWorker, &Alarm::logMessage,     this, &MainWindow::transferLog);
    connect(m_alarmWorker, &Alarm::gameTimeout,    this, &MainWindow::handleGameSessionDisconnected);
    connect(m_alarmWorker, &Alarm::finished,       this, &MainWindow::onWorkerFinished);
    // connect(m_alarmWorker, &Alarm::finished,       m_alarmThread, &QThread::quit);
    connect(m_alarmThread, &QThread::finished,     m_alarmWorker, &QObject::deleteLater);
    connect(m_alarmThread, &QThread::finished,     m_alarmThread, &QObject::deleteLater);

    // Slider & timer
    connect(m_sliderUpdateTimer, &QTimer::timeout, this, &MainWindow::updateSliderRanges);
    connect(ui->horizontalSlider_X, &QSlider::sliderMoved, this, &MainWindow::updateOverlayPosition_Slider);
    connect(ui->horizontalSlider_Y, &QSlider::sliderMoved, this, &MainWindow::updateOverlayPosition_Slider);

    // Crash handler
    connect(m_CrashHandler, &crashHandler::logMessage, this, &MainWindow::transferLog);
    connect(m_CrashHandler, &crashHandler::finished_0, this, &MainWindow::setGameHwnd);
    connect(m_CrashHandler, &crashHandler::finished_0, this, &MainWindow::onCHFinished_0);
    connect(m_CrashHandler, &crashHandler::finished_1, this, &MainWindow::onCHFinished_1);
    connect(m_crashHandlerThread, &QThread::finished, m_CrashHandler, &QObject::deleteLater);
    connect(m_crashHandlerThread, &QThread::finished, m_crashHandlerThread, &QObject::deleteLater);

    // Rejoin processor
    connect(m_RejoinProcessor, &RejoinProcessor::logMessage, this, &MainWindow::transferLog);
    connect(m_RejoinProcessor, &RejoinProcessor::finished_0, this, &MainWindow::onRPFinished_0);
    connect(m_RejoinProcessor, &RejoinProcessor::finished_1, this, &MainWindow::onRPFinished_1);
    connect(m_rejoinProcessorThread, &QThread::finished, m_RejoinProcessor, &QObject::deleteLater);
    connect(m_rejoinProcessorThread, &QThread::finished, m_rejoinProcessorThread, &QObject::deleteLater);

    // Expiration detector
    connect(m_ExpirationDetector, &ExpirationDetector::expirated, this, &MainWindow::onExpired);
    connect(m_ExpirationDetector, &ExpirationDetector::logMessage, this, &MainWindow::transferLog);

    // 连接启动信号
    connect(this, &MainWindow::turnOnMonitor, m_alarmWorker, &Alarm::startMonitoring);
    connect(this, &MainWindow::turnOffMonitor, m_alarmWorker, &Alarm::stopMonitoring);
    connect(this, &MainWindow::turnOnExpirationDetector, m_ExpirationDetector, &ExpirationDetector::start);
    connect(this, &MainWindow::turnOffExpirationDetector, m_ExpirationDetector, &ExpirationDetector::stop);
    connect(this, &MainWindow::turnOnCrashHandler, m_CrashHandler, &crashHandler::startRestartGame);
    connect(this, &MainWindow::turnOnRejoinProcessor, m_RejoinProcessor, &RejoinProcessor::startRejoin);

    // 服务器监控器(启动后每 5 秒拉一次)
    connect(this, &MainWindow::turnOnServerMonitor, m_serverMonitor, &ServerMonitor::start);
    connect(m_serverMonitor, &ServerMonitor::serversFetched, this, &MainWindow::onServersFetched);

    //多线程警报发送机连接(AlarmSender)
    connect(this, &MainWindow::playerNumberAlarm, m_alarmSender, &AlarmSender::sendText);

    // Test
    connect(ui->testAlarmpushButton, &QPushButton::clicked, this, &MainWindow::testAlarm);
    connect(this, &MainWindow::startTestAlarm, m_alarmWorker, &Alarm::testFunc);

    m_sliderUpdateTimer->start(200);
    m_alarmThread->start();
    m_crashHandlerThread->start();
    m_rejoinProcessorThread->start();
    m_expirationDetectorThread->start();
    m_alarmSenderThread->start();
    m_serverMonitorThread->start();

    ui->thresholdSpinBox->setValue(70);
}

MainWindow::~MainWindow()
{
    // 1. 清理覆盖层窗口，防止未删除导致内存泄漏
    if (m_overlayWindow) {
        m_overlayWindow->hide();
        delete m_overlayWindow;
        m_overlayWindow = nullptr;
    }

    // 2. 停止并删除滑动条更新定时器
    if (m_sliderUpdateTimer) {
        m_sliderUpdateTimer->stop();
        delete m_sliderUpdateTimer;
        m_sliderUpdateTimer = nullptr;
    }

    // 3. 停止并删除告警线程和工作对象
    if (m_alarmThread) {
        m_alarmThread->quit();    // 请求线程退出
        m_alarmThread->wait();    // 等待线程彻底结束
        // delete m_alarmThread;
        m_alarmThread = nullptr;
    }
    if (m_alarmWorker) {
        m_alarmWorker->deleteLater();
        // delete m_alarmWorker;
        m_alarmWorker = nullptr;
    }

    // 4. 停止并删除崩溃处理线程和对象
    if (m_crashHandlerThread) {
        m_crashHandlerThread->quit();
        m_crashHandlerThread->wait();
        // delete m_crashHandlerThread;
        m_crashHandlerThread = nullptr;
    }
    if (m_CrashHandler) {
        m_CrashHandler->deleteLater();
        // delete m_CrashHandler;
        m_CrashHandler = nullptr;
    }

    // 5. 停止并删除重连处理线程和对象
    if (m_rejoinProcessorThread) {
        m_rejoinProcessorThread->quit();
        m_rejoinProcessorThread->wait();
        // delete m_rejoinProcessorThread;
        m_rejoinProcessorThread = nullptr;
    }
    if (m_RejoinProcessor) {
        m_RejoinProcessor->deleteLater();
        // delete m_RejoinProcessor;
        m_RejoinProcessor = nullptr;
    }

    // 6. 停止并删除到期检测线程和对象
    if (m_expirationDetectorThread) {
        m_expirationDetectorThread->quit();
        m_expirationDetectorThread->wait();
        // delete m_expirationDetectorThread;
        m_expirationDetectorThread = nullptr;
    }
    if (m_ExpirationDetector) {
        m_ExpirationDetector->deleteLater();
        //delete m_ExpirationDetector;
        m_ExpirationDetector = nullptr;
    }

    // 7. —— 清理 ServerMonitor 线程 ——
    if (m_serverMonitor) {
        m_serverMonitor->stop();
        m_serverMonitorThread->quit();
        m_serverMonitorThread->wait();
        m_serverMonitor->deleteLater();
        m_serverMonitorThread->deleteLater();
        m_serverMonitor = nullptr;
        m_serverMonitorThread = nullptr;
    }

    if(m_alarmSender) {
        m_alarmSenderThread->quit();
        m_alarmSenderThread->wait();
        m_alarmSender->deleteLater();
        m_alarmSenderThread->deleteLater();
        m_alarmSender = nullptr;
        m_alarmSenderThread = nullptr;
    }

    // 8. 最后删除 UI，释放所有子控件
    delete ui;
}

void MainWindow::setExpireDate(const QString &dateString)
{
    // 1) 显示在界面上
    ui->expireDateLabel->setText(QString("到期时间：%1").arg(dateString));

    // 2) 解析成 QDateTime —— 和后端约定格式 “yyyy-MM-dd hh:mm” 或 “yyyy-MM-dd”
    //    这里只做到天级检查：
    m_expireDateTime = QDateTime::fromString(dateString, "yyyy-MM-dd");
    if (!m_expireDateTime.isValid()) {
        // 如果包含时间部分，可以尝试另一种格式：
        m_expireDateTime = QDateTime::fromString(dateString, "yyyy-MM-dd hh:mm");
    }
    m_ExpirationDetector->setExpDateTime(m_expireDateTime);
}

void MainWindow::startDectecExpiration()
{
    emit turnOnExpirationDetector();
}

void MainWindow::appendLog(const QString &message)
{
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logLine = QString("[%1] %2").arg(timeStamp).arg(message);
    qDebug() << logLine;
    if(ui->logTextEdit)
        ui->logTextEdit->append(logLine);
}

void MainWindow::selectWindow()
{
    appendLog("开始选择窗口……");
    ui->statusLabel->setText("正在执行窗口选择……");
    // 选择微信窗口
    WindowSelectionDialog wechatDialog(this);
    wechatDialog.setWindowTitle("选择微信窗口");
    wechatDialog.setLabel1Text("选择微信窗口");
    if (wechatDialog.exec() == QDialog::Accepted) {
        auto wechatWin = wechatDialog.selectedWindow();
        m_alarmPlatformHwnd = wechatWin.hwnd;
        m_alarmPlatformTitle = wechatWin.title;  // 保存窗口标题
        appendLog(QString("选定微信窗口：%1 句柄：%2").arg(wechatWin.title).arg((qulonglong)m_alarmPlatformHwnd));
    } else {
        appendLog("未选择微信窗口");
        QMessageBox::warning(this, "提示", "未选择微信窗口");
        return;
    }
    m_alarmWorker->setAPHwnd(m_alarmPlatformHwnd);
    m_alarmWorker->setAPTitle(m_alarmPlatformTitle);
    appendLog("窗口选择完成");
    ui->statusLabel->setText("窗口选择完成！");
    // QMessageBox::information(this, "提示", "窗口选择完成");
    emit turnOnServerMonitor();
    appendLog("已启动服务器检测器");
}

void MainWindow::startMonitoring()
{
    if(!m_alarmPlatformTitle.isEmpty() && m_alarmPlatformHwnd && gameHandler::checkWindowExist(m_alarmPlatformHwnd)) {
        m_alarmWorker->setAPTitle(m_alarmPlatformTitle);
        m_alarmWorker->setAPHwnd(m_alarmPlatformHwnd);
        emit turnOnMonitor();
        ui->statusLabel->setText("监控已启动！");
    }
    else {
        QMessageBox::warning(this, "Warning!", "请先选择微信窗口再启动监控！");
    }
}

void MainWindow::stopMonitoring()
{
    if (m_alarmThread) {

        // m_alarmWorker->stop();
        emit turnOffMonitor();
        qDebug() << "监控线程存在，尝试终止Timer";
        ui->statusLabel->setText("监控已停止！");
    }
    else appendLog("监控不存在 无法终止！");
}

void MainWindow::transferLog(const QString &message)
{
    appendLog(message);
}

void MainWindow::onWorkerFinished()
{
    appendLog("已响应监控线程发出的结束信号！");
}

void MainWindow::updateOverlayPosition_Slider(int value)
{
    if (!m_overlayWindow || !m_alarmPlatformHwnd){

        appendLog("修改无效：覆盖层未显示或尚未选择微信窗口！");
        ui->horizontalSlider_X->setValue(m_dot_X);
        ui->horizontalSlider_Y->setValue(m_dot_Y);
        return;
    }
    RECT rect;
    if(GetWindowRect(m_alarmPlatformHwnd, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        m_overlayWindow->setGeometry(rect.left, rect.top, width, height);
        // 更新红点位置由用户通过 spinbox 设置
        m_dot_X = ui->horizontalSlider_X->value();
        m_dot_Y = ui->horizontalSlider_Y->value();
        m_overlayWindow->setRedDotPosition(m_dot_X, m_dot_Y);
        m_alarmWorker->setCallButtonPosition(m_dot_X, m_dot_Y);
    }
    qDebug() << "侦测到滑块变动：" << value;
    // appendLog("微信窗口坐标已变更");
}

void MainWindow::updateSliderRanges()
{
    // 如果微信窗口句柄有效，则更新滑块范围
    if (!m_alarmPlatformHwnd) {
        return;
    }

    RECT rect;
    if (GetWindowRect(m_alarmPlatformHwnd, &rect)) {
        int winWidth = rect.right - rect.left;
        int winHeight = rect.bottom - rect.top;

        // 对于滑块，假设X滑块控制水平方向的坐标，最大值设为窗口宽度，
        // Y滑块控制垂直方向的坐标，最大值设为窗口高度

        ui->horizontalSlider_X->setMaximum(winWidth - 20);
        ui->horizontalSlider_Y->setMaximum(winHeight - 20);

        // qDebug() << QString("更新微信按钮坐标范围: 宽=%1, 高=%2").arg(winWidth).arg(winHeight);
    }
}

void MainWindow::updateCallMember()
{
    m_alarmWorker->updateCallMember(ui->callMemberlineEdit->text());
}

void MainWindow::updateImage1(const QImage &pic)
{
    ui->image1Label->setPixmap(QPixmap::fromImage(pic));
}

void MainWindow::updateImage2(const QImage &pic)
{
    ui->image2Label->setPixmap(QPixmap::fromImage(pic));
}

void MainWindow::onAlarmSent(const QString &p_ocrResult_1, const QString &p_ocrResult_2, const QString &p_key_1, const QString &p_key_2)
{
    QString text = QString("警报已生成：\n关键词1:%1\n关键词2：%2\n原文1：%3\n原文2：%4").arg(p_key_1).arg(p_key_2).arg(p_ocrResult_1).arg(p_ocrResult_2);
    appendLog(text);
}

void MainWindow::windowFailurehandler(char w)
{
    if (w == 'G') {
        appendLog("找不到窗口：游戏窗口\n监控终止！");
        this->stopMonitoring();
        emit turnOnCrashHandler();
    } else if (w == 'A') {
        appendLog("找不到窗口：微信窗口\n监控终止！");
        this->stopMonitoring();
    } else {
        appendLog("找不到窗口：未知错误");
    }
}

void MainWindow::warningHandler(const QString &msg)
{
    QMessageBox::warning(this, "Warning!", msg);
}

void MainWindow::setGameHwnd(HWND gameHwnd)
{
    m_gameHwnd = gameHwnd;
}

void MainWindow::onCHFinished_0(HWND gameHwnd)
{
    // m_crashHandlerThread->quit();
    appendLog(QString("选定游戏句柄：%1").arg((qulonglong)gameHwnd));
    m_gameHwnd = gameHwnd;
    this->startRejoin();
    appendLog("开始加入上个对局！");
}

void MainWindow::onCHFinished_1()
{
    // m_crashHandlerThread->quit();
    appendLog(QString("处理游戏崩溃失败 请手动处理！"));
}

void MainWindow::onGameCrashed()
{
    stopMonitoring();
    emit turnOnCrashHandler();
}

void MainWindow::onRPFinished_0()
{
    // m_rejoinProcessorThread->quit();
    appendLog("重启游戏成功 即将启动监控！");
    this->startMonitoring();
}

void MainWindow::onRPFinished_1()
{
    // m_rejoinProcessorThread->quit();
    appendLog("游戏失败请手动修复 监控不会启动！");
}

void MainWindow::handleGameSessionDisconnected()
{
    this->stopMonitoring();
    MotionSimulator::clickGameCenterAndEsc(m_gameHwnd);
    this->startRejoin();
}

void MainWindow::onExpired()
{
    ui->statusLabel->setText("卡密已过期 程序即将关闭！");
    emit turnOffExpirationDetector();
    this->stopMonitoring();
    qApp->quit();
}

void MainWindow::testAlarm()
{
    if (!m_alarmPlatformHwnd) {
        appendLog("testAlarm: 未选择微信窗口，无法测试警报");
    } else {
        emit startTestAlarm();
    }
}

void MainWindow::updateServerCode()
{   QString code = ui->severCodelineEdit->text();
    if (!code.isEmpty()) {
        m_RejoinProcessor->setServerCode(code);
        appendLog(QString("updateServerCode：got new code: %1").arg(code));
    }
}

void MainWindow::startRejoin()
{
    m_RejoinProcessor->setHwnd(m_gameHwnd);
    emit turnOnRejoinProcessor();
    appendLog("开始重新加入游戏！");
}

void MainWindow::overlayVisibilityChanged(int state)
{
    if(m_alarmPlatformHwnd) {
        if (state == Qt::Checked) {
            if (!m_overlayWindow) {
                m_overlayWindow = new OverlayWindow(m_alarmPlatformHwnd, this);
                connect(m_overlayWindow, &OverlayWindow::overlayUpdated, this, &MainWindow::updateSliderRanges);
            }
            m_overlayWindow->show();
            appendLog("显示微信覆盖层");
        }
        else {
            if(m_overlayWindow) {
                m_overlayWindow->hide();
                disconnect(m_overlayWindow, &OverlayWindow::overlayUpdated, this, &MainWindow::updateSliderRanges);
                delete m_overlayWindow;
                m_overlayWindow = nullptr;
                appendLog("隐藏微信覆盖层");
            }
        }
    }
    else {
        appendLog("未选择微信窗口，请选择后重试");
        return;
    }
}


void MainWindow::onServersFetched(const QList<ServerInfo> &servers)
{
    // 0) 排序表格
    QList<ServerInfo> sortedServers = servers;
    int size = sortedServers.size();
    bool swapped = false;
    for (int i = 0; i < size - 1; i++) {
        swapped = false;
        for (int f = 0; f < size - i - 1; f++) {
            if(sortedServers[f].sessionName > sortedServers[f + 1].sessionName) {
                sortedServers.swapItemsAt(f, f + 1);
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }

    // 1) 更新表格
    ui->serverTableWidget->clearContents();
    // ui->serverTableWidget->setColumnCount(2);
    ui->serverTableWidget->setHorizontalHeaderLabels(QStringList{"Players", "Session"});
    ui->serverTableWidget->setRowCount(sortedServers.size());
    for (int i = 0; i < sortedServers.size(); ++i) {
        ui->serverTableWidget->setItem(i, 0,
                                       new QTableWidgetItem(QString::number(sortedServers[i].numPlayers)));
        ui->serverTableWidget->setItem(i, 1,
                                       new QTableWidgetItem(sortedServers[i].sessionName));
    }
    ui->serverTableWidget->resizeColumnsToContents();

    // 2) 更新下拉列表（单选）
    if (!m_serverComboBoxUpdated) {
        ui->serverComboBox->clear();
        for (auto &info : sortedServers)
            ui->serverComboBox->addItem(info.sessionName);
        m_serverComboBoxUpdated = true;
    }

    // 3) 检测阈值
    if (!m_playerNumberAlarmSent) {
        QString selected = ui->serverComboBox->currentText();
        int threshold = ui->thresholdSpinBox->value();
        for (auto &info : servers) {
            if (info.sessionName == selected && info.numPlayers > threshold) {
                QString msg = QString("服务器 %1 当前人数 %2，已超出阈值 %3")
                                  .arg(info.sessionName)
                                  .arg(info.numPlayers)
                                  .arg(threshold);
                emit playerNumberAlarm(msg, m_alarmPlatformHwnd);
                emit sentPlayerNumberAlarm();
                m_playerNumberAlarmSent = true;
                break;
            }
        }
    } else {
        appendLog("服务器监控警报冷却中!");
    }
}

void MainWindow::blockPlayerNumberAlarm()
{
    QTimer *timer = new QTimer();
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [this,timer]() {
        m_playerNumberAlarmSent = false;
        if (m_playerNumberAlarmSent) appendLog("服务器监控警报冷却已清除!");
        timer->deleteLater(); // 手动释放内存
    });
    timer->start(300000);
}

