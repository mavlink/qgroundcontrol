#pragma once

#include <chrono>

#include <QtCore/QChronoTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtNetwork/QTcpSocket>

#include "MonotonicClock.h"
#include "NTRIPConfiguration.h"
#include "NTRIPHttpDecoder.h"
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
    static constexpr std::chrono::milliseconds kErrorBodyTimeout{250};

    NTRIPHttpTransport(const NTRIPConnectionConfig& config, const NTRIPRtcmFilterConfig& filter,
                       QObject* parent = nullptr);
    ~NTRIPHttpTransport() override;

    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmDecoder.setWhitelist(messageIds); }

    const NTRIPConnectionConfig& config() const { return _config; }

private:
    void _connect();
    void _fail(NTRIPError code, const QString& msg, std::chrono::milliseconds retryAfter = {});
    void _retireSocket();
    bool _write(const QByteArray& bytes);
    void _sendHttpRequest();
    void _readBytes();
    void _processHttpBytes(QByteArrayView bytes, qint64 receivedAtMs,
                           const QDateTime& utcNow = QDateTime::currentDateTimeUtc());
    void _publishHttpResult(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs);
    void _finishResponse();
    void _parseRtcm(const QByteArray& buffer,
                    qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000));

    NTRIPConnectionConfig _config;

    QPointer<QTcpSocket> _socket;
    QChronoTimer _connectTimeoutTimer;
    QChronoTimer _dataWatchdogTimer;
    QChronoTimer _errorBodyTimer;

    RTCMFrameDecoder _rtcmDecoder;
    NTRIPHttpDecoder _httpDecoder;
    bool _reading = false;
    bool _stopped = false;
    quint64 _attempt = 0;
};
