#pragma once

#include <chrono>

#include <QtCore/QPointer>
#include <QtCore/QString>

#include "NTRIPConfiguration.h"
#include "NTRIPTransport.h"
#include "RTCMFramer.h"
#include "ScheduledTask.h"

class NTRIPHttpSession;
class RuntimeScheduler;

class NTRIPHttpTransport : public NTRIPTransport
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr std::chrono::milliseconds CONNECT_TIMEOUT{10000};
    static constexpr std::chrono::milliseconds DATA_WATCHDOG{30000};

    NTRIPHttpTransport(const NTRIPConnectionConfig& config, const QVector<int>& rtcmWhitelist,
                       QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NTRIPHttpTransport() override;

    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

    void setRtcmWhitelist(const QVector<int>& messageIds) override { _rtcmDecoder.setWhitelist(messageIds); }

private:
    /// Makes @a session the live attempt and follows its response; also the test seam for socketless sessions.
    NTRIPHttpSession* _attachSession(NTRIPHttpSession* session);
    qint64 _nowMs() const;
    void _startConnectTimeout();
    void _startDataWatchdog(std::chrono::milliseconds delay);
    void _fail(const NTRIPFailure& failure);
    void _retireSession();
    /// Not stopped, and @a session is still the live one: an observer did not stop or restart this transport.
    bool _isCurrent(const NTRIPHttpSession* session) const;
    void _stopTimers();
    void _onResponseStarted();
    void _parseRtcm(const QByteArray& buffer, qint64 receivedAtMs);

    NTRIPConnectionConfig _config;

    QPointer<NTRIPHttpSession> _session;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _connectTimeoutTask;
    ScheduledTask _dataWatchdogTask;

    RTCMFrameDecoder _rtcmDecoder;
    qint64 _lastValidFrameMs = 0;
    bool _dataSinceValidFrame = false;
    bool _stopped = false;
};
