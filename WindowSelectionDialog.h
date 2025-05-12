#ifndef WINDOWSELECTIONDIALOG_H
#define WINDOWSELECTIONDIALOG_H

#include <QDialog>
#include <QString>
#include <windows.h>

struct WindowInfo {
    HWND hwnd;
    QString title;
};

namespace Ui {
class WindowSelectionDialog;
}

class WindowSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WindowSelectionDialog(QWidget *parent = nullptr);
    ~WindowSelectionDialog();

    void setLabel1Text(QString);

    // 返回用户选择的窗口信息
    WindowInfo selectedWindow() const;

private:
    Ui::WindowSelectionDialog *ui;
    std::vector<WindowInfo> m_windows;
    WindowInfo m_selectedWindow;
};

#endif // WINDOWSELECTIONDIALOG_H
