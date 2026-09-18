#pragma once

#include <chrono>

#include <QtCore/QChronoTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>
#include <QtNetwork/QTcpSocket>

#include "MonotonicClock.h"
#include "NTRIPConfiguration.h"
#include "NTRIPTransport.h"
#include "RTCMFrameDecoder.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPHttpTransportLog)

class NTRIPHttpTransport : public NTRIPTransport
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;
    friend class NTRIPReentrancyTest;

public:
    static constexpr std::chrono::milliseconds kConnectTimeout{10000};
    static constexpr std::chrono::milliseconds kDataWatchdog{30000};
    static constexpr int kMaxHttpHeaderSize = 32768;

    NTRIPHttpTransport(const NTRIPConnectionConfig& config, const NTRIPRtcmFilterConfig& filter,
                       QObject* parent = nullptr);
    ~NTRIPHttpTransport() override;

    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmDecoder.setWhitelist(messageIds); }

    const NTRIPConnectionConfig& config() const { return _config; }

protected:
    struct HttpRequest
    {
        QByteArray bytes;
        /// Credentials are present and the channel is not TLS — caller must warn.
        bool credentialsInClear = false;
    };

    static HttpRequest buildHttpRequest(const NTRIPConnectionConfig& config);

private:
    struct HttpStatus
    {
        int code = 0;
        QString reason;
        bool valid = false;
    };

    static HttpStatus _parseHttpStatusLine(const QString& line);

    static bool _isHttpSuccess(int code) { return code >= 200 && code < 300; }

    void _connect();
    void _fail(NTRIPError code, const QString& msg);
    void _retireSocket();
    bool _write(const QByteArray& bytes);
    void _sendHttpRequest();
    void _readBytes();
    void _handleHttpResponse();
    void _handleRtcmData();
    void _parseRtcm(const QByteArray& buffer,
                    qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000));

    NTRIPConnectionConfig _config;

    QPointer<QTcpSocket> _socket;
    QChronoTimer _connectTimeoutTimer;
    QChronoTimer _dataWatchdogTimer;

    RTCMFrameDecoder _rtcmDecoder;
    bool _httpHandshakeDone = false;
    bool _stopped = false;
    quint64 _attempt = 0;
    qint64 _postOkTimestampMs = 0;
    QByteArray _httpResponseBuf;
};
