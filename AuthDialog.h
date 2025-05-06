#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>

namespace Ui {
class AuthDialog;
}

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    ~AuthDialog();

private slots:
    void onValidate();
    void updateTitle();

private:
    Ui::AuthDialog *ui;
    QString titleText;

signals:
    void validated(const QString &expireDate);
};

#endif // AUTHDIALOG_H
