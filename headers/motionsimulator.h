#ifndef MOTIONSIMULATOR_H
#define MOTIONSIMULATOR_H

#include <QImage>
#include <QString>
#include <windows.h>

class MotionSimulator
{
public:
    MotionSimulator();

    static void LeftClick(HWND targetHwnd, int targetX, int targetY);

    static void simulateKey(WORD vk);

    static void CtrlV();

    static void clickGameCenterAndKeyL(HWND windowHwnd);

    static void pasteImage(const QImage &img);

    static HBITMAP QImageToHBITMAP(const QImage &image);

    static void pasteText(const QString &text);

    static void sendTextAlarm(const QString &ocrText, HWND wechatHwnd);

    static void sendImageAlarm(const QImage &screenShot, HWND wechatHwnd);

    static void sendGroupCall(const QString &targetIndices);

    static void clickGameCenterAndEsc(HWND Hwnd);

    static void WheelScroll(int verticalDelta);
};

#endif // MOTIONSIMULATOR_H
