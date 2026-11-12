#include "NTRIPManager.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <utility>

#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>

#include "GPSCorrectionManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpSession.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPVehiclePresence.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkAvailabilityMonitor.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIPManager")

namespace {

/// @a streamServed: the running connection has delivered corrections since it was started.
bool isRetryable(NTRIPError error, bool streamServed)
{
    // No default, so a new code is reported (-Wswitch) until it is classified.
    switch (error) {
        case NTRIPError::AuthFailed:
        case NTRIPError::InvalidConfig:
        case NTRIPError::RequestRejected:
        // TLS failures recur the same way until the user changes the TLS settings.
        case NTRIPError::SslError:
            return false;
        // A caster answers with its source table, or with 404 or 410, for a mountpoint it does not have. Before any
        // corrections that is a wrong mountpoint, and casters count repeated requests for a missing one towards a ban.
        // After them, the mountpoint has only gone offline.
        case NTRIPError::InvalidMountpoint:
        // Data without a valid RTCM message is, before any corrections, a mountpoint that sends another format.
        case NTRIPError::InvalidStream:
            return streamServed;
        case NTRIPError::ConnectionTimeout:
        case NTRIPError::DataWatchdog:
        case NTRIPError::SocketError:
        case NTRIPError::ServerDisconnected:
        case NTRIPError::InvalidHttpResponse:
        case NTRIPError::HttpError:
        case NTRIPError::HeaderTooLarge:
        case NTRIPError::Unknown:
            return true;
    }
    return true;
}

QString correctionSourceInstance(const NTRIPConnectionConfig& connection)
{
    QUrl endpoint = connection.url();
    endpoint.setScheme(connection.useTls ? QStringLiteral("ntrips") : QStringLiteral("ntrip"));
    return endpoint.toString(QUrl::FullyEncoded);
}

}  // namespace

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

NTRIPManager::NTRIPManager(QObject* parent, RuntimeScheduler* scheduler, const Dependencies& dependencies)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _settingsDebounceTask(_scheduler, this)
    , _reconnectTask(_scheduler, this)
    , _vehicleHoldTask(_scheduler, this)
    , _ggaReporter(this, _scheduler)
    // The display agrees with routing, which stops using a stream after the same age.
    , _stats(GPSCorrectionSelector::FRESHNESS_TIMEOUT, this, _scheduler)
    , _correctionManager(dependencies.corrections)
    , _networkMonitor(dependencies.network)
    , _vehicles(dependencies.vehicles)
    , _casterVersions(std::make_shared<NTRIPCasterVersions>())
    , _sourceTableController(this, _scheduler, _casterVersions)
{
    qCDebug(NTRIPManagerLog) << this;

    connect(&_ggaReporter, &NTRIPGgaReporter::sourceChanged, this, &NTRIPManager::_onGgaSourceChanged);

    connect(&_sourceTableController, &NTRIPSourceTableController::certificatePinned, this,
            &NTRIPManager::_setPinnedCertificate);
    connect(_casterVersions.get(), &NTRIPCasterVersions::changed, this, &NTRIPManager::_onCasterVersionsChanged);

    if (_networkMonitor) {
        connect(_networkMonitor, &QGCNetworkAvailabilityMonitor::availableChanged, this, [this](bool available) {
            if (!available || _shutdown || _connectionStatus != ConnectionStatus::Reconnecting || !_waitingForNetwork) {
                return;
            }
            _waitingForNetwork = false;
            _dispatch(Event::ReconnectDue);
            _publish();
        });
    }
    if (_vehicles) {
        connect(_vehicles, &NTRIPVehiclePresence::presentChanged, this, &NTRIPManager::_onVehiclePresenceChanged);
    }
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << this;
    // Observers must not see the teardown of an object that is being destroyed.
    blockSignals(true);
    shutdown();
}

QString NTRIPManager::securityWarning() const
{
    return _connectionStatus == ConnectionStatus::Connected ? _running.connection.credentialsInClearWarning()
                                                            : QString();
}

