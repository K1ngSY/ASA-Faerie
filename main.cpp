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



    w.show();
    return a.exec();
}
