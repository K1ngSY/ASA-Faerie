#include "gamehandler.h"

#include <QString>
#include <QStringList>
#include <windows.h>
gameHandler::gameHandler() {}

bool gameHandler::bindWindows(const QString &title, HWND &windowHwnd)
{
    std::wstring w = title.toStdWString();
    windowHwnd = ::FindWindowW(nullptr, w.c_str());
    if (windowHwnd) return true;
    else return false;
}

bool gameHandler::checkWindowExist(const HWND &windowHwnd)
{
    // 空句柄直接视为不存在
    if (windowHwnd == nullptr) {
        return false;
    }
    // IsWindow 返回非零即句柄对应的窗口当前存在
    return ::IsWindow(windowHwnd) != FALSE;
}

bool gameHandler::checkCrash()
{
    QString m_crashKeywords = "UE4-ShooterGame,UE-ShooterGame,Game has crashed and will close,Crash!,Error,Shooter Crash Reporter";
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

    if (data.found) {
        return true;
    } else {
        return false;
    }
}




