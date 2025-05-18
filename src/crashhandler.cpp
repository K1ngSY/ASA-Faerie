#include "crashhandler.h"
#include "gamehandler.h"
#include "visualprocessor.h"
#include "motionsimulator.h"
#include <QDesktopServices>
#include <QUrl>
#include <QElapsedTimer>
#include <QTimer>
#include <QString>
#include <QThread>

#include <windows.h>

crashHandler::crashHandler(QObject *parent):
    QObject{parent},
    m_ETimer(nullptr),
    m_Timer2(nullptr),
    m_Timer(nullptr),
    m_timeoutMs(120000),
    m_timeoutMsSB(60000),
    m_gameHwnd(nullptr),
    m_crashKeywords("UE4-ShooterGame,UE-ShooterGame,Game has crashed and will close,Crash!,Error,ArkAscended")
{
    connect(this, &crashHandler::gameIsRunning, this, &crashHandler::finishScanGameWindow);
    connect(this, &crashHandler::scanGWTimeOut, this, &crashHandler::finishScanGameWindow);
    connect(this, &crashHandler::scanGWTimeOut, this, &crashHandler::exit_1);
    connect(this, &crashHandler::gameIsReady, this, &crashHandler::finishScanStartButton);
    connect(this, &crashHandler::gameIsReady, this, &crashHandler::exit_0);
    connect(this, &crashHandler::scanSBTimeOut, this, &crashHandler::finishScanStartButton);
    connect(this, &crashHandler::scanSBTimeOut, this, &crashHandler::exit_1);
}

crashHandler::~crashHandler()
{
    // 1. 如果计时器还在运行，先停止它们
    if (m_Timer && m_Timer->isActive()) {
        m_Timer->stop();
    }
    if (m_Timer2 && m_Timer2->isActive()) {
        m_Timer2->stop();
    }

    // 2. QObject 机制会自动 delete 有 parent 的 QTimer，
    //    但为了保险，可以手动断开信号连接并置空指针
    if (m_Timer) {
        disconnect(m_Timer, &QTimer::timeout, this, &crashHandler::scanGameWindow);
        m_Timer = nullptr;
    }
    if (m_Timer2) {
        disconnect(m_Timer2, &QTimer::timeout, this, &crashHandler::scanStartButton);
        m_Timer2 = nullptr;
    }

    // 3. 手动 delete 非-QObject 计时器，避免内存泄漏
    if (m_ETimer) {
        delete m_ETimer;
        m_ETimer = nullptr;
    }

    // 4. 父类析构会处理剩余 QObject 子对象
}

void crashHandler::startRestartGame()
{
    // 尝试关闭所有崩溃窗口
    closeCrashWindows();
    // 通过 Steam 协议启动游戏
    emit logMessage("restartGame: 正在尝试启动……");
    QDesktopServices::openUrl(QUrl("steam://rungameid/2399830"));
    if (!m_ETimer) {
        m_ETimer = new QElapsedTimer;
    }
    if (!m_Timer) {
        m_Timer = new QTimer(this);
        connect(m_Timer, &QTimer::timeout, this, &crashHandler::scanGameWindow);
    }
    if (!m_Timer->isActive()) {
        m_Timer->start(10000);
    }
    if(m_ETimer) {
        m_ETimer->start();
    }
    emit logMessage("restartGame: 等待游戏窗口出现（最多 120 秒）……");
}

void crashHandler::startScanStartButton()
{
    emit logMessage("crashHandler: 开始检测启动游戏按钮 10s 一次（最多 60 秒）");
    if (!m_Timer2) {
        m_Timer2 = new QTimer(this);
        connect(m_Timer2, &QTimer::timeout, this, &crashHandler::scanStartButton);
    }
    if (!m_Timer2->isActive()) {
        m_Timer2->start(10000);
    }
    if(m_ETimer) {
        m_ETimer->start();
    }
}

void crashHandler::closeCrashWindows()
{
    // 1. 优先检查 Shooter Crash Reporter 窗口
    HWND shooterReporter = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
    if (shooterReporter) {
        emit logMessage("检测到 Shooter Crash Reporter 崩溃窗口，准备点击 “send and close” 按钮…");
        // TODO：将下面坐标替换为实际的按钮坐标
        RECT r;
        ::GetWindowRect(shooterReporter, &r);
        int x = (r.right - r.left);
        int y = (r.bottom - r.top);
        int sendAndCloseX = static_cast<int>(x * 0.74);  // 按钮相对于窗口左上角的 X 偏移
        int sendAndCloseY = static_cast<int>(y * 0.96);  // 按钮相对于窗口左上角的 Y 偏移
        MotionSimulator::LeftClick(shooterReporter, sendAndCloseX, sendAndCloseY);
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
        // ph = ph.trimmed();
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
        emit logMessage(QString("检测到崩溃弹窗“%1”，准备关闭…").arg(winTitle));
        ::SendMessageW(data.found, WM_CLOSE, 0, 0);
        QThread::msleep(500);
        this->closeCrashWindows();
    }
    else {
        emit logMessage("未检测到崩溃弹窗");
    }
}

void crashHandler::scanGameWindow()
{
    if (m_ETimer->elapsed() >= m_timeoutMs) {
        emit scanGWTimeOut();
    }
    else {
        gameHandler::bindWindows("ArkAscended", m_gameHwnd);
        if (m_gameHwnd) {
            startScanStartButton();
            emit gameIsRunning();
        }
    }
}

void crashHandler::scanStartButton()
{
    if (m_ETimer->elapsed() >= m_timeoutMsSB) {
        emit scanSBTimeOut();
    }
    else{
        if (VisualProcessor::checkStartButton(m_gameHwnd)) {
            emit gameIsReady();
        }
    }
}

void crashHandler::finishScanGameWindow()
{
    if (m_Timer && m_Timer->isActive()) {
        m_Timer->stop();
        emit logMessage("CrashHandler: finishScanGameWindow: 已暂停Timer");
    } else {
        emit logMessage("CrashHandler: finishScanGameWindow: Tiemr不存在或不在运行！");
    }
}

void crashHandler::finishScanStartButton()
{
    if (m_Timer2 && m_Timer2->isActive()) {
        m_Timer2->stop();
        emit logMessage("CrashHandler: finishScanStartButton: 已暂停Timer");
    } else {
        emit logMessage("CrashHandler: finishScanStartButton: Tiemr不存在或不在运行！");
    }
}

void crashHandler::exit_0()
{
    emit finished_0(m_gameHwnd);
}

void crashHandler::exit_1()
{
    emit finished_1();
}

