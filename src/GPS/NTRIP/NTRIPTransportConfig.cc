#include "NTRIPTransportConfig.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

QString NTRIPTransportConfig::validationError() const
{
    const auto tr = [](const char* s) { return QCoreApplication::translate("NTRIPTransportConfig", s); };

    if (host.isEmpty()) {
        return tr("No host address");
    }
    if (port <= 0 || port > 65535) {
        return tr("Invalid port");
    }

    static const QRegularExpression controlChars(QStringLiteral("[\\r\\n\\x00-\\x1f]"));
    if (host.contains(controlChars)) {
        return tr("Invalid host (contains control characters)");
    }
    if (!mountpoint.isEmpty() && mountpoint.contains(controlChars)) {
        return tr("Invalid mountpoint name (contains control characters)");
    }
    // RFC 7617 §2: the Basic-auth userid must not contain a colon.
    if (username.contains(QLatin1Char(':'))) {
        return tr("Invalid username (must not contain ':')");
    }

    return QString();
}

QString NTRIPTransportConfig::streamValidationError() const
{
    if (const QString error = validationError(); !error.isEmpty()) {
        return error;
    }
    if (mountpoint.trimmed().isEmpty()) {
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
        .arg(host)
        .arg(port)
        .arg(username)
        .arg(password)
        .arg(useTls ? 1 : 0)
        .arg(allowSelfSignedCerts ? 1 : 0);
}

bool NTRIPTransportConfig::udpForwardDiffers(const NTRIPTransportConfig& other) const
{
    return udpForwardEnabled != other.udpForwardEnabled || udpTargetAddress != other.udpTargetAddress ||
           udpTargetPort != other.udpTargetPort;
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
