#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overlaywindow.h"
#include "WindowSelectionDialog.h"
#include "alarm.h"
#include "gamehandler.h"
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
    m_dot_Y(20)
{
    ui->setupUi(this);
    // 注意 本线程启动前必须赋值两个参数 apTitle, apHwnd
    m_alarmWorker = new Alarm;
    m_alarmThread = new QThread(this);
    m_alarmWorker->moveToThread(m_alarmThread);

    m_sliderUpdateTimer = new QTimer(this);
    ui->image1Label->setScaledContents(true);
    ui->image2Label->setScaledContents(true);
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


    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startMonitoring);
    connect(ui->selectButton, &QPushButton::clicked, this, &MainWindow::selectWindow);
    connect(ui->closeMonitoringButton, &QPushButton::clicked, this, &MainWindow::stopMonitoring);
    connect(ui->overlayVisibleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(gameOverlayVisibilityChanged(int)));
    connect(ui->isGroupCheckBox, SIGNAL(checkStateChanged(Qt::CheckState state)), m_alarmWorker, SLOT(updateGroupStatus(Qt::CheckState)));
    connect(ui->sendMessageCheckBox, SIGNAL(checkStateChanged(Qt::CheckState state)), m_alarmWorker, SLOT(updateTextAlarmStatus(Qt::CheckState)));
    connect(ui->makeCallCheckBox, SIGNAL(checkStateChanged(Qt::CheckState state)), m_alarmWorker, SLOT(updateCallAlarmStatus(Qt::CheckState)));
    connect(m_alarmWorker, SIGNAL(gotPic_1(QImage)), this, SLOT(updateImage1(QImage)));
    connect(m_alarmWorker, SIGNAL(gotPic_2(QImage)), this, SLOT(updateImage2(QImage)));
    connect(m_alarmWorker, SIGNAL(alarm(QString,QString,QString,QString)), this, SLOT(onAlarmSent(QString,QString,QString,QString)));
    // 连接 游戏崩溃处理
    connect(m_alarmWorker, &Alarm::gameCrashed, this, &MainWindow::onGameCrashed);
    connect(m_alarmWorker, SIGNAL(findWindowsFailed(char)), this, SLOT(windowFailurehandler(char)));
    connect(m_alarmWorker, SIGNAL(warning(QString)), this, SLOT(warningHandler(QString)));
    connect(m_alarmWorker, SIGNAL(logMessage(QString)), this, SLOT(transferLog(QString)));
    connect(m_alarmWorker, &Alarm::gameTimeout, this, &MainWindow::handleGameSessionDisconnected);
    connect(m_alarmWorker, &Alarm::finished, this, &MainWindow::onWorkerFinished);
    connect(m_alarmWorker, &Alarm::finished, m_alarmThread, &QThread::quit);
    connect(m_alarmThread, &QThread::finished, m_alarmWorker, &QObject::deleteLater);
    connect(m_alarmThread, &QThread::finished, m_alarmThread, &QObject::deleteLater);

    connect(m_sliderUpdateTimer, &QTimer::timeout, this, &MainWindow::updateSliderRanges);

    connect(m_CrashHandler, SIGNAL(logMessage(QString)), this, SLOT(transferLog(QString)));
    connect(m_CrashHandler, SIGNAL(finished_0(HWND)), this, SLOT(setGameHwnd(HWND)));
    connect(m_CrashHandler, SIGNAL(finished_0(HWND)), this, SLOT(onCHFinished_0(HWND)));
    connect(m_CrashHandler, &crashHandler::finished_1, this, &MainWindow::onCHFinished_1);
    connect(m_crashHandlerThread, &QThread::finished, m_CrashHandler, &QObject::deleteLater);
    connect(m_crashHandlerThread, &QThread::finished, m_crashHandlerThread, &QObject::deleteLater);

    connect(m_RejoinProcessor, SIGNAL(logMessage(QString)), this, SLOT(transferLog(QString)));
    connect(m_RejoinProcessor, &RejoinProcessor::finished_0, this, &MainWindow::onRPFinished_0);
    connect(m_RejoinProcessor, &RejoinProcessor::finished_1, this, &MainWindow::onRPFinished_1);
    connect(m_rejoinProcessorThread, &QThread::finished, m_RejoinProcessor, &QObject::deleteLater);
    connect(m_rejoinProcessorThread, &QThread::finished, m_rejoinProcessorThread, &QObject::deleteLater);









    connect(ui->horizontalSlider_X, SIGNAL(sliderMoved(int)), this, SLOT(updateOverlayPosition_Slider(int)));
    connect(ui->horizontalSlider_Y, SIGNAL(sliderMoved(int)), this, SLOT(updateOverlayPosition_Slider(int)));
    m_sliderUpdateTimer->start(100);
}

