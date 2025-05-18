#include "motionsimulator.h"
#include <QDebug>
#include <QString>
#include <QThread>

MotionSimulator::MotionSimulator() {}

void MotionSimulator::sendGroupCall(const QString &targetIndices)
{
    //等待弹窗完全出现
    QThread::msleep(300);

    // 1) 找到“微信选择成员”对话框
    std::wstring title = QStringLiteral("微信选择成员").toStdWString();
    HWND dlg = FindWindowW(nullptr, title.c_str());
    if (!dlg) {
        qDebug() << "sendGroupCall: 无法找到“微信选择成员”窗口";
        return;
    }
    SetForegroundWindow(dlg);
    QThread::msleep(50);

    // 2) 获取当前屏幕分辨率，计算横/纵缩放比例
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    double scaleX = screenW / 1920.0;
    double scaleY = screenH / 1080.0;

    // 3) 解析用户输入的成员索引并依次点击
    QStringList list = targetIndices.split(",", Qt::SkipEmptyParts);
    for (const QString &s : list) {
        bool ok = false;
        int idx = s.trimmed().toInt(&ok);
        if (!ok || idx < 1 || idx > 10) {
            qDebug() << "sendGroupCall: 无效索引" << s;
            continue;
        }
        // 基准下第 i 项点击位置 (相对对话框客户区)：x=166, y=20+(i-1)*50+25
        int baseX = 166;
        int baseY = 20 + (idx - 1) * 50 + 25;
        // 按比例缩放
        int x = int(baseX * scaleX);
        int y = int(baseY * scaleY);

        qDebug() << "sendGroupCall: 点击成员" << idx << "，坐标=(" << x << "," << y << ")";
        LeftClick(dlg, x, y);
        QThread::msleep(50 + rand()%150);
    }

    // 4) 点击“确定”按钮（基准坐标 474,515）
    int btnX = int(474 * scaleX);
    int btnY = int(515 * scaleY);
    qDebug() << "sendGroupCall: 点击 确定 按钮，坐标=(" << btnX << "," << btnY << ")";
    LeftClick(dlg, btnX, btnY);

    qDebug() << "sendGroupCall: 群呼完成";
/////////////////////////////////////////////////////////////////////////////////////////////
    // QThread::msleep(300);
    // // 根据固定标题 "微信选择成员" 获取该窗口的句柄
    // std::wstring winTitle = QStringLiteral("微信选择成员").toStdWString();
    // HWND groupPopupHwnd = FindWindowW(nullptr, winTitle.c_str());
    // if (!groupPopupHwnd) {
    //     qDebug() << "sendGroupCall: Unable to find window titled '微信选择成员'";
    //     return;
    // } else {
    //     qDebug() << "sendGroupCall: 已获取选择成员窗口句柄";
    // }
    // // 将群聊弹出窗口置前
    // SetForegroundWindow(groupPopupHwnd);
    // QThread::msleep(10);

    // // 解析用户输入的目标号码，假设输入格式为 "1,3,5"（1~10之间数字，以英文逗号分隔）
    // QStringList targetList = targetIndices.split(",", Qt::SkipEmptyParts);
    // for (const QString &numStr : targetList) {
    //     bool ok = false;
    //     int index = numStr.trimmed().toInt(&ok);
    //     if (!ok || index < 1 || index > 10) {
    //         qDebug() << "sendGroupCall: Invalid target index:" << numStr;
    //         continue;
    //     }
    //     // 计算对应条形区域的点击坐标
    //     // 条形列表参数：总条高度 50 像素, 第一条距离顶部 9 像素, 每条高度为 50, 中心 y = 9 + (index-1)*50 + 25
    //     // 水平坐标：假设列表宽度为 331 像素，则取中间 331/2 = 165.5, 约 166
    //     int x = 166;
    //     int y = 20 + (index - 1) * 50 + 25;
    //     qDebug() << "sendGroupCall: Clicking target bar" << index << "at (" << x << "," << y << ")";
    //     LeftClick(groupPopupHwnd, x, y);
    //     QThread::msleep(10);
    // }
    // // 模拟点击确认按钮，确定呼叫
    // // 按钮坐标固定为 (474, 501)（相对于弹出窗口客户区）
    // qDebug() << "sendGroupCall: Clicking confirm button at (474,501)";
    // LeftClick(groupPopupHwnd, 474, 515);
    // qDebug() << "sendGroupCall: Group call action completed.";
}

