#include "NTRIPTransportConfig.h"

#include <QtCore/QCoreApplication>
#include "NTRIPRequest.h"

QString NTRIPTransportConfig::validationError() const
{
    return NTRIPRequest::validationError(*this);
}

QString NTRIPTransportConfig::streamValidationError() const
{
    if (const QString error = validationError(); !error.isEmpty()) {
        return error;
    }
    QString name = mountpoint;
    while (name.startsWith(QLatin1Char('/'))) {
        name.remove(0, 1);
    }
    if (name.trimmed().isEmpty()) {
        return QCoreApplication::translate("NTRIPTransportConfig", "Select a mountpoint before connecting");
    }
    return {};
}

bool NTRIPTransportConfig::transportDiffers(const NTRIPTransportConfig& other) const
{
    return host != other.host || port != other.port || username != other.username || password != other.password ||
           mountpoint != other.mountpoint || useTls != other.useTls ||
           allowSelfSignedCerts != other.allowSelfSignedCerts;
}

QString NTRIPTransportConfig::casterIdentity() const
{
    return QStringLiteral("%1\x1f%2\x1f%3\x1f%4\x1f%5\x1f%6")
        .arg(NTRIPRequest::casterUrl(*this).toString(QUrl::FullyEncoded))
        .arg(port)
        .arg(username)
        .arg(password)
        .arg(useTls ? 1 : 0)
        .arg(allowSelfSignedCerts ? 1 : 0);
}



QVector<int> NTRIPTransportConfig::parseWhitelist(const QString& csv)
{
    QVector<int> ids;
    if (csv.isEmpty()) {
        return ids;
    }
    for (const auto& token : csv.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int id = token.trimmed().toInt(&ok);
        if (ok && id > 0) {
            ids.append(id);
        }
    }
    return ids;
}