QString NTRIPManager::statusMessage() const
{
    // No state has been entered before the first connection.
    const QString message = _statusMessage.isEmpty() ? _defaultMessageFor(_connectionStatus) : _statusMessage;
    if (!_savedCaster) {
        return message;
    }
    // One arg() pass: a second would take a "%1" in the message or the name for a place marker.
    //: %1 is the connection status, such as "Connected"; %2 the name of the saved caster in use instead of the
    //: caster that is entered
    return tr("%1 (saved caster %2)").arg(message, _savedCaster->name);
}

void NTRIPManager::init()
{
    if (_initialized || _shutdown) {
        qCWarning(NTRIPManagerLog) << "Already initialized or shut down";
        return;
    }
    _initialized = true;
    _ggaReporter.configure(_configuration.gga);
    _applyConfiguration();
}

void NTRIPManager::setConfiguration(const Configuration& configuration)
{
    if (_shutdown || configuration == _configuration) {
        return;
    }
    const bool ggaChanged = configuration.gga != _configuration.gga;
    const bool streamChanged =
        configuration.enabled != _configuration.enabled || configuration.connection != _configuration.connection;
    const bool castersChanged = configuration.version1Casters != _configuration.version1Casters;
    const bool failoverChanged =
        configuration.failover != _configuration.failover || configuration.savedCasters != _configuration.savedCasters;
    const bool vehicleGateChanged = configuration.requireVehicle != _configuration.requireVehicle;
    const bool dataCapChanged = configuration.dataCapMB != _configuration.dataCapMB;
    const bool couldConnect = canConnect();
    _configuration = configuration;
    // Opting out forgets the pinned certificate, so opting in again trusts the caster's current certificate.
    QString& pin = _configuration.connection.pinnedCertificate;
    const bool pinForgotten = !_configuration.connection.allowSelfSignedCerts && !pin.isEmpty();
    if (pinForgotten) {
        pin.clear();
    }
    if (_initialized && ggaChanged) {
        _ggaReporter.configure(configuration.gga);
    }
    // The saved caster in use may have changed or gone, so the connection starts again from the entered one.
    if (_initialized && (streamChanged || vehicleGateChanged || dataCapChanged || (failoverChanged && _savedCaster))) {
        _settingsDebounceTask.schedule(SETTINGS_DEBOUNCE, [this]() { _applyConfiguration(); });
    }
    if (pinForgotten) {
        emit certificatePinChanged(QString());
    }
    if (canConnect() != couldConnect) {
        emit canConnectChanged();
    }
    if (castersChanged) {
        _casterVersions->restore(_configuration.version1Casters);
    }
}

void NTRIPManager::setGgaPositionProvider(NTRIPGgaReporter::PositionSource source,
                                          NTRIPGgaReporter::PositionProvider provider)
{
    if (_initialized || _transport) {
        qCWarning(NTRIPManagerLog) << "Inject GGA position providers before initializing NTRIP";
        return;
    }
    _ggaReporter.setPositionProvider(source, std::move(provider));
}

// -----------------------------------------------------------------------------
// Public control surface
// -----------------------------------------------------------------------------

void NTRIPManager::connectCaster()
{
    if (_shutdown) {
        return;
    }
    // After an error the setting is still on, and storing it again would start nothing.
    if (_connectionStatus == ConnectionStatus::Error) {
        _configuration.enabled = true;
        // The user's Connect or Retry is a new run of the connection.
        _linkBytes = 0;
        _start();
    }
    emit enabledRequested(true);
}

void NTRIPManager::disconnectCaster()
{
    if (!_shutdown) {
        emit enabledRequested(false);
    }
}

void NTRIPManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    _sourceTableController.cancel();
    _stop();
}

void NTRIPManager::fetchMountpoints(const QGeoCoordinate& sortCoordinate)
{
    if (_shutdown) {
        return;
    }
    _sourceTableController.fetch(_configuration.connection, sortCoordinate);
}

// -----------------------------------------------------------------------------
// State machine
// -----------------------------------------------------------------------------

void NTRIPManager::_start()
{
    if (_shutdown || _connectionStatus == ConnectionStatus::Connecting ||
        _connectionStatus == ConnectionStatus::Connected) {
        return;
    }
    _settingsDebounceTask.cancel();
    _cancelReconnect();
    _reconnectBackoff.reset();
    _useEnteredCaster();
    _dispatch(_shouldWaitForVehicle() ? Event::NoVehicle : Event::StartRequested);
    _publish();
}

