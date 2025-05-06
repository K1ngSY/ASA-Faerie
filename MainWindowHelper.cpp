#include "MainWindowHelper.h"
#include <windows.h>
#include "MotionSimulator.h"
#include <QThread>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <shellapi.h>
#include <QElapsedTimer>
#include <QThread>
#include <QCoreApplication>

void MainWindowHelper::h_closeCrashWindows(const QString &m_crashKeywords)
{
    // 1. 优先检查 Shooter Crash Reporter 窗口
    HWND shooterReporter = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
    if (shooterReporter) {
        // appendLog("检测到 Shooter Crash Reporter 崩溃窗口，准备点击 “send and close” 按钮…");
        qDebug() << "检测到 Shooter Crash Reporter 崩溃窗口，准备点击 “send and close” 按钮…";
        // TODO：将下面坐标替换为实际的按钮坐标
        RECT r;
        ::GetWindowRect(shooterReporter, &r);
        int x = (r.right - r.left);
        int y = (r.bottom - r.top);
        int sendAndCloseX = static_cast<int>(x * 0.74);  // 按钮相对于窗口左上角的 X 偏移
        int sendAndCloseY = static_cast<int>(y * 0.96);  // 按钮相对于窗口左上角的 Y 偏移
        MotionSimulator::SimulateLeftClick(shooterReporter, sendAndCloseX, sendAndCloseY);
        QThread::msleep(500);
        HWND gameHwnd = ::FindWindowW(nullptr, L"ArkAscended");
        HWND reporterHwnd = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
        if (!gameHwnd && !reporterHwnd) {return;}
        else {h_closeCrashWindows(m_crashKeywords);}
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
        // appendLog(QString("检测到崩溃弹窗“%1”，准备关闭…").arg(winTitle));
        qDebug() << QString("检测到崩溃弹窗“%1”，准备关闭…").arg(winTitle);
        ::SendMessageW(data.found, WM_CLOSE, 0, 0);
        QThread::msleep(500);
        // 再递归一次，防止残留
        h_closeCrashWindows(m_crashKeywords);
    }
    else {
        // appendLog("未检测到崩溃弹窗");
        qDebug() << "未检测到崩溃弹窗";
    }
}

bool MainWindowHelper::h_restartGame()
{
    // 尝试通过 Steam 协议启动游戏
    // appendLog("restartGame: 游戏未检测到，正在尝试启动……");
    qDebug() << "h_restartGame: 游戏未检测到，正在尝试启动……";
    QDesktopServices::openUrl(QUrl("steam://rungameid/2399830"));

    // 启动后等待窗口出现，最长等待 120 秒
    const int timeoutMs = 120000;
    QElapsedTimer timer;
    timer.start();
    // appendLog("restartGame: 等待游戏窗口出现（最多 120 秒）……");
    qDebug() << "h_restartGame: 等待游戏窗口出现（最多 120 秒）……";

    while (timer.elapsed() < timeoutMs) {
        QThread::msleep(20000);              // 每20 s 检查一次
        QCoreApplication::processEvents(); // 保证 UI 不假死

        if (h_checkWindowsExist()) {
            // 找到后返回 true 剩下的任务交回主窗进程处理
            return true;
        }
    }

    // 超时未出现
    // appendLog("restartGame: 等待超时，未检测到游戏窗口，重启失败！");
    return false;
}

bool MainWindowHelper::h_checkWindowsExist()
{
    // 转换成宽字符
    QString gameTitle = "ArkAscended";
    std::wstring gameTitleW   = gameTitle.toStdWString();

    // 尝试查找两个窗口句柄
    HWND gameHwnd   = ::FindWindowW(nullptr, gameTitleW.c_str());

    // 如果找到了就返回 true，否则 false
    return gameHwnd != nullptr;
}

