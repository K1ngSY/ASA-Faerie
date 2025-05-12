#include "motionsimulator.h"
#include <QDebug>
#include <QString>
#include <QThread>

MotionSimulator::MotionSimulator() {}

void MotionSimulator::sendGroupCall(const QString &targetIndices)
{
    // 根据固定标题 "微信选择成员" 获取该窗口的句柄
    std::wstring winTitle = QStringLiteral("微信选择成员").toStdWString();
    HWND groupPopupHwnd = FindWindowW(nullptr, winTitle.c_str());
    if (!groupPopupHwnd) {
        qDebug() << "sendGroupCall: Unable to find window titled '微信选择成员'";
        return;
    }
    // 将群聊弹出窗口置前
    SetForegroundWindow(groupPopupHwnd);
    QThread::msleep(100);

    // 解析用户输入的目标号码，假设输入格式为 "1,3,5"（1~10之间数字，以英文逗号分隔）
    QStringList targetList = targetIndices.split(",", Qt::SkipEmptyParts);
    for (const QString &numStr : targetList) {
        bool ok = false;
        int index = numStr.trimmed().toInt(&ok);
        if (!ok || index < 1 || index > 10) {
            qDebug() << "sendGroupCall: Invalid target index:" << numStr;
            continue;
        }
        // 计算对应条形区域的点击坐标
        // 条形列表参数：总条高度 50 像素, 第一条距离顶部 9 像素, 每条高度为 50, 中心 y = 9 + (index-1)*50 + 25
        // 水平坐标：假设列表宽度为 331 像素，则取中间 331/2 = 165.5, 约 166
        int x = 166;
        int y = 20 + (index - 1) * 50 + 25;
        qDebug() << "sendGroupCall: Clicking target bar" << index << "at (" << x << "," << y << ")";
        LeftClick(groupPopupHwnd, x, y);
        QThread::msleep(100);
    }
    // 模拟点击确认按钮，确定呼叫
    // 按钮坐标固定为 (474, 501)（相对于弹出窗口客户区）
    qDebug() << "sendGroupCall: Clicking confirm button at (474,501)";
    LeftClick(groupPopupHwnd, 474, 515);
    qDebug() << "sendGroupCall: Group call action completed.";
}

// Helper: 模拟单个按键，使用花括号初始化
void MotionSimulator::simulateKey(WORD vk)
{
    INPUT input{};  // 全部初始化为0
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    SendInput(1, &input, sizeof(INPUT));

    INPUT inputUp{};
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = vk;
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputUp, sizeof(INPUT));
}

void MotionSimulator::LeftClick(HWND targetHwnd, int targetX, int targetY) {
    // 移动光标到目标位置
    RECT rect;
    GetWindowRect(targetHwnd, &rect);
    SetForegroundWindow(targetHwnd);
    SetCursorPos(rect.left + targetX, rect.top + targetY);

    // Sleep(100); // 稍等一会，确保光标已移动
    QThread::msleep(100);

    // 模拟鼠标按下
    INPUT mouseDown{};
    mouseDown.type = INPUT_MOUSE;
    mouseDown.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &mouseDown, sizeof(INPUT));

    // Sleep(50); // 模拟点击持续时间
    QThread::msleep(50);

    // 模拟鼠标松开
    INPUT mouseUp{};
    mouseUp.type = INPUT_MOUSE;
    mouseUp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &mouseUp, sizeof(INPUT));
}

void MotionSimulator::pasteImage(const QImage &img)
{
    // 将图像转换为 Format_RGB32（无 alpha）以适应 CF_DIB 格式
    QImage image = img.convertToFormat(QImage::Format_RGB32);
    int width = image.width();
    int height = image.height();
    int bytesPerLine = image.bytesPerLine();

    // 构造 BITMAPINFOHEADER，注意对于 CF_DIB，通常使用正高度（表示底部为第一行）
    BITMAPINFOHEADER bih;
    ZeroMemory(&bih, sizeof(bih));
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height;  // 正值表示底部在前（DIB标准格式）
    bih.biPlanes = 1;
    bih.biBitCount = 32;    // 32位格式
    bih.biCompression = BI_RGB;
    bih.biSizeImage = bytesPerLine * height;

    // 总内存大小 = header + 像素数据
    int totalSize = sizeof(BITMAPINFOHEADER) + bih.biSizeImage;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, totalSize);
    if (!hGlobal) {
        qDebug() << "pasteImage: GlobalAlloc failed";
        return;
    }
    void *pGlobal = GlobalLock(hGlobal);
    if (!pGlobal) {
        qDebug() << "pasteImage: GlobalLock failed";
        GlobalFree(hGlobal);
        return;
    }
    // 将 BITMAPINFOHEADER 复制到全局内存块
    memcpy(pGlobal, &bih, sizeof(bih));
    // 像素数据存放在 header 之后
    void *pPixels = static_cast<char*>(pGlobal) + sizeof(bih);
    memcpy(pPixels, image.bits(), bih.biSizeImage);
    GlobalUnlock(hGlobal);

    // 打开剪贴板并清空
    if (!OpenClipboard(NULL)) {
        qDebug() << "pasteImage: Unable to open clipboard";
        GlobalFree(hGlobal);
        return;
    }
    EmptyClipboard();
    // 设置剪贴板数据为 CF_DIB 格式
    if (!SetClipboardData(CF_DIB, hGlobal)) {
        qDebug() << "pasteImage: SetClipboardData failed";
        CloseClipboard();
        GlobalFree(hGlobal);
        return;
    }
    // 成功后剪贴板拥有内存块，不需手动释放
    CloseClipboard();

    // 模拟 Ctrl+V 粘贴操作
    CtrlV();
}

void MotionSimulator::CtrlV()
{
    INPUT inputs[4] = {};
    // Ctrl 按下
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    // V 按下
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    // V 释放
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    // Ctrl 释放
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
    QThread::msleep(100);
}

void MotionSimulator::clickGameCenterAndKeyL(HWND windowHwnd) // 用MotionSimulator中的函数代替掉
{
    RECT r;
    ::GetWindowRect(windowHwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    // 点击
    LeftClick(windowHwnd, cx, cy);
    // 按键 L
    WORD vk = 'L';
    simulateKey(vk);
}
