#ifndef GAMEHANDLER_H
#define GAMEHANDLER_H

#include <windows.h>
#include <QString>

class gameHandler
{
public:
    gameHandler();

    static bool bindWindows(const QString &title, HWND &windowHwnd);
    static bool checkWindowExist(const HWND &windowHwnd);
    static bool checkCrash();
};

#endif // GAMEHANDLER_H
