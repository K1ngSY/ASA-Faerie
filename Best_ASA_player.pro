QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += lrelease

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    WindowSelectionDialog.cpp \
    alarm.cpp \
    crashhandler.cpp \
    expirationdetector.cpp \
    gamehandler.cpp \
    main.cpp \
    mainwindow.cpp \
    motionsimulator.cpp \
    overlaywindow.cpp \
    rejoinprocessor.cpp \
    visualprocessor.cpp

HEADERS += \
    WindowSelectionDialog.h \
    alarm.h \
    crashhandler.h \
    expirationdetector.h \
    gamehandler.h \
    mainwindow.h \
    motionsimulator.h \
    overlaywindow.h \
    rejoinprocessor.h \
    visualprocessor.h

FORMS += \
    WindowSelectionDialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

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
