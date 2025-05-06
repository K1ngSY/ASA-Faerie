#ifndef OVERLAYWINDOW_H
#define OVERLAYWINDOW_H

#include <QWidget>
#include <QTimer>
#include <windows.h>

class OverlayWindow : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayWindow(HWND targetHwnd, QWidget *parent = nullptr);
    void setRedDotPosition(int x, int y);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    HWND m_targetHwnd;
    int m_redDotX;
    int m_redDotY;
    QTimer m_updateTimer;
    void updateOverlayPosition();
};

#endif // OVERLAYWINDOW_H
