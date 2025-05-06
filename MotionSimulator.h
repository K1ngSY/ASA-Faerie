#ifndef MOTIONSIMULATOR_H
#define MOTIONSIMULATOR_H

#include <QString>
#include <windows.h>
#include <QImage>
#include <QObject>

class MotionSimulator : public QObject{
public:
    explicit MotionSimulator(QObject *parent = nullptr);

    // 新的 sendTextAlarm 函数使用 OCR 识别文本作为参数
    static void sendTextAlarm(const QString &ocrText, HWND wechatHwnd);

    static void sendImageAlarm(const QImage &screenShot, HWND wechatHwnd);

    static void SimulateLeftClick(HWND targetHwnd, int targetX, int targetY);

    static void simulateCtrlV();

    static HBITMAP QImageToHBITMAP(const QImage &image);

    static void pasteText(const QString &text);

    static void simulateKey(WORD vk);

    static void pasteImage(const QImage &img);

    static void sendGroupCall(const QString &targetIndices);

signals:
    void finsh();
};

#endif // MOTIONSIMULATOR_H