void NTRIPManager::_stop()
{
    _settingsDebounceTask.cancel();
    _cancelReconnect();
    _dispatch(Event::StopRequested);
    _publish();
}

void NTRIPManager::_publish()
{
    // Each value is recorded before its signal, so an observer that re-enters the manager cannot cause a repeat.
    if (std::exchange(_notified.status, _connectionStatus) != _connectionStatus) {
        emit connectionStatusChanged();
    }
    const QString savedCaster = _savedCaster ? _savedCaster->name : QString();
    const bool messageChanged = std::exchange(_notified.message, _statusMessage) != _statusMessage;
    if (std::exchange(_notified.savedCaster, savedCaster) != savedCaster || messageChanged) {
        emit statusMessageChanged();
    }
    if (const QString warning = securityWarning(); std::exchange(_notified.securityWarning, warning) != warning) {
        emit securityWarningChanged();
    }
    if (const QString mountpoint = streamMountpoint();
        std::exchange(_notified.streamMountpoint, mountpoint) != mountpoint) {
        emit streamMountpointChanged();
    }
}

bool NTRIPManager::_dispatch(Event ev, const QString& detail, std::chrono::milliseconds retryAfter)
{
    using CS = ConnectionStatus;

    struct Transition
    {
        CS from;
        Event event;
        CS to;
    };

    // Every legal transition; events not listed for the current state are ignored. Entry actions are in
    // _onEnterState().
    static constexpr Transition kTransitions[] = {
        // Disconnected: user or settings can kick us into Connecting.
        {CS::Disconnected, Event::StartRequested, CS::Connecting},

        // Connecting: transport handshake outcome.
        {CS::Connecting, Event::TransportConnected, CS::Connected},
        {CS::Connecting, Event::TransportError, CS::Reconnecting},
        {CS::Connecting, Event::TransportFatalError, CS::Error},
        {CS::Connecting, Event::ConfigInvalid, CS::Error},
        {CS::Connecting, Event::StopRequested, CS::Disconnected},

        // Connected: streaming; may lose link or be stopped.
        {CS::Connected, Event::TransportError, CS::Reconnecting},
        {CS::Connected, Event::TransportFatalError, CS::Error},
        {CS::Connected, Event::StopRequested, CS::Disconnected},
        {CS::Connected, Event::HotReconfigure, CS::Connecting},

        // Self-transition: a transport-affecting setting changed mid-handshake. Re-runs the Connecting entry action
        // (teardown + restart) without a state change so the in-flight attempt picks up the new config.
        {CS::Connecting, Event::HotReconfigure, CS::Connecting},

        // Reconnecting: backoff timer owns when we try again, but user/settings can short-circuit it.
        {CS::Reconnecting, Event::ReconnectDue, CS::Connecting},
        {CS::Reconnecting, Event::ReconnectGaveUp, CS::Error},
        {CS::Reconnecting, Event::StopRequested, CS::Disconnected},
        {CS::Reconnecting, Event::StartRequested, CS::Connecting},

        // Error: user-visible fault. User retry or stop both resolve it.
        {CS::Error, Event::StartRequested, CS::Connecting},
        {CS::Error, Event::StopRequested, CS::Disconnected},

        // No vehicle: the connection stays on and waits in Reconnecting, from wherever it was.
        {CS::Disconnected, Event::NoVehicle, CS::Reconnecting},
        {CS::Error, Event::NoVehicle, CS::Reconnecting},
        {CS::Connecting, Event::NoVehicle, CS::Reconnecting},
        {CS::Connected, Event::NoVehicle, CS::Reconnecting},
        {CS::Reconnecting, Event::NoVehicle, CS::Reconnecting},

        // The data limit ends the connection wherever it is reached or lowered.
        {CS::Connecting, Event::DataCapReached, CS::Error},
        {CS::Connected, Event::DataCapReached, CS::Error},
        {CS::Reconnecting, Event::DataCapReached, CS::Error},
    };

    for (const auto& row : kTransitions) {
        if (row.from == _connectionStatus && row.event == ev) {
            _enterState(row.to, detail, retryAfter);
            return true;
        }
    }
    qCDebug(NTRIPManagerLog) << "NTRIP event" << ev << "ignored in state" << _connectionStatus;
    return false;
}

