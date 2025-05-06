#ifndef CDKVALIDATOR_H
#define CDKVALIDATOR_H

#include <QString>

struct ValidateResult {
    enum Code { Ok, Invalid, HWIDMismatch, Expired, NetworkError } code;
    QString message;
    QString expireDate;
};

class CDKValidator {
public:
    // 同步验证卡密，返回详细结果
    static ValidateResult validate(const QString &cdk);
};

#endif // CDKVALIDATOR_H
