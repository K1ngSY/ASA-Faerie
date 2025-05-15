#include "mainwindow.h"
#include "AuthDialog.h"

#include <QApplication>
#include <QObject>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AuthDialog authDlg;
    MainWindow w;

    QObject::connect(&authDlg, &AuthDialog::validated, &w, [&](const QString &date){
        w.setExpireDate(date);
    });
    if (authDlg.exec() != QDialog::Accepted) {
        // 用户取消或认证失败，直接退出
        return 0;
    }
    w.show();
    w.turnOnExpirationDetector();
    return a.exec();
}