void NTRIPManager::_enterState(ConnectionStatus to, const QString& detail, std::chrono::milliseconds retryAfter)
{
    const ConnectionStatus from = _connectionStatus;
    const QString msg = detail.isEmpty() ? _defaultMessageFor(to) : detail;
    if (from != to) {
        _waitingForNetwork = false;
        qCDebug(NTRIPManagerLog) << "NTRIP state" << from << "→" << to << msg;
    }

    // Commit state + message before running entry actions so that a recursive
    // _dispatch() from inside an entry action (e.g. _openSession → ConfigInvalid)
    // observes the already-committed state, not the stale caller value.
    _connectionStatus = to;
    _statusMessage = msg;

    // Entry action runs on every dispatched transition, including self-transitions
    // (e.g. Connecting→Connecting on HotReconfigure).
    _onEnterState(to, retryAfter);
}

QString NTRIPManager::_defaultMessageFor(ConnectionStatus state)
{
    switch (state) {
        case ConnectionStatus::Disconnected:
            return tr("Disconnected");
        case ConnectionStatus::Connecting:
            return tr("Connecting...");
        case ConnectionStatus::Connected:
            return tr("Connected");
        case ConnectionStatus::Reconnecting:
            return tr("Reconnecting...");
        case ConnectionStatus::Error:
            break;
    }
    return tr("Error");
}

QString NTRIPManager::_connectedMessage() const
{
    if (_stats.messagesReceived() > 0) {
        return _defaultMessageFor(ConnectionStatus::Connected);
    }
    return _ggaReporter.currentSource().isEmpty() ? tr("Connected, waiting for a position to report")
                                                  : tr("Connected, waiting for corrections");
}

void NTRIPManager::_refreshConnectedMessage()
{
    if (_connectionStatus == ConnectionStatus::Connected) {
        _statusMessage = _connectedMessage();
        _publish();
    }
}

void NTRIPManager::_onEnterState(ConnectionStatus to, std::chrono::milliseconds retryAfter)
{
    switch (to) {
        case ConnectionStatus::Disconnected:
        case ConnectionStatus::Error:
            _cancelReconnect();
            _vehicleHoldTask.cancel();
            _stopStreaming();
            _running = {};
            _useEnteredCaster();
            if (to == ConnectionStatus::Disconnected) {
                _correctionSource.reset();
                // Turned off, this run of the connection is over.
                _linkBytes = 0;
            }
            break;

        case ConnectionStatus::Connecting:
            _cancelReconnect();
            _openSession();
            break;

        case ConnectionStatus::Connected:
            _stats.start();
            _statusMessage = _connectedMessage();
            // Last, because a position reported at once publishes this state.
            _ggaReporter.start(_transport);
            break;

        case ConnectionStatus::Reconnecting:
            _stopStreaming();
            _scheduleReconnect(retryAfter);
            break;
    }
}

// -----------------------------------------------------------------------------
// Entry-action helpers
// -----------------------------------------------------------------------------

void NTRIPManager::_stopStreaming()
{
    const QPointer<NTRIPTransport> transport = std::exchange(_transport, nullptr);
    if (transport) {
        transport->disconnect(this);
        transport->stop();
        transport->deleteLater();
    }
    _ggaReporter.stop();
    _stats.stop();
}

void NTRIPManager::_scheduleReconnect(std::chrono::milliseconds retryAfter)
{
    if (_shouldWaitForVehicle()) {
        _waitForVehicle();
        return;
    }
    if (_shouldWaitForNetwork()) {
        _waitForNetwork();
        return;
    }

    if (_reconnectExhausted()) {
        _dispatch(Event::ReconnectGaveUp,
                  tr("Gave up after %1 reconnect attempts: %2").arg(MAX_RECONNECT_ATTEMPTS).arg(_lastFailureDetail));
        return;
    }
    const ExponentialBackoff beforeAttempt = _reconnectBackoff;
    const auto backoff = _reconnectBackoff.next(retryAfter);
    _reconnectTask.schedule(backoff, [this, beforeAttempt]() {
        if (_shouldWaitForVehicle()) {
            _waitForVehicle();
        } else if (_shouldWaitForNetwork()) {
            // The attempt never ran, so the next one keeps its delay.
            _reconnectBackoff = beforeAttempt;
            _waitForNetwork();
        } else {
            _dispatch(Event::ReconnectDue);
        }
        _publish();
    });
}

