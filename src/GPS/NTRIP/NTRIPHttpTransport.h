#pragma once

#include <chrono>

#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSRevision.h"
#include "MonotonicClock.h"
#include "NTRIPConfiguration.h"
#include "NTRIPHttpCodec.h"
#include "NTRIPHttpSession.h"
#include "NTRIPTransport.h"
#include "RTCMFrameDecoder.h"
#include "ScheduledTask.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPHttpTransportLog)

class RuntimeScheduler;

class NTRIPHttpTransport : public NTRIPTransport
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr std::chrono::milliseconds kConnectTimeout{10000};
    static constexpr std::chrono::milliseconds kDataWatchdog{30000};
    static constexpr std::chrono::milliseconds kErrorBodyTimeout{250};

    NTRIPHttpTransport(const NTRIPConnectionConfig& config, const NTRIPRtcmFilterConfig& filter,
                       QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NTRIPHttpTransport() override;

    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmDecoder.setWhitelist(messageIds); }

    const NTRIPConnectionConfig& config() const { return _config; }

private:
    void _connect();
    qint64 _nowMs() const;
    void _startConnectTimeout();
    void _startDataWatchdog();
    void _startValidFrameWatchdog();
    void _startErrorBodyTimeout();
    void _fail(NTRIPError code, const QString& msg, std::chrono::milliseconds retryAfter = {});
    void _retireSession();
    void _stopTimers();
    bool _write(const QByteArray& bytes);
    void _sendHttpRequest();
    void _processHttpBytes(QByteArrayView bytes, qint64 receivedAtMs,
                           const QDateTime& utcNow = QDateTime::currentDateTimeUtc());
    void _publishHttpResult(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs);
    void _finishResponse();
    void _parseRtcm(const QByteArray& buffer,
                    qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000));

    NTRIPConnectionConfig _config;

    QPointer<NTRIPHttpSession> _session;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _connectTimeoutTask;
    ScheduledTask _dataWatchdogTask;
    ScheduledTask _validFrameWatchdogTask;
    ScheduledTask _errorBodyTask;

    RTCMFrameDecoder _rtcmDecoder;
    NTRIPHttpDecoder _httpDecoder;
    bool _stopped = false;
    GPSRevision _attempt;
};
