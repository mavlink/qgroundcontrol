#pragma once

#include <chrono>
#include <functional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSNotificationQueue.h"
#include "GPSRevision.h"
#include "NTRIPConfiguration.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTransport.h"
#include "RTCMDecodedFrame.h"
#include "ScheduledTask.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)

class GPSCorrectionManager;
class RuntimeScheduler;

/// Manages the NTRIP caster connection lifecycle as an explicit event-driven
/// state machine. All connection state changes flow through `_dispatch()` and
/// the transition table in NTRIPManager.cc — there is no second, internal
/// state enum. Entry actions own per-state side effects (start/tear down
/// transport, schedule reconnect, toggle GGA/stats).
class NTRIPManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("NTRIPConnectionStats.h")
    Q_MOC_INCLUDE("NTRIPSourceTableController.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged)
    Q_PROPERTY(NTRIPSourceTableController* sourceTableController READ sourceTableController CONSTANT)
    Q_PROPERTY(NTRIPConnectionStats* connectionStats READ connectionStats CONSTANT)

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

    /// State-machine events. Each represents an external stimulus; the
    /// transition table in NTRIPManager.cc maps (state, event) → next state.
    /// Public so test code can drive the machine directly — entry actions
    /// are internal, events are the observable API.
    enum class Event
    {
        StartRequested,       ///< startNTRIP() called or settings enable went true.
        StopRequested,        ///< stopNTRIP() called or settings enable went false.
        ConfigInvalid,        ///< Connection configuration failed validation.
        TransportConnected,   ///< NTRIPTransport emitted connected().
        RTCMBeforeConnected,  ///< RTCM data arrived before the connected() signal was processed.
        TransportError,       ///< NTRIPTransport emitted a retryable error.
        TransportFatalError,  ///< NTRIPTransport emitted a non-retryable error.
        ReconnectDue,         ///< NTRIPReconnectPolicy fired reconnectRequested().
        ReconnectGaveUp,      ///< NTRIPReconnectPolicy fired gaveUp().
        HotReconfigure,       ///< Transport-affecting setting changed while connected; reconnect in place.
    };

    /// Position that orders fetched mountpoints by distance; an invalid coordinate keeps the caster's order.
    using SortPositionProvider = std::function<QGeoCoordinate()>;

    /// The NTRIP settings as values; the application's settings binding supplies them.
    struct Configuration
    {
        bool enabled = false;
        NTRIPConfiguration stream;
        NTRIPGgaProvider::Configuration gga;
        bool operator==(const Configuration&) const = default;
    };

    explicit NTRIPManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NTRIPManager() override;

    /// Applies the configuration; call once the correction manager and providers are injected.
    void init();
    /// Before init() the configuration is only stored. After it, stream changes apply after a short debounce, so
    /// editing a field does not reconnect per keystroke, and GGA changes apply at once.
    void setConfiguration(const Configuration& configuration);

    const Configuration& configuration() const { return _configuration; }

    ConnectionStatus connectionStatus() const { return _connectionStatus; }

    QString statusMessage() const { return _statusMessage; }

    QString securityWarning() const { return _securityWarning; }

    QString ggaSource() const { return _ggaProvider.currentSource(); }

    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }

    NTRIPConnectionStats* connectionStats() { return &_stats; }

    Q_INVOKABLE void fetchMountpoints();

    Q_INVOKABLE void selectMountpoint(const QString& mountpoint)
    {
        _sourceTableController.selectMountpoint(mountpoint);
    }

    /// Test seam: inject a transport (e.g. MockNTRIPTransport) consumed by the
    /// next Connecting entry. Production always constructs NTRIPHttpTransport.
    void setTransportForTest(NTRIPTransport* transport) { _injectedTransport = transport; }

    /// Inject before init(); the caller retains ownership.
    void setCorrectionManager(GPSCorrectionManager* manager);

    void setGgaPositionProvider(NTRIPGgaProvider::PositionSource source, NTRIPGgaProvider::PositionProvider provider);

    void setSortPositionProvider(SortPositionProvider provider) { _sortPositionProvider = std::move(provider); }

    /// Explicit start/retry begins a fresh retry budget; it is a no-op while already active.
    Q_INVOKABLE void startNTRIP();
    /// Retry a user-visible error, preserving the enabled setting for subsequent automatic retries.
    Q_INVOKABLE void retryNTRIP();
    void stopNTRIP();
    /// Permanently retire this manager, including deferred settings work.
    void shutdown();

