#include "AuthDialog.h"
#include "ui_AuthDialog.h"
#include "CDKValidator.h"

#include <QMessageBox>
#include <QTimer>

AuthDialog::AuthDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AuthDialog),
    titleText("欢迎使用方舟：生存飞升 报警器 - 请进行卡密认证")
{
    ui->setupUi(this);
    ui->titleLabel->setText(titleText);

    // 定时器实现标题滚动效果
    QTimer *titleTimer = new QTimer(this);
    connect(titleTimer, &QTimer::timeout, this, &AuthDialog::updateTitle);
    titleTimer->start(300);

    // 连接验证按钮点击事件
    connect(ui->validateButton, &QPushButton::clicked, this, &AuthDialog::onValidate);
}

AuthDialog::~AuthDialog()
{
    delete ui;
}

void AuthDialog::updateTitle()
{
    // 简单滚动效果：将首字符移到末尾
    titleText = titleText.mid(1) + titleText[0];
    ui->titleLabel->setText(titleText);
}

void AuthDialog::onValidate()
{
    QString cdk = ui->cdkEdit->text().trimmed();
    if (cdk.isEmpty()) {
        QMessageBox::warning(this, "错误", "卡密不能为空");
        return;
    }
    // 通过 HTTP 请求调用后端接口验证卡密
    auto res = CDKValidator::validate(cdk);
    switch (res.code) {
    case ValidateResult::Ok:
        emit validated(res.expireDate);
        QMessageBox::information(this, "成功", "卡密有效，正在进入系统……");
        QTimer::singleShot(3000, this, &QDialog::accept);
        break;
    case ValidateResult::Invalid:
        QMessageBox::warning(this, "错误", "卡密无效");
        break;
    case ValidateResult::HWIDMismatch:
        QMessageBox::warning(this, "错误", "HWID 不符");
        break;
    case ValidateResult::Expired:
        QMessageBox::warning(this, "错误", "卡密已过期");
        break;
    default:
        QMessageBox::warning(this, "错误", "网络或服务器错误：" + res.message);
        break;
    }
}
