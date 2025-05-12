#include <windows.h>               // 用于获取卷序列号
#include <intrin.h>                // 用于 __cpuid，如果你不需要 CPU ID，这一行也可以删掉
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>       // 用来获取 MAC
#include <QDebug>
#include <QUrl>
#include "cdkvalidator.h"

// 1. 获取系统盘卷序列号作为一个 HWID
static QString getVolumeSerialHWID() {
    DWORD serial = 0;
    if (GetVolumeInformationW(L"C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        return QString::number(serial, 16).toUpper();
    }
    return QString();
}

// 2. 用 Qt 接口获取第一个非回环网络接口的 MAC 地址
static QString getFirstMacHWID() {
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            QString mac = iface.hardwareAddress();       // e.g. "00:1A:2B:3C:4D:5E"
            if (!mac.isEmpty() && mac != "00:00:00:00:00:00") {
                return mac.remove(':').toUpper();       // "001A2B3C4D5E"
            }
        }
    }
    return QString();
}

// （可选）3. 获取 CPU ID 作为第三个 HWID
static QString getCpuIdHWID() {
    int regs[4] = {0};
    __cpuid(regs, 1);
    return QString("%1%2")
        .arg(static_cast<uint>(regs[0]), 8, 16, QChar('0'))
        .arg(static_cast<uint>(regs[1]), 8, 16, QChar('0'))
        .toUpper();
}

ValidateResult CDKValidator::validate(const QString &cdk) {
    // 收集 HWID 列表
    QStringList hwids;
    hwids << getVolumeSerialHWID()
          << getFirstMacHWID();

    // 构造 JSON 请求体
    QJsonObject body;
    body["cdk"] = cdk;
    QJsonArray arr;
    for (const QString &h : hwids) arr.append(h);
    body["hwids"] = arr;

    // 准备 HTTP 请求
    QNetworkRequest req(QUrl("https://cdk.k8126.uk/api/validate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager mgr;
    QEventLoop loop;
    QNetworkReply *reply = mgr.post(req, QJsonDocument(body).toJson());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // 网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        reply->deleteLater();
        return { ValidateResult::NetworkError, err , QString()};
    }

    // 解析返回的 JSON
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
    if (!doc.isObject()) {
        return { ValidateResult::NetworkError, "Invalid JSON response" , QString()};
    }
    QJsonObject obj = doc.object();
    QString status = obj.value("status").toString();

    if (status != "ok") {
        QString code = obj.value("code").toString();
        if (code == "INVALID")
            return { ValidateResult::Invalid, "" , QString()};
        if (code == "HWID_MISMATCH")
            return { ValidateResult::HWIDMismatch, "" , QString()};
        if (code == "EXPIRED")
            return { ValidateResult::Expired, "" , QString()};
        return { ValidateResult::NetworkError, obj.value("message").toString() , QString()};
    }

    // 走到这里说明 status == "ok"
    ValidateResult r;
    r.code = ValidateResult::Ok;
    // 服务器返回 expire_date 字段，形如 "2025-05-18"
    r.expireDate = obj.value("expire_date").toString();
    return r;
}
