#ifndef GAMEMONITOR_H
#define GAMEMONITOR_H

#include <QImage>
#include <QString>
#include <windows.h>
#include <opencv2/opencv.hpp>

class GameMonitor {
public:
    // 截图三个 QImage（全屏、ROI1、ROI2）
    static bool captureWindowImage(HWND hwnd,
                                   QImage &roi1,
                                   QImage &roi2,
                                   QImage &fullImage);

    // 对 roi1（蓝通道）和 roi2（红通道）做 OCR
    static bool ocrImage(const QImage &inputImage1,
                         const QImage &inputImage2,
                         QString &recognizedText1,
                         QString &recognizedText2);

    // 一体化接口
    static bool analyzeGameWindow(HWND hwnd,
                                  QString &ocrResult1,
                                  QString &ocrResult2,
                                  QImage &pic1,
                                  QImage &pic2,
                                  QImage &fullImage);

    // 辅助转换
    static cv::Mat QImageToCvMat(const QImage &inImage);
    static QImage CvMatToQImage(const cv::Mat &inMat);

    // PrintWindow API 截图
    static bool CaptureWindowWithPrintWindow(HWND hwnd, QImage &image);

    static bool checkForRejoin(HWND Hwnd);

    static bool checkJoinCard(HWND Hwnd, int &X, int&Y);

    static bool checkJoinLast(HWND Hwnd);
};

#endif // GAMEMONITOR_H
