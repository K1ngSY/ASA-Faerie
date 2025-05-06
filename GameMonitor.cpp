#include "GameMonitor.h"

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

#include <float.h>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QImage>
#include <QDebug>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <windows.h>

// OpenCV 头文件
#include <opencv2/opencv.hpp>
// const char *tessdataPath = "C:/msys64/mingw64/share/tessdata";
const char *tessdataPath = "tessdata";
bool GameMonitor::captureWindowImage(HWND hwnd,
                                     QImage &roi1,
                                     QImage &roi2,
                                     QImage &fullImage)
{
    // 1. 用 PrintWindow 捕获全屏
    if (!CaptureWindowWithPrintWindow(hwnd, fullImage)) {
        qDebug() << "captureWindowImage: PrintWindow 失败";
        return false;
    }

    // 2. 计算窗口大小
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        qDebug() << "captureWindowImage: GetWindowRect 失败";
        return false;
    }
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    // 3. 按比例裁剪两个 ROI
    QImage img = fullImage;
    roi1 = img.copy(int(w*0.08),    int(h*0.01203),
                    int(w*0.2661),  int(h*0.02778));
    roi2 = img.copy(int(w*0.39531), int(h*0.17778),
                    int(w*0.209375),int(h*0.584259));

    // 4. 转 cv::Mat 并拆通道
    cv::Mat m1 = QImageToCvMat(roi1);
    cv::Mat m2 = QImageToCvMat(roi2);
    std::vector<cv::Mat> ch1, ch2;
    cv::split(m1, ch1);
    cv::split(m2, ch2);
    if (ch1.size() < 3 || ch2.size() < 3) {
        qDebug() << "captureWindowImage: 通道不足";
        return false;
    }

    // ROI1 用蓝色通道(索引 0)，ROI2 用红色通道(索引 2)
    roi1 = CvMatToQImage(ch1[0]);
    roi2 = CvMatToQImage(ch2[2]);

    return true;
}

bool GameMonitor::ocrImage(const QImage &inputImage1,
                           const QImage &inputImage2,
                           QString &recognizedText1,
                           QString &recognizedText2)
{
    tesseract::TessBaseAPI ocr;
    // 用 mingw64 下的 tessdata 目录

    if (ocr.Init(tessdataPath, "chi_sim+eng")) {
        qDebug() << "ocrImage: Init 失败，请检查 tessdataPath";
        return false;
    }
    ocr.SetPageSegMode(tesseract::PSM_AUTO);

    // 第一幅图
    int bpp1 = inputImage1.depth()/8;
    ocr.SetImage(const_cast<uchar*>(inputImage1.bits()),
                 inputImage1.width(),
                 inputImage1.height(),
                 bpp1,
                 inputImage1.bytesPerLine());
    char *out1 = ocr.GetUTF8Text();
    recognizedText1 = QString::fromUtf8(out1);
    delete[] out1;

    // 第二幅图
    ocr.Clear();
    int bpp2 = inputImage2.depth()/8;
    ocr.SetImage(const_cast<uchar*>(inputImage2.bits()),
                 inputImage2.width(),
                 inputImage2.height(),
                 bpp2,
                 inputImage2.bytesPerLine());
    char *out2 = ocr.GetUTF8Text();
    recognizedText2 = QString::fromUtf8(out2);
    delete[] out2;

    ocr.End();
    return true;
}

bool GameMonitor::analyzeGameWindow(HWND hwnd,
                                    QString &ocrResult1,
                                    QString &ocrResult2,
                                    QImage &pic1,
                                    QImage &pic2,
                                    QImage &fullImage)
{
    if (!captureWindowImage(hwnd, pic1, pic2, fullImage)) {
        return false;
    }
    if (!ocrImage(pic1, pic2, ocrResult1, ocrResult2)) {
        return false;
    }
    return true;
}

cv::Mat GameMonitor::QImageToCvMat(const QImage &inImage)
{
    return cv::Mat(inImage.height(),
                   inImage.width(),
                   CV_8UC4,
                   const_cast<uchar*>(inImage.bits()),
                   inImage.bytesPerLine()).clone();
}

QImage GameMonitor::CvMatToQImage(const cv::Mat &inMat)
{
    if (inMat.type() == CV_8UC1) {
        QImage img(inMat.data, inMat.cols, inMat.rows,
                   inMat.step, QImage::Format_Grayscale8);
        return img.copy();
    } else if (inMat.type() == CV_8UC4) {
        QImage img(inMat.data, inMat.cols, inMat.rows,
                   inMat.step, QImage::Format_ARGB32);
        return img.copy();
    }
    qDebug() << "CvMatToQImage: 不支持的 Mat 类型";
    return {};
}

bool GameMonitor::CaptureWindowWithPrintWindow(HWND hwnd, QImage &image)
{
    RECT r;
    GetWindowRect(hwnd, &r);
    int w = r.right - r.left, h = r.bottom - r.top;
    HDC hdcWin = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWin);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, w, h);
    SelectObject(hdcMem, hBmp);
    if (!PrintWindow(hwnd, hdcMem, PW_RENDERFULLCONTENT)) {
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWin);
        return false;
    }
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(bi);
    bi.biWidth = w;
    bi.biHeight = -h;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    QImage tmp(w, h, QImage::Format_ARGB32);
    GetDIBits(hdcWin, hBmp, 0, h, tmp.bits(),
              reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

    image = tmp;
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWin);
    return true;
}

