#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>
#include <chrono>

#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTransport.h"
#include "NTRIPTransportConfig.h"
#include "UdpForwarder.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)

class NTRIPSettings;
class RTCMMavlink;
class RTCMUdpInput;

/// Manages the NTRIP caster connection lifecycle as an explicit event-driven
/// state machine. All connection state changes flow through `_dispatch()` and
/// the transition table in NTRIPManager.cc — there is no second, internal
/// state enum. Entry actions own per-state side effects (start/tear down
/// transport, schedule reconnect, toggle GGA/stats).
class NTRIPManager : public QObject
{
    Q_OBJECT
    friend class NTRIPManagerTest;
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("NTRIPConnectionStats.h")
    Q_MOC_INCLUDE("NTRIPSourceTableController.h")
    Q_MOC_INCLUDE("RTCMMavlink.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged)
    Q_PROPERTY(CasterStatus casterStatus READ casterStatus NOTIFY casterStatusChanged)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged)
    Q_PROPERTY(NTRIPSourceTableController* sourceTableController READ sourceTableController CONSTANT)
    Q_PROPERTY(NTRIPConnectionStats* connectionStats READ connectionStats CONSTANT)
    // CONSTANT is safe: rtcmMavlink is established once in init() (or injected for
    // tests) before any QML binds, and is never reassigned thereafter.
    Q_PROPERTY(RTCMMavlink* rtcmMavlink READ rtcmMavlink CONSTANT)

public:
    /// Public connection status. Numeric values are stable — QML binds against them.
    enum class ConnectionStatus
    {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        Reconnecting = 3,
        Error = 4
    };
    Q_ENUM(ConnectionStatus)

    enum class CasterStatus
    {
        CasterConnected,
        CasterNoLocation,
        CasterError
    };
    Q_ENUM(CasterStatus)

    /// State-machine events. Each represents an external stimulus; the
    /// transition table in NTRIPManager.cc maps (state, event) → next state.
    /// Public so test code can drive the machine directly — entry actions
    /// are internal, events are the observable API.
    enum class Event
    {
        StartRequested,       ///< startNTRIP() called or settings enable went true.
        StopRequested,        ///< stopNTRIP() called or settings enable went false.
        ConfigInvalid,        ///< NTRIPTransportConfig::isValid() returned false.
        TransportConnected,   ///< NTRIPTransport emitted connected().
        RTCMBeforeConnected,  ///< RTCM data arrived before the connected() signal was processed.
        TransportError,       ///< NTRIPTransport emitted a retryable error.
        TransportFatalError,  ///< NTRIPTransport emitted a non-retryable error.
        ReconnectDue,         ///< NTRIPReconnectPolicy fired reconnectRequested().
        ReconnectGaveUp,      ///< NTRIPReconnectPolicy fired gaveUp().
        HotReconfigure,       ///< Transport-affecting setting changed while connected; reconnect in place.
    };

    explicit NTRIPManager(QObject* parent = nullptr);
    ~NTRIPManager() override;

    static NTRIPManager* instance();

    /// Explicit post-construction init. Must be called from QGCApplication after
    /// SettingsManager is ready — matches QGCPositionManager::init(). Do not rely
    /// on the singleton constructor for anything that touches other singletons.
    void init();

    ConnectionStatus connectionStatus() const { return _connectionStatus; }

    QString statusMessage() const { return _statusMessage; }

    QString securityWarning() const { return _securityWarning; }

    CasterStatus casterStatus() const { return _casterStatus; }

