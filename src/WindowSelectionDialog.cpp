#include "WindowSelectionDialog.h"
#include "ui_WindowSelectionDialog.h"
#include <windows.h>

// 枚举所有顶级窗口的辅助函数
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    int length = GetWindowTextLengthW(hwnd);
    if (length == 0)
        return TRUE; // 忽略无标题窗口

    wchar_t* buffer = new wchar_t[length + 1];
    GetWindowTextW(hwnd, buffer, length + 1);
    QString title = QString::fromWCharArray(buffer);
    delete[] buffer;

    if (IsWindowVisible(hwnd)) {
        std::vector<WindowInfo>* vec = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
        vec->push_back({ hwnd, title });
    }
    return TRUE;
}

WindowSelectionDialog::WindowSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WindowSelectionDialog)
{
    ui->setupUi(this);

    // ui->label1->setText(this->windowTitle());

    // 枚举所有窗口，并存入 m_windows
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&m_windows));
    // 在 listWidget 中显示所有窗口的 “HWND - 标题”
    for (const auto &win : m_windows) {
        ui->listWidget->addItem(QString("HWND: %1   -   %2")
                                    .arg((qulonglong)win.hwnd)
                                    .arg(win.title));
    }

    connect(ui->okButton, &QPushButton::clicked, this, [=](){
        int row = ui->listWidget->currentRow();
        if (row >= 0 && row < static_cast<int>(m_windows.size())) {
            m_selectedWindow = m_windows[row];
            accept();
        }
    });
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

WindowSelectionDialog::~WindowSelectionDialog()
{
    delete ui;
}

WindowInfo WindowSelectionDialog::selectedWindow() const {
    return m_selectedWindow;
}

void WindowSelectionDialog::setLabel1Text(QString text) {
    ui->label1->setStyleSheet("font-weight: bold; font-size: 20px; color:#ff0000;");
    ui->label1->setText(text);
}
