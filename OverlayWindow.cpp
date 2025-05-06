#include "OverlayWindow.h"
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QPoint>
#include <QDebug>


OverlayWindow::OverlayWindow(HWND targetHwnd, QWidget *parent) :
    QWidget(parent),
    m_targetHwnd(targetHwnd),
    m_redDotX(20),
    m_redDotY(20)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents); // 不拦截鼠标
    setAttribute(Qt::WA_ShowWithoutActivating);
    resize(300, 200);

    connect(&m_updateTimer, &QTimer::timeout, this, &OverlayWindow::updateOverlayPosition);
    m_updateTimer.start(10); // 定时追踪位置
    qDebug() << "OverLayWindow Created";
    show();

    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
}

void OverlayWindow::setRedDotPosition(int x, int y)
{
    m_redDotX = x;
    m_redDotY = y;
    update();
}

void OverlayWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPen pen;
    QPoint pointHL, pointHR, pointVU, pointVL;
    int L = 10;
    pointHL.setX(m_redDotX-L);
    pointHL.setY(m_redDotY);
    pointHR.setX(m_redDotX+L);
    pointHR.setY(m_redDotY);
    pointVU.setX(m_redDotX);
    pointVU.setY(m_redDotY+L);
    pointVL.setX(m_redDotX);
    pointVL.setY(m_redDotY-L);
    pen.setWidth(2);
    pen.setColor(Qt::red);
    pen.setCapStyle(Qt::SquareCap);
    painter.setPen(pen);
    // painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), QColor(0, 0, 0, 1));  // 几乎完全透明，但不是0
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawLine(pointHL,pointHR);
    painter.drawLine(pointVU,pointVL);
}

void OverlayWindow::updateOverlayPosition()
{
    if (!IsWindow(m_targetHwnd)) {
        close();
        return;
    }

    RECT rect;
    GetWindowRect(m_targetHwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    move(rect.left, rect.top);
    resize(width, height);
    update();
    qDebug() << "微信窗口绘制刷新";
}