MainWindow::~MainWindow()
{
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
    /*
    // 3) 启动定时器并立即检查一次
    if (m_expireDateTime.isValid()) {
        m_expireTimer->start();
        checkExpiration();
    } else {
        appendLog("setExpireDate: 无法解析到期时间：" + dateString);
    }
    */
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
    appendLog("窗口选择完成");
    ui->statusLabel->setText("窗口选择完成！");
    QMessageBox::information(this, "提示", "窗口选择完成");
}

void MainWindow::startMonitoring()
{
    if(!m_alarmPlatformTitle.isEmpty() && m_alarmPlatformHwnd && gameHandler::checkWindowExist(m_alarmPlatformHwnd)) {
        if (!m_alarmThread) m_alarmThread = new QThread(this);
        if (!m_alarmWorker) m_alarmWorker = new Alarm;
        if (!m_alarmThread->isRunning()) m_alarmThread->start();
        m_alarmWorker->setAPTitle(m_alarmPlatformTitle);
        m_alarmWorker->setAPHwnd(m_alarmPlatformHwnd);
        m_alarmWorker->startMonitoring();
        ui->statusLabel->setText("监控已启动！");
    }
    else {
        QMessageBox::warning(this, "Warning!", "请先选择微信窗口再启动监控！");
    }
}

void MainWindow::stopMonitoring()
{
    m_alarmWorker->stop();
    ui->statusLabel->setText("监控已停止！");
}

void MainWindow::transferLog(const QString &message)
{
    appendLog(message);
}

void MainWindow::onWorkerFinished()
{
    m_alarmWorker = nullptr;
    m_alarmThread = nullptr;
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
    }
    qDebug() << "侦测到滑块变动：" << value;
    appendLog("微信窗口坐标已变更");
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

void MainWindow::setGameHwnd(HWND gameHwnd)
{
    m_gameHwnd = gameHwnd;
}

void MainWindow::onCHFinished_0(HWND gameHwnd)
{
    m_crashHandlerThread->quit();
    appendLog(QString("选定游戏句柄：%1").arg((qulonglong)gameHwnd));
    m_gameHwnd = gameHwnd;
    m_CrashHandler = nullptr;
    m_crashHandlerThread = nullptr;
    startRejoin();
    appendLog("开始加入上个对局！");
}

void MainWindow::onCHFinished_1()
{
    m_crashHandlerThread->quit();
    appendLog(QString("处理游戏崩溃失败 请手动处理！"));
    m_CrashHandler = nullptr;
    m_crashHandlerThread = nullptr;
}

void MainWindow::onGameCrashed()
{
    stopMonitoring();
    if (!m_crashHandlerThread) new QThread(this);
    if (!m_CrashHandler)
    m_CrashHandler->restartGame();
}

void MainWindow::onRPFinished_0()
{
    m_rejoinProcessorThread->quit();
    appendLog("重启游戏成功 即将启动监控！");
    m_RejoinProcessor = nullptr;
    m_rejoinProcessorThread = nullptr;
    this->startMonitoring();
}

void MainWindow::onRPFinished_1()
{

}

void MainWindow::handleGameSessionDisconnected()
{

}

void MainWindow::startRejoin()
{
    m_RejoinProcessor->setHwnd(m_gameHwnd);
    m_rejoinProcessorThread->start();
    m_RejoinProcessor->startRejoin();
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
