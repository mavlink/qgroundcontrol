#include "NTRIPConfiguration.h"
#include "NTRIPHttpDecoder.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_QML_LIB) || defined(QT_GUI_LIB) || defined(QT_SERIALPORT_LIB)
#error NTRIP HTTP framing must not inherit positioning, receiver, or application dependencies.
#endif

int main()
{
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("localhost");
    config.mountpoint = QStringLiteral("BASE");
    if (!config.streamValidationError().isEmpty()) {
        return 1;
    }
    NTRIPHttpDecoder decoder;
    const auto result = decoder.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n", {});
    if (!result.connected || !result.complete || result.failure || result.body != "abc") {
        return 2;
    }
    decoder.reset();
    const auto failure = decoder.feed("HTTP/1.1 503 Unavailable\r\nRetry-After: 17\r\nContent-Length: 0\r\n\r\n", {});
    if (!failure.failure || failure.failure->code != NTRIPError::HttpError ||
        failure.failure->retryAfter != std::chrono::seconds(17)) {
        return 3;
    }
    return 0;
}
