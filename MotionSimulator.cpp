#include "MotionSimulator.h"
#include <windows.h>
#include <QString>
#include <QDebug>
#include <QThread>
#include <winuser.h>
#include <shellapi.h>
#include <QTimer>
MotionSimulator::MotionSimulator(QObject *parent) : QObject(parent) {
    qDebug() << "MotionSimulator 对象已创建！";
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

// Helper: 将文本放入剪贴板并模拟 Ctrl+V 粘贴
void MotionSimulator::pasteText(const QString &text)
{
    if (!OpenClipboard(NULL)) {
        qDebug() << "pasteText: Unable to open clipboard";
        return;
    }
    EmptyClipboard();
    // 使用 utf16() 获取指向 UTF-16 数据的指针
    const ushort *data = text.utf16();
    // 计算字节数：每个字符 2 字节，加上终止符
    int byteSize = (text.size() + 1) * sizeof(ushort);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, byteSize);
    if (!hMem) {
        CloseClipboard();
        qDebug() << "pasteText: GlobalAlloc failed";
        return;
    }
    void *pMem = GlobalLock(hMem);
    memcpy(pMem, data, byteSize);
    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();

    // 模拟 Ctrl+V 组合键
    INPUT inputs[4]{};
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
}

// 新的 sendAlarm 实现
// 1. 将微信窗口置前；
// 2. 获取窗口尺寸，计算目标点击位置：X为窗口水平中心，Y为窗口顶部加90%的高度；
// 3. 模拟鼠标点击目标位置；
// 4. 粘贴 OCR 识别到的完整文本；
// 5. 模拟回车键发送消息。
void MotionSimulator::sendTextAlarm(const QString &ocrText, HWND wechatHwnd)
{
    if (!wechatHwnd) {
        qDebug() << "WeChatSimulator: Invalid WeChat window handle";
        return;
    }
    // 将微信窗口置前
    SetForegroundWindow(wechatHwnd);
    // 获取窗口尺寸
    RECT rect;
    if (!GetWindowRect(wechatHwnd, &rect)) {
        qDebug() << "WeChatSimulator: Failed to get window rect";
        return;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int targetX = rect.left + width / 2;
    int targetY = rect.top + static_cast<int>(height * 0.9); // 90%位置
    // 模拟鼠标点击
    SimulateLeftClick(wechatHwnd, targetX, targetY);
    // 粘贴 OCR 文本
    pasteText(ocrText);
    QTimer::singleShot(500, [](){
        // 模拟按下回车键
        simulateKey(VK_RETURN);
        qDebug() << "WeChatSimulator: Message sent";
    });
}

void MotionSimulator::SimulateLeftClick(HWND targetHwnd, int targetX, int targetY) {
    // 移动光标到目标位置
    RECT rect;
    GetWindowRect(targetHwnd, &rect);
    SetForegroundWindow(targetHwnd);
    SetCursorPos(rect.left + targetX, rect.top + targetY);

    // Sleep(100); // 稍等一会，确保光标已移动

    // 模拟鼠标按下
    INPUT mouseDown{};
    mouseDown.type = INPUT_MOUSE;
    mouseDown.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &mouseDown, sizeof(INPUT));

    // Sleep(50); // 模拟点击持续时间
    // 模拟鼠标松开
    INPUT mouseUp{};
    mouseUp.type = INPUT_MOUSE;
    mouseUp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &mouseUp, sizeof(INPUT));
}

void MotionSimulator::simulateCtrlV()
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
    // QThread::msleep(100);
}

HBITMAP MotionSimulator::QImageToHBITMAP(const QImage &image)
{
    // 确保图像为 32 位格式
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    int width = img.width();
    int height = img.height();

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 负值表示顶端在前
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;    // 32 位
    bmi.bmiHeader.biCompression = BI_RGB;

    // CreateDIBSection 创建一个 DIB 区域，并返回 HBITMAP
    void *bits = nullptr;
    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);

    if (!hBitmap || !bits) {
        qDebug() << "QImageToHBITMAP: CreateDIBSection failed";
        return NULL;
    }

    // 计算图像占用字节数
    int sizeInBytes = img.bytesPerLine() * img.height();

    // 将 QImage 数据复制到 DIB 内存中
    memcpy(bits, img.bits(), sizeInBytes);
    return hBitmap;
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
    simulateCtrlV();
}

void MotionSimulator::sendImageAlarm(const QImage &screenShot, HWND wechatHwnd)
{
    if (!wechatHwnd) {
        qDebug() << "WeChatSimulator: Invalid WeChat window handle";
        return;
    }
    // 将微信窗口置前
    SetForegroundWindow(wechatHwnd);
    // 获取窗口尺寸
    RECT rect;
    if (!GetWindowRect(wechatHwnd, &rect)) {
        qDebug() << "WeChatSimulator: Failed to get window rect";
        return;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int targetX = rect.left + width / 2;
    int targetY = rect.top + static_cast<int>(height * 0.9); // 90%位置
    // 模拟鼠标点击
    SimulateLeftClick(wechatHwnd, targetX, targetY);
    // 粘贴 OCR 文本
    pasteImage(screenShot);
    // Sleep(200);
    // 模拟按下回车键
    simulateKey(VK_RETURN);
    qDebug() << "WeChatSimulator: Image sent";
}

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
    // Sleep(500);

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
        SimulateLeftClick(groupPopupHwnd, x, y);
        // QThread::msleep(100);
    }
    // 模拟点击确认按钮，确定呼叫
    // 按钮坐标固定为 (474, 501)（相对于弹出窗口客户区）
    qDebug() << "sendGroupCall: Clicking confirm button at (474,501)";
    SimulateLeftClick(groupPopupHwnd, 474, 515);
    qDebug() << "sendGroupCall: Group call action completed.";
}

