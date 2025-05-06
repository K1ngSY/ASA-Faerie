#ifndef MAINWINDOWHELPER_H
#define MAINWINDOWHELPER_H
#include <QString>
class MainWindowHelper
{
public:
    static void h_closeCrashWindows(const QString &m_crashKeywords);
    static bool h_restartGame();
    static bool h_checkWindowsExist();
};

#endif // MAINWINDOWHELPER_H
