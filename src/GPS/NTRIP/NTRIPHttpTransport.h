#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QLoggingCategory>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>

#include <chrono>

#include "NTRIPHttpDecoder.h"
#include "NTRIPStream.h"
#include "NTRIPTransportConfig.h"
#include "RTCMParser.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPHttpTransportLog)

class NTRIPHttpTransport : public NTRIPStream
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr std::chrono::milliseconds kConnectTimeout{10000};
    static constexpr std::chrono::milliseconds kDataWatchdog{30000};
    static constexpr int kMaxHttpHeaderSize = 32768;

    explicit NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent = nullptr);
    ~NTRIPHttpTransport() override;

    bool providesTimestampedFrames() const override { return true; }
    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmParser.setWhitelist(messageIds); }

    const NTRIPTransportConfig& config() const { return _config; }

    // plaintextCredentialsWarning lives on the NTRIPStream base signal set so
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
    void _fail(const NTRIPFailure& failure);
    void _sendHttpRequest();
    void _readBytes();
    void _scheduleRead();
    void _parseRtcm(const QByteArray& buffer);

    NTRIPTransportConfig _config;

    QTcpSocket* _socket = nullptr;
    QChronoTimer _connectTimeoutTimer;
    QChronoTimer _dataWatchdogTimer;

    RTCMParser _rtcmParser;
    bool _httpHandshakeDone = false;
    bool _stopped = false;

    qint64 _postOkTimestampMs = 0;

    NTRIPHttpDecoder _httpDecoder;
    quint64 _generation = 0;
    qint64 _receivedAtMs = 0;
    bool _readScheduled = false;
    static constexpr qint64 MAX_READ_PER_TURN = 16384;
    static constexpr qint64 MAX_SOCKET_BUFFER = 65536;
};
