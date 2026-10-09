#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "ExponentialBackoff.h"
#include "NTRIPConfiguration.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPGgaReporter.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTransport.h"
#include "ScheduledTask.h"

class GPSCorrectionManager;
class GPSCorrectionSourceHandle;
class QGCNetworkAvailabilityMonitor;
class RuntimeScheduler;

/// The RTCM message filter as the settings store it: comma-separated message IDs, empty for every message.
struct NTRIPRTCMFilterConfig
{
    QString whitelist;
    bool operator==(const NTRIPRTCMFilterConfig&) const = default;

    /// The positive message IDs of the whitelist; entries that are not one are skipped.
    [[nodiscard]] QVector<int> messageIds() const;
};

/// Services an NTRIPManager uses, which must outlive it.
struct NTRIPManagerDependencies
{
    /// Receives the stream as the NTRIP correction source.
    GPSCorrectionManager* corrections = nullptr;
    /// Reconnects wait for a network while it reports none.
    QGCNetworkAvailabilityMonitor* network = nullptr;
};

/// Manages the NTRIP caster connection lifecycle as an explicit event-driven
/// state machine. All connection state changes flow through `_dispatch()` and
/// the transition table in NTRIPManager.cc — there is no second, internal
/// state enum. Entry actions own per-state side effects: each Connecting entry
/// opens a new transport and registers it as the NTRIP correction source,
/// teardown retires both, and the manager schedules reconnects.
class NTRIPManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the GPS manager")
    Q_MOC_INCLUDE("NTRIPConnectionStats.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)
    /// Short connection state for compact views.
    Q_PROPERTY(QString connectionStatusText READ connectionStatusText NOTIFY connectionStatusChanged FINAL)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged FINAL)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged FINAL)
    Q_PROPERTY(NTRIPSourceTableController* sourceTableController READ sourceTableController CONSTANT FINAL)
    Q_PROPERTY(NTRIPConnectionStats* connectionStats READ connectionStats CONSTANT FINAL)

    friend class NTRIPManagerTest;

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

    /// Internal state-machine events, public only so the log names them. Each represents an external stimulus; the
    /// transition table in _dispatch() maps (state, event) to the next state.
    enum class Event
    {
        StartRequested,       ///< Settings enable went true, or the user retried an error.
        StopRequested,        ///< Settings enable went false, or the manager shut down.
        ConfigInvalid,        ///< Connection configuration failed validation.
        TransportConnected,   ///< NTRIPTransport emitted connected().
        TransportError,       ///< NTRIPTransport emitted a retryable error.
        TransportFatalError,  ///< NTRIPTransport emitted a non-retryable error.
        ReconnectDue,         ///< The reconnect delay elapsed or the network returned.
        ReconnectGaveUp,      ///< The reconnect attempt ceiling was reached.
        HotReconfigure,       ///< Transport-affecting setting changed while connected; reconnect in place.
    };
    Q_ENUM(Event)

    /// The NTRIP settings as values; the application's settings binding supplies them.
    struct Configuration
    {
        bool enabled = false;
        NTRIPConnectionConfig connection;
        NTRIPRTCMFilterConfig filter;
        NTRIPGgaReporter::Configuration gga;
        bool operator==(const Configuration&) const = default;
    };

    using Dependencies = NTRIPManagerDependencies;

    explicit NTRIPManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                          const Dependencies& dependencies = {});
    ~NTRIPManager() override;

    /// Applies the configuration; call once the position providers are installed.
    void init();
    /// Before init() the configuration is only stored. After it, stream changes apply after a short debounce, so
    /// editing a field does not reconnect per keystroke, and GGA changes apply at once.
    void setConfiguration(const Configuration& configuration);

    const Configuration& configuration() const { return _configuration; }

    ConnectionStatus connectionStatus() const { return _connectionStatus; }

    QString connectionStatusText() const;

    QString statusMessage() const { return _statusMessage; }

    /// Set while a connected stream sends its credentials without TLS.
    QString securityWarning() const;

    QString ggaSource() const { return _ggaReporter.currentSource(); }

    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }

    NTRIPConnectionStats* connectionStats() { return &_stats; }

    /// Fetches the caster's mountpoints, ordered by distance from @a sortCoordinate; an invalid coordinate keeps the
    /// caster's order.
    Q_INVOKABLE void fetchMountpoints(const QGeoCoordinate& sortCoordinate = QGeoCoordinate());

    /// Creates the transport of one connection attempt, parented to @a parent; null opens an NTRIPHttpTransport.
    using TransportFactory = std::function<NTRIPTransport*(const Configuration& configuration, QObject* parent)>;

    /// An empty factory restores NTRIPHttpTransport for every attempt.
    void setTransportFactory(TransportFactory factory) { _transportFactory = std::move(factory); }

    void setGgaPositionProvider(NTRIPGgaReporter::PositionSource source, NTRIPGgaReporter::PositionProvider provider);

    /// Retry a user-visible error, preserving the enabled setting for subsequent automatic retries.
    Q_INVOKABLE void retryNTRIP();
    /// Permanently retire this manager, including deferred settings work.
    void shutdown();

