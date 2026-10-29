#include "UomRegistrationTypes.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace UomRegistration {
namespace {
bool officialUrl(const QUrl& url)
{
    return url.isValid() && url.scheme() == QStringLiteral("https") &&
           url.host() == QStringLiteral("uom.caac.gov.cn") && url.port(443) == 443 && url.userInfo().isEmpty() &&
           !url.hasQuery() && !url.hasFragment() && !url.path().isEmpty();
}
}  // namespace

QString Config::scope() const
{
    const QJsonArray identity{environment, producerId, verifyUrl.toString(QUrl::FullyEncoded),
                              reportUrl.toString(QUrl::FullyEncoded), cancelUrl.toString(QUrl::FullyEncoded)};
    return QString::fromLatin1(
        QCryptographicHash::hash(QJsonDocument(identity).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256)
            .toHex());
}

bool Config::valid() const
{
    return (environment == QStringLiteral("integration") || environment == QStringLiteral("official")) &&
           producerId.size() == 36 && validSerial(expectedSerial) && officialUrl(verifyUrl) && officialUrl(reportUrl) &&
           officialUrl(cancelUrl);
}

QUrl Config::endpoint(Operation operation) const
{
    switch (operation) {
        case Operation::Verify:
            return verifyUrl;
        case Operation::ReportActive:
        case Operation::ReportInactive:
            return reportUrl;
        case Operation::Cancel:
            return cancelUrl;
    }
    return {};
}

bool validSerial(const QString& serial)
{
    if (serial.size() != 20) {
        return false;
    }
    for (QChar ch : serial) {
        if ((ch.unicode() < 0x21 || ch.unicode() > 0x7e)) {
            return false;
        }
    }
    return true;
}

QString operationName(Operation operation)
{
    switch (operation) {
        case Operation::Verify:
            return QStringLiteral("verify");
        case Operation::ReportActive:
            return QStringLiteral("report-active");
        case Operation::ReportInactive:
            return QStringLiteral("report-inactive");
        case Operation::Cancel:
            return QStringLiteral("cancel");
    }
    return {};
}
}  // namespace UomRegistration