void NTRIPManager::_cancelReconnect()
{
    _reconnectTask.cancel();
    _waitingForNetwork = false;
    _waitingForVehicle = false;
}

bool NTRIPManager::_dataCapReached() const
{
    const quint64 megabyte = 1024 * 1024;
    return _configuration.dataCapMB > 0 && _linkBytes >= _configuration.dataCapMB * megabyte;
}

QString NTRIPManager::_dataCapMessage() const
{
    //: %1 is the limit, a number of megabytes
    return tr("Stopped at the data limit of %1 MB").arg(_configuration.dataCapMB);
}

void NTRIPManager::_onDataReceived(qint64 bytes)
{
    if (bytes <= 0) {
        return;
    }
    _linkBytes += static_cast<quint64>(bytes);
    if (_dataCapReached()) {
        qCWarning(NTRIPManagerLog) << "NTRIP stopped at the data limit of" << _configuration.dataCapMB << "MB";
        _dispatch(Event::DataCapReached, _dataCapMessage());
        _publish();
    }
}

bool NTRIPManager::_shouldWaitForVehicle() const
{
    return _configuration.requireVehicle && _vehicles && !_vehicles->present();
}

void NTRIPManager::_waitForVehicle()
{
    // A reconnect that was due is not: the next vehicle starts the connection anew, from the entered caster.
    _reconnectTask.cancel();
    _waitingForNetwork = false;
    _waitingForVehicle = true;
    _reconnectBackoff.reset();
    _useEnteredCaster();
    _statusMessage = tr("Waiting for a vehicle");
}

void NTRIPManager::_onVehiclePresenceChanged(bool present)
{
    if (_shutdown) {
        return;
    }
    if (present) {
        _vehicleHoldTask.cancel();
        if (_connectionStatus == ConnectionStatus::Reconnecting && _waitingForVehicle) {
            _waitingForVehicle = false;
            _dispatch(Event::ReconnectDue);
            _publish();
        }
        return;
    }
    if (!_configuration.requireVehicle || _connectionStatus == ConnectionStatus::Disconnected ||
        _connectionStatus == ConnectionStatus::Error) {
        return;
    }
    // A link that drops and returns within the hold leaves the stream as it is.
    _vehicleHoldTask.schedule(VEHICLE_HOLD, [this]() {
        if (_shouldWaitForVehicle() && !_waitingForVehicle) {
            _dispatch(Event::NoVehicle);
            _publish();
        }
    });
}

bool NTRIPManager::_shouldWaitForNetwork() const
{
    return _networkMonitor && !_networkMonitor->available() && !_casterIsLoopback();
}