signals:
    void connectionStatusChanged();
    void statusMessageChanged();
    void securityWarningChanged();
    void ggaSourceChanged();
    /// Retrying after an error turns the saved connection on.
    void enableRequested();
    /// The trusted self-signed caster certificate changed, or was forgotten (empty); the settings should store it.
    void certificatePinChanged(const QString& pin);

private:
    /// What the change signals last announced.
    struct Notified
    {
        ConnectionStatus status = ConnectionStatus::Disconnected;
        QString message;
        QString securityWarning;
    };

    /// Emits one change signal for each property that differs from what the signals last announced. Public operations
    /// and event handlers call it once the state machine has settled, so observers never see a half-entered state.
    void _publish();
    /// Starts with a fresh retry budget, unless already connecting or connected, and publishes the result.
    void _start();
    /// Stops, cancelling deferred settings work and reconnects, and publishes the result.
    void _stop();

    /// Dispatch an event. Returns true if a transition was found and taken.
    /// Events with no matching row for the current state are ignored (debug log).
    bool _dispatch(Event ev, const QString& detail = {}, std::chrono::milliseconds retryAfter = {});

    /// Commit a state change before invoking entry actions, so recursive dispatches from entry
    /// actions observe the new state.
    void _enterState(ConnectionStatus to, const QString& detail, std::chrono::milliseconds retryAfter = {});

    /// Per-state side effects (open or retire the transport, schedule reconnect, etc.).
    void _onEnterState(ConnectionStatus to, std::chrono::milliseconds retryAfter);

    /// Default user-visible message for a state. Callers may override via detail.
    static QString _defaultMessageFor(ConnectionStatus state);

    /// Stops the previous stream, validates the configuration, then opens a new transport and registers it as the
    /// NTRIP correction source.
    void _openSession();
    /// Ends the correction source, retires the transport, and stops GGA and statistics.
    void _stopStreaming();

    // The single-shot reconnect timer fires ReconnectDue; a failure after the attempt ceiling fires ReconnectGaveUp.
    static constexpr int MAX_RECONNECT_ATTEMPTS = 100;

    void _scheduleReconnect(std::chrono::milliseconds retryAfter = {});

    void _cancelReconnect();

    bool _shouldWaitForNetwork() const;
    bool _casterIsLoopback() const;
    void _waitForNetwork();

    bool _reconnectExhausted() const { return _reconnectBackoff.attempts() >= MAX_RECONNECT_ATTEMPTS; }

    void _onTransportConnected();
    void _onTransportError(const NTRIPFailure& failure);
    void _onCertificatePinned(const QString& pin);
    void _setPinnedCertificate(const QString& pin);
    void _onCorrectionFrame(const RTCMDecodedFrame& frame);
    /// Brings the connection in line with the latest configuration.
    void _applyConfiguration();

    bool _isEnabled() const { return _configuration.enabled; }

    RuntimeScheduler* const _scheduler;
    ScheduledTask _settingsDebounceTask;
    ScheduledTask _reconnectTask;
    NTRIPGgaReporter _ggaReporter;
    NTRIPConnectionStats _stats;

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    Notified _notified;

    TransportFactory _transportFactory;
    QPointer<NTRIPTransport> _transport;
    /// The stream's registration as the NTRIP correction source.
    std::unique_ptr<GPSCorrectionSourceHandle> _correctionSource;

    const QPointer<GPSCorrectionManager> _correctionManager;
    const QPointer<QGCNetworkAvailabilityMonitor> _networkMonitor;

    // Latest supplied configuration, and the one the current transport uses.
    Configuration _configuration;
    Configuration _running;

    NTRIPSourceTableController _sourceTableController;

    static constexpr std::chrono::milliseconds SETTINGS_DEBOUNCE{250};
    ExponentialBackoff _reconnectBackoff{std::chrono::seconds(1), 2, std::chrono::seconds(30),
                                         NTRIPFailure::MAX_RETRY_AFTER};
    QString _lastFailureDetail;  ///< The latest transport failure, for the give-up message.
    bool _waitingForNetwork = false;
    bool _initialized = false;
    bool _shutdown = false;
};
