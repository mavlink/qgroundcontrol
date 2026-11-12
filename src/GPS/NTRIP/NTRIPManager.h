#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
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
class NTRIPCasterVersions;
class NTRIPVehiclePresence;
class QGCNetworkAvailabilityMonitor;
class RuntimeScheduler;

/// Services an NTRIPManager uses, which must outlive it.
struct NTRIPManagerDependencies
{
    /// Receives the stream as the NTRIP correction source.
    GPSCorrectionManager* corrections = nullptr;
    /// Reconnects wait for a network while it reports none.
    QGCNetworkAvailabilityMonitor* network = nullptr;
    /// With Configuration::requireVehicle, the connection runs only while it reports a vehicle.
    NTRIPVehiclePresence* vehicles = nullptr;
};

/// Manages the NTRIP caster connection lifecycle as an explicit event-driven
/// state machine. All connection state changes flow through `_dispatch()` and
/// the transition table in NTRIPManager.cc — there is no second, internal
/// state enum. Entry actions own per-state side effects: each Connecting entry
/// registers the stream as the NTRIP correction source, in place of the previous
/// registration, and opens a new transport; Reconnecting, Error and Disconnected
/// retire the transport; only Disconnected releases the registration; and the
/// manager schedules reconnects.
///
/// Failover (Configuration::failover) goes on to the saved casters when the caster in use cannot be reached:
/// - Order: the caster that is entered, then the saved casters in their order, then the entered one again. A saved
///   caster takes no part when it is the entered one, has no mountpoint or is otherwise not valid, or accepts
///   self-signed certificates without one saved with it to check against.
/// - A caster is left for the next after FAILOVER_ATTEMPTS failed attempts in a row without corrections between
///   them. Only failures that are retried count. The delays between attempts are the same as for one caster, across
///   casters, and so is the limit of MAX_RECONNECT_ATTEMPTS; what a caster asks for in Retry-After delays only an
///   attempt on that caster.
/// - A failure that is not retried, such as a refused login or a mountpoint the caster does not have, still ends
///   the connection with that error when the caster is the entered one. A saved caster that fails so is left out
///   until the connection is started again, and the next one is tried.
/// - Nothing returns to an earlier caster while the one in use serves: only the order above does, after the last
///   saved caster. Every start of the connection begins with the entered caster: Connect, Retry, a change of the
///   connection settings, and, while a saved caster is in use, a change of the saved casters or of failover.
/// - While the caster in use, or the next to be tried, is a saved one, the status message ends with its name.
///
/// With Configuration::requireVehicle and a Dependencies::vehicles to ask, the connection runs only while a vehicle
/// is connected. Without one it opens no transport and waits, in the Reconnecting state, saying so; a vehicle starts
/// it at once, from the entered caster and with the first retry delay. When the last vehicle is gone, the stream is
/// kept for VEHICLE_HOLD, so that a link that drops and returns does not restart it, and then ended. Turning the
/// option on applies like any settings change, without that hold.
class NTRIPManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the GPS manager")
    Q_MOC_INCLUDE("NTRIPConnectionStats.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged FINAL)
    /// What the connection is doing, or why it stopped; never empty.
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged FINAL)
    /// The mountpoint of the stream that is connected or being connected: the entered caster's or, during failover,
    /// a saved caster's. Empty while the connection is off or ended with an error.
    Q_PROPERTY(QString streamMountpoint READ streamMountpoint NOTIFY streamMountpointChanged FINAL)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged FINAL)
    /// A caster host is configured, so there is a caster to connect to and to ask for its mountpoints.
    Q_PROPERTY(bool canConnect READ canConnect NOTIFY canConnectChanged FINAL)
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
        Error = 4,
    };
    Q_ENUM(ConnectionStatus)

    /// Internal state-machine events, public only so the log names them. Each represents an external stimulus; the
    /// transition table in _dispatch() maps (state, event) to the next state.
    enum class Event
    {
        StartRequested,       ///< Settings enable went true, or the user retried an error.
        StopRequested,        ///< Settings enable went false, or the manager shut down.
        ConfigInvalid,        ///< The connection configuration failed validation.
        TransportConnected,   ///< NTRIPTransport emitted connected().
        TransportError,       ///< NTRIPTransport emitted a retryable error.
        TransportFatalError,  ///< NTRIPTransport emitted a non-retryable error.
        ReconnectDue,         ///< The reconnect delay elapsed or the network returned.
        ReconnectGaveUp,      ///< The reconnect attempt ceiling was reached.
        HotReconfigure,       ///< Transport-affecting setting changed while connected; reconnect in place.
        NoVehicle,            ///< The connection may run only with a vehicle, and none is connected.
    };
    Q_ENUM(Event)

    /// A caster kept under a name beside the one that is entered.
    struct SavedCaster
    {
        QString name;
        NTRIPConnectionConfig connection;
        bool operator==(const SavedCaster&) const = default;
    };

    /// The NTRIP settings as values; the application's settings binding supplies them.
    struct Configuration
    {
        bool enabled = false;
        NTRIPConnectionConfig connection;
        NTRIPGgaReporter::Configuration gga;
        /// The casters an earlier run found to take NTRIP version 1 requests only, as version1CastersChanged()
        /// reported them.
        QString version1Casters;
        /// Go on to savedCasters when the entered caster cannot be reached.
        bool failover = false;
        /// In the order failover takes them.
        QList<SavedCaster> savedCasters = {};
        /// Connect only while a vehicle is connected.
        bool requireVehicle = false;
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

    QString statusMessage() const;

    /// Set while a connected stream sends its credentials without TLS.
    QString securityWarning() const;

    QString streamMountpoint() const { return _running.connection.mountpoint; }

    QString ggaSource() const { return _ggaReporter.currentSource(); }

    bool canConnect() const { return !_configuration.connection.host.isEmpty(); }

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

    /// Turns the saved connection on, which connects after the settings debounce. After an error the connection is
    /// still on, so this starts a new attempt at once instead.
    Q_INVOKABLE void connectCaster();
    /// Turns the saved connection off, which disconnects after the settings debounce.
    Q_INVOKABLE void disconnectCaster();
    /// Permanently retire this manager, including deferred settings work.
    void shutdown();

signals:
    void connectionStatusChanged();
    void statusMessageChanged();
    void securityWarningChanged();
    void streamMountpointChanged();
    void ggaSourceChanged();
    void canConnectChanged();
    /// The user turned the saved connection on or off; the settings should store it.
    void enabledRequested(bool enabled);
    /// The trusted self-signed caster certificate changed, or was forgotten (empty); the settings should store it.
    void certificatePinChanged(const QString& pin);
    /// The casters found to take NTRIP version 1 requests only changed; the settings should store them for the next
    /// run.
    void version1CastersChanged(const QString& casters);

private:
    /// What the change signals last announced.
    struct Notified
    {
        ConnectionStatus status = ConnectionStatus::Disconnected;
        QString message;
        /// The saved caster the message names; empty for the entered one.
        QString savedCaster;
        QString securityWarning;
        QString streamMountpoint;
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
    /// The message of the Connected state. A network RTK (VRS) mountpoint sends nothing until it is told a position,
    /// and may then take a minute or more over its first corrections, so until corrections arrive the message says
    /// which of the two the stream waits for.
    QString _connectedMessage() const;
    /// Publishes _connectedMessage() while connected; the other states publish their own message.
    void _refreshConnectedMessage();

    /// Stops the previous stream, registers the new one as the NTRIP correction source in place of the previous
    /// registration, then validates the configuration and opens a new transport.
    void _openSession();
    /// Retires the transport and stops GGA and statistics. The correction source stays registered; it is selected
    /// only while its corrections are fresh.
    void _stopStreaming();

    // The single-shot reconnect timer fires ReconnectDue; a failure after the attempt ceiling fires ReconnectGaveUp.
    static constexpr int MAX_RECONNECT_ATTEMPTS = 100;
    /// Failed attempts in a row, without corrections, after which failover leaves a caster.
    static constexpr int FAILOVER_ATTEMPTS = 3;

    /// The connection of the caster in use: a saved one during failover, else the entered one.
    const NTRIPConnectionConfig& _casterConnection() const;
    /// Whether failover may go to @a caster.
    bool _usableForFailover(const SavedCaster& caster) const;
    /// Makes the next caster in failover's order the one in use. False, changing nothing, when there is no other.
    bool _goToNextCaster();
    /// Makes the entered caster the one in use, as at every start.
    void _useEnteredCaster();

    void _scheduleReconnect(std::chrono::milliseconds retryAfter = {});

    void _cancelReconnect();

    bool _shouldWaitForNetwork() const;
    bool _casterIsLoopback() const;
    void _waitForNetwork();

    /// How long the stream outlasts the last vehicle.
    static constexpr std::chrono::milliseconds VEHICLE_HOLD{10000};

    bool _shouldWaitForVehicle() const;
    void _waitForVehicle();
    void _onVehiclePresenceChanged(bool present);

    bool _reconnectExhausted() const { return _reconnectBackoff.attempts() >= MAX_RECONNECT_ATTEMPTS; }

    void _onGgaSourceChanged();
    void _onTransportConnected();
    void _onTransportError(const NTRIPFailure& failure);
    void _onCertificatePinned(const QString& pin);
    void _setPinnedCertificate(const QString& pin);
    void _onCasterVersionsChanged();
    void _onCorrectionFrame(const RTCMDecodedFrame& frame);
    /// Brings the connection in line with the latest configuration.
    void _applyConfiguration();

    bool _isEnabled() const { return _configuration.enabled; }

    RuntimeScheduler* const _scheduler;
    ScheduledTask _settingsDebounceTask;
    ScheduledTask _reconnectTask;
    ScheduledTask _vehicleHoldTask;
    NTRIPGgaReporter _ggaReporter;
    NTRIPConnectionStats _stats;

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    Notified _notified;

    TransportFactory _transportFactory;
    QPointer<NTRIPTransport> _transport;
    /// The stream's registration as the NTRIP correction source, from _openSession() until the next one or the
    /// Disconnected state.
    std::unique_ptr<GPSCorrectionSourceHandle> _correctionSource;

    const QPointer<GPSCorrectionManager> _correctionManager;
    const QPointer<QGCNetworkAvailabilityMonitor> _networkMonitor;
    const QPointer<NTRIPVehiclePresence> _vehicles;

    // Latest supplied configuration, and the one the current transport uses.
    Configuration _configuration;
    Configuration _running;

    /// The request form each caster takes, which the stream and the mountpoint browser find out for each other and
    /// the settings keep between runs.
    const std::shared_ptr<NTRIPCasterVersions> _casterVersions;
    NTRIPSourceTableController _sourceTableController;

    static constexpr std::chrono::milliseconds SETTINGS_DEBOUNCE{250};
    ExponentialBackoff _reconnectBackoff{std::chrono::seconds(1), 2, std::chrono::seconds(30),
                                         NTRIPFailure::MAX_RETRY_AFTER};
    QString _lastFailureDetail;  ///< The latest transport failure, for the give-up message.
    /// Corrections have arrived over the current connection settings since they were last started, so a source
    /// table in place of the stream means the mountpoint went offline, not that it is wrong.
    bool _streamServed = false;
    /// The saved caster in use during failover; none while it is the entered one.
    std::optional<SavedCaster> _savedCaster;
    /// The saved casters that failed in a way that is not retried, by name, since the connection was started.
    QStringList _castersLeftOut;
    /// Failed attempts on the caster in use since it was taken up or last sent corrections.
    int _casterFailures = 0;
    bool _waitingForNetwork = false;
    bool _waitingForVehicle = false;
    bool _initialized = false;
    bool _shutdown = false;
};
