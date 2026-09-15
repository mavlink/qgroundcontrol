#pragma once

#include <chrono>

#include <QtCore/QChronoTimer>
#include <QtCore/QLoggingCategory>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>

#include "MonotonicClock.h"
#include "NTRIPTransport.h"
#include "NTRIPTransportConfig.h"
#include "RTCMFrameDecoder.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPHttpTransportLog)

class NTRIPHttpTransport : public NTRIPTransport
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr std::chrono::milliseconds kConnectTimeout{10000};
    static constexpr std::chrono::milliseconds kDataWatchdog{30000};
    static constexpr int kMaxHttpHeaderSize = 32768;

    explicit NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent = nullptr);
    ~NTRIPHttpTransport() override;

    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmDecoder.setWhitelist(messageIds); }

    const NTRIPTransportConfig& config() const { return _config; }

    // plaintextCredentialsWarning lives on the NTRIPTransport base signal set so
    // NTRIPManager can connect without concrete-type knowledge.

protected:
    struct HttpStatus
    {
        int code = 0;
        QString reason;
        bool valid = false;
    };

    static HttpStatus parseHttpStatusLine(const QString& line);

    static bool isHttpSuccess(int code) { return code >= 200 && code < 300; }

    struct HttpRequest
    {
        QByteArray bytes;
        /// Credentials are present and the channel is not TLS — caller must warn.
        bool credentialsInClear = false;
    };

    static HttpRequest buildHttpRequest(const NTRIPTransportConfig& config);

private:
    void _connect();
    void _fail(NTRIPError code, const QString& msg);
    void _sendHttpRequest();
    void _readBytes();
    void _handleHttpResponse();
    void _handleRtcmData();
    void _parseRtcm(const QByteArray& buffer,
                    qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000));

    NTRIPTransportConfig _config;

    QTcpSocket* _socket = nullptr;
    QChronoTimer _connectTimeoutTimer;
    QChronoTimer _dataWatchdogTimer;

    RTCMFrameDecoder _rtcmDecoder;
    bool _httpHandshakeDone = false;
    bool _stopped = false;

    qint64 _postOkTimestampMs = 0;

    QByteArray _httpResponseBuf;
};
