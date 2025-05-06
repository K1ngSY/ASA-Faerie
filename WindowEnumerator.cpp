#include "WindowEnumerator.h"
#include <windows.h>

// 回调函数：枚举窗口
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    int length = GetWindowTextLengthW(hwnd);
    if (length == 0)
        return TRUE; // 忽略无标题窗口

    wchar_t* buffer = new wchar_t[length + 1];
    GetWindowTextW(hwnd, buffer, length + 1);
    QString title = QString::fromWCharArray(buffer);
    delete[] buffer;

    // 只处理可见窗口
    if (IsWindowVisible(hwnd)) {
        std::vector<WindowInfo>* windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
        windows->push_back(WindowInfo{ hwnd, title });
    }
    return TRUE;
}

std::vector<WindowInfo> WindowEnumerator::enumerate() {
    std::vector<WindowInfo> windows;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}
