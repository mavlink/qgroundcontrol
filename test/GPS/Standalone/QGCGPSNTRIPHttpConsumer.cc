#include <array>

#include "NTRIPConfiguration.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPHttpRequest.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_QML_LIB) || defined(QT_GUI_LIB) || defined(QT_SERIALPORT_LIB) || \
    defined(QT_BLUETOOTH_LIB) || defined(QT_HTTPSERVER_LIB)
#error NTRIP HTTP must not inherit positioning, receiver, application, Bluetooth, or HTTP server dependencies.
#endif

#if __has_include("RTCMDecodedFrame.h") || __has_include("RTCMFrameDecoder.h")
#error NTRIP HTTP must not inherit RTCM decoding headers.
#endif

#if __has_include("RTCMFramer.h") || __has_include("GPSProtocol.h")
#error NTRIP HTTP must not inherit receiver protocol headers.
#endif

int main()
{
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("localhost");
    config.mountpoint = QStringLiteral("BASE");
    if (!config.streamValidationError().isEmpty()) {
        return 1;
    }

    struct Credentials
    {
        QString username;
        QString password;
        QByteArray encoded;
    };

    const std::array credentials = {
        Credentials{{}, {}, {}},
        Credentials{QStringLiteral("user"), QStringLiteral("pass"), "dXNlcjpwYXNz"},
        Credentials{QStringLiteral("user"), {}, "dXNlcjo="},
        Credentials{{}, QStringLiteral("pass"), "OnBhc3M="},
        Credentials{QString::fromUtf8("Us\xc3\xa9r"), QString::fromUtf8("Pa\xc3\x9fs"), "VXPDqXI6UGHDn3M="},
    };
    config.host = QStringLiteral("Caster.Example.com");
    config.mountpoint = QStringLiteral("MixedCase_1");
    const QByteArray expectedHeaders =
        "GET /MixedCase_1 HTTP/1.1\r\n"
        "Host: Caster.Example.com\r\n"
        "Ntrip-Version: Ntrip/2.0\r\n"
        "User-Agent: NTRIP QGroundControl/1.0\r\n";
    for (const auto& credential : credentials) {
        config.username = credential.username;
        config.password = credential.password;
        for (const bool useTls : {false, true}) {
            config.useTls = useTls;
            const auto request = NTRIPHttpRequest::build(config);
            const bool hasCredentials = !credential.encoded.isEmpty();
            const QByteArray authorization =
                hasCredentials ? "Authorization: Basic " + credential.encoded + "\r\n" : QByteArray{};
            if (!request.error.isEmpty() || request.credentialsInClear != (hasCredentials && !useTls) ||
                request.bytes != expectedHeaders + authorization + "\r\n") {
                return 2;
            }
        }
    }
    config.useTls = false;
    for (const QString& mountpoint : {QString(), QStringLiteral("bad\r\nInjected: header")}) {
        config.mountpoint = mountpoint;
        const auto request = NTRIPHttpRequest::build(config);
        if (request.error.isEmpty() || !request.bytes.isEmpty() || request.credentialsInClear) {
            return 3;
        }
    }

    NTRIPHttpDecoder decoder;
    const auto result = decoder.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n", {});
    if (!result.connected || !result.complete || result.failure || result.body != "abc") {
        return 4;
    }
    decoder.reset();
    const auto failure = decoder.feed("HTTP/1.1 503 Unavailable\r\nRetry-After: 17\r\nContent-Length: 0\r\n\r\n", {});
    if (!failure.failure || failure.failure->code != NTRIPError::HttpError ||
        failure.failure->retryAfter != std::chrono::seconds(17)) {
        return 5;
    }
    return 0;
}
