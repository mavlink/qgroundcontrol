#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QUrl>

namespace UomRegistration {
enum class Operation
{
    Verify,
    ReportActive,
    ReportInactive,
    Cancel
};

struct Config
{
    QString environment = QStringLiteral("integration");
    QString producerId{};
    QString expectedSerial{};
    QUrl verifyUrl{QStringLiteral("https://uom.caac.gov.cn/api/uom-uavreg/eqgb/openapi/verifyRegisterFactory")};
    QUrl reportUrl{QStringLiteral("https://uom.caac.gov.cn/api/uom-uavreg/eqgb/openapi/reportActiveStateFactory")};
    QUrl cancelUrl{QStringLiteral("https://uom.caac.gov.cn/api/uom-uavreg/eqgb/openapi/cancelRegisterFactory")};

    [[nodiscard]] QString scope() const;
    [[nodiscard]] bool valid() const;
    [[nodiscard]] QUrl endpoint(Operation operation) const;
};

struct Result
{
    bool success = false;
    bool retryable = false;
    bool uncertain = false;
    int retryAfterSeconds = 0;
    QString value{};
    QString token{};
    QString detail{};
};

struct Record
{
    QString id{};
    QString scope{};
    QString serial{};
    QString uid{};
    QString kind{};
    QString stage{};
    QString type{};
    QString reason{};
    QString tokenReference{};
    QString configJson{};
    int deviceState = 0;
    qint64 updatedMs = 0;
    QString reportedValue{};
    QString reportedScope{};
    qint64 reportedAtMs = 0;
    bool deviceVerified = false;
    quint32 verifiedBoot = 0;
    quint32 verifiedRevision = 0;
};

[[nodiscard]] bool validSerial(const QString& serial);
[[nodiscard]] QString operationName(Operation operation);
}  // namespace UomRegistration

Q_DECLARE_METATYPE(UomRegistration::Result)
