#include "NTRIPRequest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

#include "NTRIPTransportConfig.h"

QUrl NTRIPRequest::casterUrl(const NTRIPTransportConfig& config)
{
    QString host = config.host;
    if (host.startsWith(QLatin1Char('[')) && host.endsWith(QLatin1Char(']'))) {
        host = host.mid(1, host.size() - 2);
    }
    QUrl url;
    url.setScheme(config.useTls ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(host, QUrl::DecodedMode);
    const int defaultPort = config.useTls ? 443 : 80;
    if (config.port != defaultPort) {
        url.setPort(config.port);
    }
    url.setPath(QStringLiteral("/"));
    return url;
}

QString NTRIPRequest::validationError(const NTRIPTransportConfig& config)
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("NTRIPTransportConfig", text); };
    if (config.host.isEmpty()) {
        return tr("No host address");
    }
    if (config.port <= 0 || config.port > 65535) {
        return tr("Invalid port");
    }
    static const QRegularExpression controlChars(QStringLiteral("[\\x00-\\x1f\\x7f]"));
    if (config.host.contains(controlChars)) {
        return tr("Invalid host (contains control characters)");
    }
    const QUrl url = casterUrl(config);
    if (!url.isValid() || url.host().isEmpty() || config.host.contains(QLatin1Char('/')) ||
        config.host.contains(QLatin1Char('\\')) || config.host.contains(QLatin1Char('?')) ||
        config.host.contains(QLatin1Char('#')) || config.host.contains(QLatin1Char('@'))) {
        return tr("Enter a hostname or IP address without a URL, path, or port");
    }
    if (config.mountpoint.contains(controlChars)) {
        return tr("Invalid mountpoint name (contains control characters)");
    }
    if (config.username.contains(QLatin1Char(':'))) {
        return tr("Invalid username (must not contain ':')");
    }
    if (config.username.contains(controlChars) || config.password.contains(controlChars)) {
        return tr("Invalid credentials (contain control characters)");
    }
    return {};
}

NTRIPRequest::Request NTRIPRequest::build(const NTRIPTransportConfig& config, bool sourceTable)
{
    Request request;
    request.url = casterUrl(config);
    if (!sourceTable) {
        QString mountpoint = config.mountpoint;
        while (mountpoint.startsWith(QLatin1Char('/'))) {
            mountpoint.remove(0, 1);
        }
        request.url.setPath(QLatin1Char('/') + mountpoint, QUrl::DecodedMode);
    }
    using Header = QHttpHeaders::WellKnownHeader;
    if (!request.headers.append(Header::Host, request.url.authority(QUrl::FullyEncoded)) ||
        !request.headers.append("Ntrip-Version", "Ntrip/2.0") ||
        !request.headers.append(Header::UserAgent, "NTRIP QGroundControl/1.0")) {
        return {};
    }
    if (!config.username.isEmpty() || !config.password.isEmpty()) {
        const QByteArray credentials = (config.username + QLatin1Char(':') + config.password).toUtf8().toBase64();
        if (!request.headers.append(Header::Authorization, "Basic " + credentials)) {
            return {};
        }
        request.credentialsInClear = !config.useTls;
    }
    request.bytes = "GET " + request.url.path(QUrl::FullyEncoded).toUtf8() + " HTTP/1.1\r\n";
    for (qsizetype i = 0; i < request.headers.size(); ++i) {
        request.bytes +=
            request.headers.nameAt(i).toString().toLatin1() + ": " + request.headers.valueAt(i).toByteArray() + "\r\n";
    }
    request.bytes += "\r\n";
    return request;
}
