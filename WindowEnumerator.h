#ifndef WINDOWENUMERATOR_H
#define WINDOWENUMERATOR_H

#include <vector>
#include <QString>
#include <windows.h>

// 用于保存窗口信息
struct WindowInfo {
    HWND hwnd;
    QString title;
};

class WindowEnumerator {
public:
    // 枚举所有顶级窗口，返回窗口列表
    static std::vector<WindowInfo> enumerate();
};

#endif // WINDOWENUMERATOR_H
