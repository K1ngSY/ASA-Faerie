#ifndef VISUALPROCESSOR_H
#define VISUALPROCESSOR_H

#include <QObject>
#include <QImage>
#include <QString>
#include <windows.h>
#include <cfloat>
#include <opencv2/opencv.hpp>

class VisualProcessor
{
public:
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
    static bool printWindow(HWND hwnd, QImage &image);

    static bool checkJoinCard(HWND Hwnd, int &Val_X, int &Val_Y);

    static bool checkStartButton(HWND Hwnd);


signals:
    void logMessage(const QString &msg);
};

#endif // VISUALPROCESSOR_H