bool GameMonitor::checkForRejoin(HWND Hwnd) {
    QImage fullImage;
    if (!CaptureWindowWithPrintWindow(Hwnd, fullImage)) {
        qDebug() << "checkForRejoin: Failed to capture full image using PrintWindow";
        return false;
    }
    qDebug() << "checkForRejoin: Full image captured, size:" << fullImage.width() << "x" << fullImage.height();
    RECT rect;
    if (!GetWindowRect(Hwnd, &rect)) {
        qDebug() << "checkForRejoin: Failed to get window rect";
        return false;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    qDebug() << "checkForRejoin: Window rect:" << rect.left << rect.top << width << height;
    QImage roi = fullImage.copy(static_cast<int>(width * 760 / 1920),
                                static_cast<int>(height * 820 / 1080),
                                static_cast<int>(width * 400 / 1920),
                                static_cast<int>(height * 80 / 1080));
    qDebug() << "checkForRejoin: ROI size:" << roi.width() << "x" << roi.height();

    // 进入OCR流程
    tesseract::TessBaseAPI ocr;
    // 用 mingw64 下的 tessdata 目录

    if (ocr.Init(tessdataPath, "chi_sim+eng")) {
        qDebug() << "ocrImage: 初始化OCR路径失败，请检查 tessdataPath";
        return false;
    }
    ocr.SetPageSegMode(tesseract::PSM_AUTO);

    int bpp1 = roi.depth()/8;
    ocr.SetImage(const_cast<uchar*>(roi.bits()),
                 roi.width(),
                 roi.height(),
                 bpp1,
                 roi.bytesPerLine());
    char *out1 = ocr.GetUTF8Text();
    QString recognizedText1 = QString::fromUtf8(out1);
    delete[] out1;

    QStringList keywordsofStartbutton = {"PRESS", "START", "按下", "开始"};

    qDebug() << "尝试检测开始游戏按钮是否存在……";
    if(!recognizedText1.isEmpty()) {
        qDebug() << "OCR已识别到内容: " << recognizedText1 << "尝试检测是否为开始游戏按钮……";
        for (const QString &keyword : keywordsofStartbutton) {
            if (!keyword.isEmpty() && recognizedText1.contains(keyword)) {
                qDebug() << "已检测到开始游戏按钮！";
                return true;
            }
        }
        qDebug() << "未检测到开始游戏按钮";
        return false;
    }
    else {
        qDebug() << "OCR未识别到任何内容！";
        return false;
    }
}

bool GameMonitor::checkJoinCard(HWND hwnd, int &X, int &Y)
{
    // 1. 截取整个窗口
    QImage fullImage;
    if (!CaptureWindowWithPrintWindow(hwnd, fullImage)) {
        qDebug() << "checkJoinCard: 截取窗口失败";
        return false;
    }

    // 2. 转为灰度，OCR 效果更稳定 (待测试)
    // 转 cv::Mat 并拆通道
    cv::Mat m1 = QImageToCvMat(fullImage);

    std::vector<cv::Mat> ch1;
    cv::split(m1, ch1);
    if (ch1.size() < 3) {
        qDebug() << "captureWindowImage: 通道不足";
        return false;
    }

    // ROI1 用蓝色通道(索引 0)
    QImage gray = CvMatToQImage(ch1[0]);

    // 3. 初始化 Tesseract
    tesseract::TessBaseAPI ocr;

    if (ocr.Init(tessdataPath, "chi_sim+eng")) {
        qDebug() << "checkJoinCard: Tesseract Init 失败";
        return false;
    }
    ocr.SetPageSegMode(tesseract::PSM_AUTO);

    // 4. 设置图像并运行 OCR
    ocr.SetImage(const_cast<uchar*>(gray.bits()),
                 gray.width(),
                 gray.height(),
                 1,  // 每像素 1 字节
                 gray.bytesPerLine());
    ocr.Recognize(0);

    // 5. 遍历所有单词，寻找关键词
    tesseract::ResultIterator *ri = ocr.GetIterator();
    if (ri) {
        do {
            // 获取本次的单词文本
            char *word = ri->GetUTF8Text(tesseract::RIL_WORD);
            if (!word) continue;
            QString ws = QString::fromUtf8(word).trimmed();
            delete [] word;

            // 匹配“JOIN GAME”（英文）或“加入游戏”（中文）
            if (ws.compare("JOIN GAME", Qt::CaseInsensitive) == 0
                || ws == QStringLiteral("加入游戏"))
            {
                // 取出这块文字的包围盒坐标
                int x1, y1, x2, y2;
                ri->BoundingBox(tesseract::RIL_WORD, &x1, &y1, &x2, &y2);

                // 中心点
                X = (x1 + x2) / 2;
                Y = (y1 + y2) / 2;

                qDebug() << "checkJoinCard: 找到关键词" << ws << "坐标:" << X << Y;
                ocr.End();
                return true;
            }
        } while (ri->Next(tesseract::RIL_WORD));
    }

    ocr.End();
    qDebug() << "checkJoinCard: 未检测到 JOIN GAME 或 加入游戏";
    return false;
}
