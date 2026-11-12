#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QPointer>
#include <QtCore/QString>

#include "NTRIPConfiguration.h"
#include "NTRIPTransport.h"
#include "RTCMFramer.h"
#include "ScheduledTask.h"

class NTRIPCasterVersions;
class NTRIPHttpSession;
class RuntimeScheduler;

class NTRIPHttpTransport : public NTRIPTransport
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr std::chrono::milliseconds CONNECT_TIMEOUT{10000};
    /// How long a stream that has delivered data may stall.
    static constexpr std::chrono::milliseconds DATA_WATCHDOG{30000};
    /// How long a stream that was told a position may take over its first data. A network RTK (VRS) caster works out
    /// corrections for the position first: Mission Planner's client allows for "60+ seconds", and BKG's reference
    /// client gives up after two minutes without data.
    static constexpr std::chrono::milliseconds FIRST_DATA_TIMEOUT{120000};

    /// @a casterVersions is shared by the attempts and source-table fetches of one caster, so that each asks the
    /// caster in the form it answered before.
    explicit NTRIPHttpTransport(const NTRIPConnectionConfig& config, QObject* parent = nullptr,
                                RuntimeScheduler* scheduler = nullptr,
                                std::shared_ptr<NTRIPCasterVersions> casterVersions = {});
    ~NTRIPHttpTransport() override;

    void start() override;
    void stop() override;
    void sendNMEA(const QByteArray& nmea) override;

private:
    /// Makes @a session the live attempt and follows its response; also the test seam for socketless sessions.
    NTRIPHttpSession* _attachSession(NTRIPHttpSession* session);
    qint64 _nowMs() const;
    void _startConnectTimeout();
    /// The stream is due to deliver from now on: starts the stall clock unless it runs. A network RTK (VRS) mountpoint
    /// sends nothing before it is told a position, so the response alone does not start it.
    void _expectData();
    /// How long the stream may go without a valid frame: longer until its first data than between corrections.
    std::chrono::milliseconds _stallTimeout() const;
    void _startDataWatchdog(std::chrono::milliseconds delay);
    void _fail(const NTRIPFailure& failure);
    void _retireSession();
    /// Not stopped, and @a session is still the live one: an observer did not stop or restart this transport.
    bool _isCurrent(const NTRIPHttpSession* session) const;
    void _stopTimers();
    void _onResponseStarted();
    void _parseRtcm(const QByteArray& buffer, qint64 receivedAtMs);

    NTRIPConnectionConfig _config;
    const std::shared_ptr<NTRIPCasterVersions> _casterVersions;

    QPointer<NTRIPHttpSession> _session;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _connectTimeoutTask;
    ScheduledTask _dataWatchdogTask;

    RTCMFrameDecoder _rtcmDecoder;
    qint64 _lastValidFrameMs = 0;
    /// The live attempt's stream has delivered data.
    bool _dataReceived = false;
    bool _dataSinceValidFrame = false;
    bool _stopped = false;
};