void MotionSimulator::clickGameCenterAndEsc(HWND Hwnd)
{
    RECT r;
    ::GetWindowRect(Hwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    // 点击
    LeftClick(Hwnd, cx, cy);
    // 按键 L
    simulateKey(VK_ESCAPE);
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
    SetForegroundWindow(targetHwnd);
    QThread::msleep(100);
    SetForegroundWindow(targetHwnd);
    QThread::msleep(100);
    qDebug() << QString("MotionSimulator::LeftClick: received HWND: %1").arg((qulonglong)targetHwnd);
    // 1) 客户区坐标 → 屏幕坐标
    POINT pt{ targetX, targetY };
    if (!ClientToScreen(targetHwnd, &pt)) {
        qWarning() << "ClientToScreen failed, error=" << GetLastError();
        return;
    }

    // 2) 计算归一化绝对坐标 (0–65535)
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    UINT normX = MulDiv(pt.x, 65535, screenW - 1);
    UINT normY = MulDiv(pt.y, 65535, screenH - 1);

    // 3) 构造鼠标移动 + 点击事件
    INPUT inputs[3] = {};
    inputs[0].type               = INPUT_MOUSE;
    inputs[0].mi.dwFlags         = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    inputs[0].mi.dx              = normX;
    inputs[0].mi.dy              = normY;
    inputs[1].type               = INPUT_MOUSE;
    inputs[1].mi.dwFlags         = MOUSEEVENTF_LEFTDOWN;
    inputs[2].type               = INPUT_MOUSE;
    inputs[2].mi.dwFlags         = MOUSEEVENTF_LEFTUP;

    // 4) 发送给系统
    UINT sent = SendInput(_countof(inputs), inputs, sizeof(INPUT));
    if (sent != _countof(inputs)) {
        qWarning() << "SendInput failed, sent=" << sent;
    }
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




void MotionSimulator::sendTextAlarm(const QString &ocrText, HWND wechatHwnd)
{
    // if (!wechatHwnd) {
    //     qDebug() << "WeChatSimulator: Invalid WeChat window handle";
    //     return;
    // }
    // // 将微信窗口置前
    // SetForegroundWindow(wechatHwnd);
    // // 获取窗口尺寸
    // RECT rect;
    // if (!GetWindowRect(wechatHwnd, &rect)) {
    //     qDebug() << "WeChatSimulator: Failed to get window rect";
    //     return;
    // }
    // int width = rect.right - rect.left;
    // int height = rect.bottom - rect.top;
    // int targetX = rect.left + width / 2;
    // int targetY = rect.top + static_cast<int>(height * 0.9); // 90%位置
    // // 模拟鼠标点击
    // LeftClick(wechatHwnd, targetX, targetY);
    // // 粘贴 OCR 文本
    // pasteText(ocrText);
    // QThread::msleep(500);
    // simulateKey(VK_RETURN);
    // qDebug() << "WeChatSimulator: Message sent";
    if (!wechatHwnd) {
        qDebug() << "WeChatSimulator: Invalid WeChat window handle";
        return;
    }
    SetForegroundWindow(wechatHwnd);
    QThread::msleep(100);
    SetForegroundWindow(wechatHwnd);
    QThread::msleep(100);
    // 1) 获取客户区尺寸
    RECT clientRect;
    if (!GetClientRect(wechatHwnd, &clientRect)) {
        qDebug() << "WeChatSimulator: Failed to get client rect";
        return;
    }
    int width  = clientRect.right  - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // 2) 计算点击位置（90% 处于底端偏上）
    int clientX = width / 2;
    int clientY = static_cast<int>(height * 0.9);

    // 3) 发起点击、粘贴文本、回车
    LeftClick(wechatHwnd, clientX, clientY);
    pasteText(ocrText);
    QThread::msleep(500);
    simulateKey(VK_RETURN);

    qDebug() << "WeChatSimulator: Message sent";
}



void MotionSimulator::sendImageAlarm(const QImage &screenShot, HWND wechatHwnd)
{
    // if (!wechatHwnd) {
    //     qDebug() << "WeChatSimulator: Invalid WeChat window handle";
    //     return;
    // }
    // // 将微信窗口置前
    // SetForegroundWindow(wechatHwnd);
    // // 获取窗口尺寸
    // RECT rect;
    // if (!GetWindowRect(wechatHwnd, &rect)) {
    //     qDebug() << "WeChatSimulator: Failed to get window rect";
    //     return;
    // }
    // int width = rect.right - rect.left;
    // int height = rect.bottom - rect.top;
    // int targetX = rect.left + width / 2;
    // int targetY = rect.top + static_cast<int>(height * 0.9); // 90%位置
    // // 模拟鼠标点击
    // LeftClick(wechatHwnd, targetX, targetY);
    // // 粘贴 OCR 文本
    // pasteImage(screenShot);
    // QThread::msleep(200);
    // // 模拟按下回车键
    // simulateKey(VK_RETURN);
    // qDebug() << "WeChatSimulator: Image sent";
    if (!wechatHwnd) {
        qDebug() << "MotionalSimulator: Invalid WeChat window handle";
        return;
    }
    SetForegroundWindow(wechatHwnd);

    // 1) 获取客户区尺寸
    RECT clientRect;
    if (!GetClientRect(wechatHwnd, &clientRect)) {
        qDebug() << "MotionalSimulator: Failed to get client rect";
        return;
    }
    int width  = clientRect.right  - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // 2) 计算点击位置
    int clientX = width / 2;
    int clientY = static_cast<int>(height * 0.9);

    // 3) 发起点击、粘贴图片、回车
    LeftClick(wechatHwnd, clientX, clientY);
    pasteImage(screenShot);
    QThread::msleep(200);
    simulateKey(VK_RETURN);

    qDebug() << "MotionalSimulator: Image sent";
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

void MotionSimulator::WheelScroll(int verticalDelta)
{
    // 1) 可选：先把鼠标移到目标位置
    //    SetCursorPos(x, y);
    // 2) 构造滚轮事件
    INPUT inp{};
    inp.type                = INPUT_MOUSE;
    inp.mi.dwFlags          = MOUSEEVENTF_WHEEL;
    inp.mi.mouseData        = verticalDelta;    // 一般用 ±WHEEL_DELTA(120)
    inp.mi.dwExtraInfo      = 0;
    inp.mi.time             = 0;
    SendInput(1, &inp, sizeof(inp));
}