    QString ggaSource() const { return _ggaProvider.currentSource(); }

    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }

    NTRIPConnectionStats* connectionStats() { return &_stats; }

    /// Shared RTCM→MAVLink forwarder (created in init()). GPSRtk routes its serial-RTK
    /// corrections through this same instance for one GPS_RTCM_DATA sequence-id domain.
    RTCMMavlink* rtcmMavlink() const;

    Q_INVOKABLE void fetchMountpoints();

    Q_INVOKABLE void selectMountpoint(const QString& mountpoint)
    {
        _sourceTableController.selectMountpoint(mountpoint);
    }

    /// Test seam: inject a transport (e.g. MockNTRIPTransport) consumed by the
    /// next Connecting entry. Production always constructs NTRIPHttpTransport.
    void setTransportForTest(NTRIPTransport* transport) { _injectedTransport = transport; }

    void setRtcmMavlink(RTCMMavlink* mavlink);

    void startNTRIP();
    void stopNTRIP();

signals:
    void connectionStatusChanged();
    void statusMessageChanged();
    void securityWarningChanged();
    void casterStatusChanged(CasterStatus status);
    void ggaSourceChanged();

private:
    /// Dispatch an event. Returns true if a transition was found and taken.
    /// Events with no matching row for the current state are ignored (debug log).
    bool _dispatch(Event ev, const QString& detail = {});

    /// Commit a state change. Updates _connectionStatus/_statusMessage and
    /// emits change signals *before* invoking entry actions so recursive
    /// dispatches from entry actions observe the new state, not the old.
    void _enterState(ConnectionStatus to, const QString& detail);

    /// Per-state side effects (start transport, tear down, schedule reconnect, etc.).
    void _onEnterState(ConnectionStatus from, ConnectionStatus to);

    /// Default user-visible message for a state. Callers may override via detail.
    static QString _defaultMessageFor(ConnectionStatus state);

    void _startTransport();
    void _teardownTransport();

    // Reconnect backoff (inlined; was NTRIPReconnectPolicy). The single-shot
    // timer fires reconnectRequested → ReconnectDue; exhausting the attempt
    // ceiling fires ReconnectGaveUp instead.
    static constexpr int kMinReconnectMs = 1000;
    static constexpr int kMaxReconnectMs = 30000;
    static constexpr int kMaxReconnectAttempts = 100;

    void _scheduleReconnect();

    void _cancelReconnect() { _reconnectTimer.stop(); }

    void _resetReconnectAttempts() { _reconnectAttempts = 0; }

    int _reconnectBackoffMs() const;

    bool _reconnectExhausted() const { return _reconnectAttempts >= kMaxReconnectAttempts; }

    /// Apply UDP-forwarder config in place (no transport touch). Used by both
    /// transport startup and the "warm setting changed while running" path.
    void _applyUdpForwarderConfig(const NTRIPTransportConfig& config);

    void _onTransportError(NTRIPError code, const QString& detail);
    void _onPlaintextCredentialsWarning();
    void _setSecurityWarning(const QString& warning);
    void _rtcmDataReceived(const QByteArray& data, int messageId);
    void _onSettingChanged();
    bool _isEnabled() const;

    /// Wire RTCMUdpInput (inbound RTCM over UDP → RTCMMavlink). Complementary to
    /// UdpForwarder, which sends outbound. Driven by rtcmUdpInput* settings.
    void _setupRtcmUdpInput();

    NTRIPGgaProvider _ggaProvider{this};
    NTRIPConnectionStats _stats{this};
    UdpForwarder _udpForwarder{this};

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    QString _securityWarning;
    CasterStatus _casterStatus = CasterStatus::CasterError;

    QPointer<NTRIPTransport> _injectedTransport;
    QPointer<NTRIPTransport> _transport;

    QPointer<RTCMMavlink> _rtcmMavlink;
    RTCMUdpInput* _rtcmUdpInput = nullptr;

    NTRIPTransportConfig _runningConfig;
    NTRIPSettings* _settings = nullptr;

    NTRIPSourceTableController _sourceTableController{this};

    static constexpr std::chrono::milliseconds kSettingsDebounceMs{250};
    QChronoTimer _settingsDebounceTimer{this};
    QChronoTimer _reconnectTimer{this};
    int _reconnectAttempts = 0;
    bool _initialized = false;
};
