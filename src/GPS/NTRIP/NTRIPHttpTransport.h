#pragma once

#include <QtCore/QLoggingCategory>

#include "GPSScheduledTask.h"
#include "NTRIPHttpResponse.h"
#include "NTRIPStream.h"
#include "RTCMFrameDecoder.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPHttpTransportLog)

/// Correction-specific framing and liveness over the shared NTRIP response transaction.
class NTRIPHttpTransport : public NTRIPStream
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr auto kConnectTimeout = NTRIPHttpResponse::kConnectTimeout;
    static constexpr std::chrono::milliseconds kDataWatchdog{30000};

    explicit NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent = nullptr,
                                GPSRuntimeScheduler* scheduler = nullptr);
    ~NTRIPHttpTransport() override;
    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmDecoder.setWhitelist(messageIds); }

    const NTRIPTransportConfig& config() const { return _config; }

private:
    void _fail(const NTRIPFailure& failure);
    void _parseRtcm(const QByteArray& buffer);
    void _armWatchdog();

    NTRIPTransportConfig _config;
    QPointer<GPSRuntimeScheduler> _scheduler;
    GPSScheduledTask _dataWatchdog;
    NTRIPHttpResponse _response;
    RTCMFrameDecoder _rtcmDecoder;
    quint64 _generation = 0;
    qint64 _receivedAtMs = 0;
    bool _stopped = false;
};