signals:
    void connectionStatusChanged();
    void statusMessageChanged();
    void securityWarningChanged();
    void ggaSourceChanged();
    /// The user chose a mountpoint from the source table; the settings should store it.
    void mountpointChosen(const QString& mountpoint);
    /// Retrying after an error turns the saved connection on.
    void enableRequested();

private:
    /// Dispatch an event. Returns true if a transition was found and taken.
    /// Events with no matching row for the current state are ignored (debug log).
    bool _dispatch(Event ev, const QString& detail = {}, std::chrono::milliseconds retryAfter = {});

    /// Commit a state change before invoking entry actions, so recursive dispatches from entry
    /// actions observe the new state. Change signals are delivered after the outermost operation.
    void _enterState(ConnectionStatus to, const QString& detail, std::chrono::milliseconds retryAfter = {});

    /// Per-state side effects (start transport, tear down, schedule reconnect, etc.).
    void _onEnterState(ConnectionStatus from, ConnectionStatus to, std::chrono::milliseconds retryAfter);

    /// Default user-visible message for a state. Callers may override via detail.
    static QString _defaultMessageFor(ConnectionStatus state);

    void _startTransport();
    void _teardownTransport();
    bool _stopStreaming();

    // Reconnect backoff (inlined; was NTRIPReconnectPolicy). The single-shot
    // timer fires reconnectRequested → ReconnectDue; exhausting the attempt
    // ceiling fires ReconnectGaveUp instead.
    static constexpr int kMinReconnectMs = 1000;
    static constexpr int kMaxReconnectMs = 30000;
    static constexpr int kMaxReconnectAttempts = 100;

    void _scheduleReconnect(std::chrono::milliseconds retryAfter = {});

    void _cancelReconnect();

    void _resetReconnectAttempts() { _reconnectAttempts = 0; }

    int _reconnectBackoffMs(std::chrono::milliseconds retryAfter = {}) const;

    bool _reconnectExhausted() const { return _reconnectAttempts >= kMaxReconnectAttempts; }

    /// Reconfigure the manager-owned NTRIP sink without restarting transport.

    void _onTransportError(const NTRIPFailure& failure);
    void _onPlaintextCredentialsWarning();
    void _setSecurityWarning(const QString& warning);
    void _rtcmDataReceived(const RTCMDecodedFrame& frame);
    /// Brings the connection in line with the latest configuration.
    void _applyConfiguration();

    bool _isEnabled() const { return _configuration.enabled; }

    RuntimeScheduler* const _scheduler;
    ScheduledTask _settingsDebounceTask;
    ScheduledTask _reconnectTask;
    NTRIPGgaProvider _ggaProvider;
    NTRIPConnectionStats _stats{this};

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    QString _securityWarning;

    QPointer<NTRIPTransport> _injectedTransport;
    QPointer<NTRIPTransport> _transport;

    QPointer<GPSCorrectionManager> _correctionManager;
    GPSCorrectionSourceRegistration _correctionRegistration;

    // Latest supplied configuration, and the one the running transport uses.
    Configuration _configuration;
    NTRIPConfiguration _runningConfig;
    SortPositionProvider _sortPositionProvider;

    NTRIPSourceTableController _sourceTableController;

    static constexpr std::chrono::milliseconds kSettingsDebounceMs{250};
    std::chrono::milliseconds _pendingReconnectDelay{};
    int _reconnectAttempts = 0;
    bool _initialized = false;
    bool _shutdown = false;
    GPSRevision _stateRevision;
    GPSNotificationQueue _notifications{this};
};