bool NTRIPManager::_casterIsLoopback() const
{
    const QString host = _casterConnection().host.trimmed();
    if (host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    QHostAddress address;
    return address.setAddress(host) && address.isLoopback();
}

void NTRIPManager::_waitForNetwork()
{
    _waitingForNetwork = true;
    _statusMessage = tr("Waiting for network");
}

void NTRIPManager::_openSession()
{
    _stopStreaming();

    Configuration config = _configuration;
    config.connection = _casterConnection();
    const auto& connection = config.connection;

    _correctionSource.reset();
    if (_correctionManager) {
        _correctionSource =
            _correctionManager->openSource(GPSCorrectionSettings::Ntrip, correctionSourceInstance(connection));
    }

    const QString err = connection.streamValidationError();
    if (!err.isEmpty()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid:" << err << "host=" << connection.host
                                   << " port=" << connection.port;
        _dispatch(Event::ConfigInvalid, err);
        return;
    }

    qCDebug(NTRIPManagerLog) << "Starting NTRIP transport: host=" << connection.host << " port=" << connection.port
                             << " mount=" << connection.mountpoint;

    // Replace the generic "Connecting..." with a host-specific message.
    // One arg() pass: a second would take a "%1" in the host for a place marker.
    _statusMessage = tr("Connecting to %1:%2...").arg(connection.host, QString::number(connection.port));

    _running = config;
    _stats.reset();

    NTRIPTransport* transport = _transportFactory ? _transportFactory(_running, this) : nullptr;
    if (!transport) {
        transport = new NTRIPHttpTransport(connection, this, _scheduler, _casterVersions);
    }
    _transport = transport;

    // Qt still delivers events queued before a transport was retired; they belong to the ended stream.
    const auto isCurrent = [this, guard = QPointer<NTRIPTransport>(transport)]() {
        return guard && guard == _transport;
    };

    // Queued: error handling stops and deletes the transport, which must not happen inside the transport's (or its
    // socket's) own signal emission.
    connect(
        transport, &NTRIPTransport::error, this,
        [this, isCurrent](const NTRIPFailure& failure) {
            if (isCurrent()) {
                _onTransportError(failure);
            }
        },
        Qt::QueuedConnection);

    // Direct, so the handshake state precedes any error or frame queued after it. _stopStreaming() disconnects a
    // retired transport before stopping it, so direct signals only ever come from the current one.
    connect(transport, &NTRIPTransport::connected, this, &NTRIPManager::_onTransportConnected);

    // Queued, like errors, so routing and statistics run outside the transport's parse loop and keep their order
    // relative to a queued failure.
    connect(
        transport, &NTRIPTransport::correctionFrameReceived, this,
        [this, isCurrent](const RTCMDecodedFrame& frame) {
            if (isCurrent()) {
                _onCorrectionFrame(frame);
            }
        },
        Qt::QueuedConnection);

    connect(transport, &NTRIPTransport::certificatePinned, this, &NTRIPManager::_onCertificatePinned);

    // Queued, like errors: reaching the data limit stops and deletes the transport.
    connect(
        transport, &NTRIPTransport::dataReceived, this,
        [this, isCurrent](qint64 bytes) {
            if (isCurrent()) {
                _onDataReceived(bytes);
            }
        },
        Qt::QueuedConnection);

    transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP transport started";
}

// -----------------------------------------------------------------------------
// Signal handlers
// -----------------------------------------------------------------------------

void NTRIPManager::_onGgaSourceChanged()
{
    emit ggaSourceChanged();
    _refreshConnectedMessage();
}

void NTRIPManager::_onTransportConnected()
{
    _dispatch(Event::TransportConnected);
    _publish();
}

void NTRIPManager::_onTransportError(const NTRIPFailure& failure)
{
    const auto code = failure.code;
    const auto& detail = failure.detail;
    if (_connectionStatus != ConnectionStatus::Connecting && _connectionStatus != ConnectionStatus::Connected) {
        return;
    }
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << code << detail;
    _lastFailureDetail = detail;

    const bool retryable = isRetryable(code, _streamServed);
    // What is wrong with the entered caster is the user's to see; a saved caster that fails so is left out instead.
    if (!_isEnabled() || (!retryable && !_savedCaster)) {
        _dispatch(Event::TransportFatalError, detail);
        _publish();
        return;
    }
    bool nextCaster = false;
    if (!retryable) {
        qCWarning(NTRIPManagerLog) << "Leaving out saved caster" << _savedCaster->name << "until the next start";
        _castersLeftOut.append(_savedCaster->name);
        nextCaster = _goToNextCaster();
    } else if (++_casterFailures >= FAILOVER_ATTEMPTS && _configuration.failover) {
        nextCaster = _goToNextCaster();
    }
    // A caster's Retry-After says nothing of another caster.
    const auto retryAfter = nextCaster ? std::chrono::milliseconds{} : failure.retryAfter;
    const auto backoffMs = _reconnectBackoff.peek(retryAfter).count();
    qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << backoffMs << "ms (attempt"
                             << (_reconnectBackoff.attempts() + 1) << ")";
    _dispatch(Event::TransportError, tr("Reconnecting in %1s: %2").arg(backoffMs / 1000).arg(detail), retryAfter);
    _publish();
}

const NTRIPConnectionConfig& NTRIPManager::_casterConnection() const
{
    return _savedCaster ? _savedCaster->connection : _configuration.connection;
}

bool NTRIPManager::_usableForFailover(const SavedCaster& caster) const
{
    NTRIPConnectionConfig saved = caster.connection;
    NTRIPConnectionConfig entered = _configuration.connection;
    // Its certificate could be any: none was trusted for it to compare with.
    const bool unchecked = saved.useTls && saved.allowSelfSignedCerts && saved.pinnedCertificate.isEmpty();
    saved.pinnedCertificate.clear();
    entered.pinnedCertificate.clear();
    return saved != entered && !unchecked && saved.streamValidationError().isEmpty() &&
           !_castersLeftOut.contains(caster.name);
}

bool NTRIPManager::_goToNextCaster()
{
    const QList<SavedCaster>& casters = _configuration.savedCasters;
    // After the saved caster in use, or from the first one after the entered caster.
    qsizetype next = 0;
    if (_savedCaster) {
        const auto inUse = std::ranges::find(casters, _savedCaster->name, &SavedCaster::name);
        next = inUse == casters.cend() ? casters.size() : std::distance(casters.cbegin(), inUse) + 1;
    }
    for (; next < casters.size(); ++next) {
        if (_usableForFailover(casters.at(next))) {
            break;
        }
    }
    // The entered caster follows the last saved one.
    if (next >= casters.size() && !_savedCaster) {
        return false;
    }
    _savedCaster = next < casters.size() ? std::optional(casters.at(next)) : std::nullopt;
    _casterFailures = 0;
    _streamServed = false;
    return true;
}

void NTRIPManager::_useEnteredCaster()
{
    _savedCaster.reset();
    _castersLeftOut.clear();
    _casterFailures = 0;
    _streamServed = false;
}

void NTRIPManager::_onCertificatePinned(const QString& pin)
{
    // The stored pin is the entered caster's. A saved caster is used only with its own, so it reports none.
    if (_savedCaster) {
        return;
    }
    // The running transport already trusts the certificate, so storing its pin must not reconnect.
    _running.connection.pinnedCertificate = pin;
    _setPinnedCertificate(pin);
}

void NTRIPManager::_setPinnedCertificate(const QString& pin)
{
    QString& pinned = _configuration.connection.pinnedCertificate;
    if (pinned == pin) {
        return;
    }
    pinned = pin;
    emit certificatePinChanged(pin);
}

void NTRIPManager::_onCasterVersionsChanged()
{
    // Also what restore() kept of a stored list that it did not write, so that the settings hold what is remembered.
    const QString casters = _casterVersions->saved();
    if (std::exchange(_configuration.version1Casters, casters) != casters) {
        emit version1CastersChanged(casters);
    }
}

void NTRIPManager::_onCorrectionFrame(const RTCMDecodedFrame& frame)
{
    if (_correctionSource) {
        _correctionSource->submit(frame.data, frame.receivedAtMs);
    }
    const bool first = _stats.messagesReceived() == 0;
    _stats.recordMessage(frame.data.size(), frame.receivedAtMs);
    if (first) {
        _refreshConnectedMessage();
    }

    // Only corrections show the caster serves this stream; one that accepts and then drops must keep backing off.
    _reconnectBackoff.reset();
    _casterFailures = 0;
    _streamServed = true;
}

void NTRIPManager::_applyConfiguration()
{
    if (_shutdown) {
        return;
    }
    const Configuration& newConfig = _configuration;
    const bool isActive =
        (_connectionStatus == ConnectionStatus::Connecting || _connectionStatus == ConnectionStatus::Connected);

    if (!_isEnabled()) {
        _reconnectBackoff.reset();
        _stop();
    } else if (_dataCapReached()) {
        // The limit was set below what has arrived. A connection that ended at it stays ended, whatever else is
        // changed: only Connect, Retry or a limit that leaves room starts it.
        if (_connectionStatus != ConnectionStatus::Error) {
            qCWarning(NTRIPManagerLog) << "NTRIP stopped at the data limit of" << _configuration.dataCapMB << "MB";
            _dispatch(Event::DataCapReached, _dataCapMessage());
        }
    } else if (!isActive) {
        // Disconnected / Error / Reconnecting: start fresh. The connecting
        // path re-reads settings, so the new values take effect there.
        _start();
    } else if (_shouldWaitForVehicle()) {
        // The option was turned on with no vehicle connected.
        _dispatch(Event::NoVehicle);
    } else if (_savedCaster || newConfig.connection != _running.connection) {
        qCDebug(NTRIPManagerLog) << "NTRIP transport-affecting setting changed, reconnecting";
        _useEnteredCaster();
        _dispatch(Event::HotReconfigure);
    }
    _publish();
}
