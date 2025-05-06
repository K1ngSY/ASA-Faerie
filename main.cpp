#include "AlarmWindow.h"
#include "AuthDialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 先设置 TESSDATA_PREFIX 环境变量
    // qputenv("TESSDATA_PREFIX", QByteArray("C:/msys64/mingw64/share/tessdata/"));

    // 先弹出授权/登录对话框
    AuthDialog authDlg;
    AlarmWindow w;

    QObject::connect(&authDlg, &AuthDialog::validated, &w, [&](const QString &date){
        w.setExpireDateLabel(date);
        w.setExpireDate(date);
    });

    if (authDlg.exec() != QDialog::Accepted) {
        // 用户取消或认证失败，直接退出
        return 0;
    }

    // 认证通过后，再创建并显示主窗口

    w.show();

    return a.exec();
}
