QT       += core gui network widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += lrelease

SOURCES += \
    AlarmWindow.cpp \
    AuthDialog.cpp \
    CDKValidator.cpp \
    GameMonitor.cpp \
    MotionSimulator.cpp \
    OverlayWindow.cpp \
    WindowEnumerator.cpp \
    WindowSelectionDialog.cpp \
    main.cpp

HEADERS += \
    AlarmWindow.h \
    AuthDialog.h \
    CDKValidator.h \
    GameMonitor.h \
    MotionSimulator.h \
    OverlayWindow.h \
    WindowEnumerator.h \
    WindowSelectionDialog.h

FORMS += \
    AlarmWindow.ui \
    AuthDialog.ui \
    WindowSelectionDialog.ui

# ———————————— OpenCV ————————————
INCLUDEPATH += "C:/msys64/mingw64/include/opencv4"
LIBS += -L"C:/msys64/mingw64/lib" \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_imgcodecs

# ————————— Tesseract OCR & Leptonica —————————
INCLUDEPATH += "C:/msys64/mingw64/include"
LIBS += -L"C:/msys64/mingw64/lib" \
        -ltesseract \
        -lleptonica

# ———————— 其他依赖（PNG, JPEG, etc.） ————————
LIBS += -L"C:/msys64/mingw64/lib" \
        -lpng16 \
        -ljpeg \
        -lz \
        -ltiff \
        -lwebp \
        -lcurl \
        -larchive

# ————————— Windows API —————————
win32: LIBS += -lUser32 -lgdi32 -lws2_32 -liphlpapi
