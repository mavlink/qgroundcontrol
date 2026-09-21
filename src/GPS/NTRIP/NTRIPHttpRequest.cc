#include "NTRIPHttpRequest.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QHttpHeaders>

#include "NTRIPConfiguration.h"
#include "QGCNetworkClient.h"

NTRIPHttpRequest NTRIPHttpRequest::build(const NTRIPConnectionConfig& config, Purpose purpose)
{
    NTRIPHttpRequest result;
    result.error = purpose == Purpose::Corrections ? config.streamValidationError() : config.validationError();
    if (!result.error.isEmpty()) {
        return result;
    }

    result.url.setScheme(config.useTls ? QStringLiteral("https") : QStringLiteral("http"));
    result.url.setHost(config.host);
    result.url.setPort(config.port);
    result.url.setPath(purpose == Purpose::SourceTable ? QStringLiteral("/") : QLatin1Char('/') + config.mountpoint);
    if (!result.url.isValid() || result.url.host().isEmpty()) {
        result.error = QCoreApplication::translate("NTRIPHttpTransport", "Invalid NTRIP endpoint");
        return result;
    }

    using Header = QHttpHeaders::WellKnownHeader;
    auto& headers = result.headers;
    const QByteArray host = result.url.authority(QUrl::FullyEncoded).toLatin1();
    if (!headers.append(Header::Host, QLatin1StringView(host.constData(), host.size())) ||
        !headers.append("Ntrip-Version", "Ntrip/2.0") ||
        !headers.append(Header::UserAgent, "NTRIP QGroundControl/1.0")) {
        result.error = QCoreApplication::translate("NTRIPHttpTransport", "Invalid NTRIP request header");
        return result;
    }

    const bool hasCredentials = !config.username.isEmpty() || !config.password.isEmpty();
    if (hasCredentials) {
        const QString authorization =
            QStringLiteral("Basic ") + QGCNetworkHelper::createBasicAuthCredentials(config.username, config.password);
        if (!headers.append(Header::Authorization, authorization)) {
            result.error = QCoreApplication::translate("NTRIPHttpTransport", "Invalid NTRIP authorization header");
            return result;
        }
    }

    result.bytes = "GET " + result.url.path(QUrl::FullyEncoded).toLatin1() + " HTTP/1.1\r\n";
    // Some legacy casters match these spellings case-sensitively.
    for (const char* name : {"Host", "Ntrip-Version", "User-Agent", "Authorization"}) {
        if (headers.contains(QLatin1StringView(name))) {
            const auto value = headers.value(QLatin1StringView(name));
            result.bytes += name;
            result.bytes += ": ";
            result.bytes.append(value.data(), value.size());
            result.bytes += "\r\n";
        }
    }
    result.bytes += "\r\n";
    result.credentialsInClear = hasCredentials && !config.useTls;
    return result;
}
