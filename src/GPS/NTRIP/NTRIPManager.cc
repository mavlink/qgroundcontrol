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
            if (available && !_shutdown) {
                _gateOpened(Wait::Network);
            }
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

NTRIPManager::ConnectionStatus NTRIPManager::connectionStatus() const
{
    using Phase = GPSConnectionSupervisor::Phase;
    switch (_supervisor.phase()) {
        case Phase::Off:
            break;
        case Phase::Waiting:
            return ConnectionStatus::Reconnecting;
        case Phase::Opening:
            return ConnectionStatus::Connecting;
        case Phase::Up:
            return ConnectionStatus::Connected;
        case Phase::Stopped:
            return ConnectionStatus::Error;
    }
    return ConnectionStatus::Disconnected;
}

QString NTRIPManager::securityWarning() const
{
    return connectionStatus() == ConnectionStatus::Connected ? _running.connection.credentialsInClearWarning()
                                                             : QString();
}

QString NTRIPManager::statusMessage() const
{
    const QString message = _stateMessage();
    if (!_savedCaster) {
        return message;
    }
    // One arg() pass: a second would take a "%1" in the message or the name for a place marker.
    //: %1 is the connection status, such as "Connected"; %2 the name of the saved caster in use instead of the
    //: caster that is entered
    return tr("%1 (saved caster %2)").arg(message, _savedCaster->name);
}

QString NTRIPManager::_stateMessage() const
{
    switch (connectionStatus()) {
        case ConnectionStatus::Disconnected:
            break;
        case ConnectionStatus::Connecting:
            //: %1 is the caster's host and port, such as caster.example.com:2101
            return tr("Connecting to %1...").arg(_running.connection.endpoint());
        case ConnectionStatus::Connected:
            if (_stats.messagesReceived() > 0) {
                return tr("Connected");
            }
            // A network RTK (VRS) mountpoint sends nothing until it is told a position, and may then take a minute
            // or more over its first corrections, so until corrections arrive the message says which of the two the
            // stream waits for.
            return _ggaReporter.currentSource().isEmpty() ? tr("Connected, waiting for a position to report")
                                                          : tr("Connected, waiting for corrections");
        case ConnectionStatus::Reconnecting:
            switch (_waitingFor) {
                case Wait::Vehicle:
                    return tr("Waiting for a vehicle");
                case Wait::Network:
                    return tr("Waiting for network");
                case Wait::Delay:
                    break;
            }
            return tr("Reconnecting in %1s: %2")
                .arg(_supervisor.retryDelay().value_or(GPSConnectionSupervisor::Delay::zero()).count() / 1000)
                .arg(_lastFailureDetail);
        case ConnectionStatus::Error:
            switch (_stopReason) {
                case StopReason::GaveUp:
                    return tr("Gave up after %1 reconnect attempts: %2")
                        .arg(MAX_RECONNECT_ATTEMPTS)
                        .arg(_lastFailureDetail);
                case StopReason::DataLimit:
                    //: %1 is the limit, a number of megabytes
                    return tr("Stopped at the data limit of %1 MB").arg(_configuration.dataCapMB);
                case StopReason::Failure:
                    break;
            }
            // A failure that gives no reason still says that the connection ended.
            return _lastFailureDetail.isEmpty() ? tr("Error") : _lastFailureDetail;
    }
    return tr("Disconnected");
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
    if (connectionStatus() == ConnectionStatus::Error) {
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
// Connection
// -----------------------------------------------------------------------------

void NTRIPManager::_start()
{
    if (_shutdown || connectionStatus() == ConnectionStatus::Connecting ||
        connectionStatus() == ConnectionStatus::Connected) {
        return;
    }
    _settingsDebounceTask.cancel();
    _useEnteredCaster();
    _everStarted = true;
    if (_shouldWaitForVehicle()) {
        _waitForVehicle();
        _stopStreaming();
    } else {
        _supervisor.start();
        _openSession();
    }
    _publish();
}

void NTRIPManager::_stop()
{
    _settingsDebounceTask.cancel();
    if (connectionStatus() != ConnectionStatus::Disconnected) {
        _supervisor.stop();
        _end();
        _correctionSource.reset();
        // Turned off, this run of the connection is over.
        _linkBytes = 0;
    }
    _publish();
}

void NTRIPManager::_publish()
{
    // Each value is recorded before its signal, so an observer that re-enters the manager cannot cause a repeat.
    const ConnectionStatus status = connectionStatus();
    if (const ConnectionStatus previous = std::exchange(_notified.status, status); previous != status) {
        qCDebug(NTRIPManagerLog) << "NTRIP state" << previous << "→" << status;
        emit connectionStatusChanged();
    }
    const QString savedCaster = _savedCaster ? _savedCaster->name : QString();
    // Until the connection is first started its message was announced by no signal, and is no change to announce.
    const QString message = _everStarted ? _stateMessage() : QString();
    const bool messageChanged = std::exchange(_notified.message, message) != message;
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

void NTRIPManager::_halt(StopReason reason)
{
    _supervisor.halt();
    _stopReason = reason;
    _end();
}

void NTRIPManager::_end()
{
    _reconnectTask.cancel();
    _vehicleHoldTask.cancel();
    _stopStreaming();
    _running = {};
    _useEnteredCaster();
}

void NTRIPManager::_refreshConnectedMessage()
{
    if (connectionStatus() == ConnectionStatus::Connected) {
        _publish();
    }
}

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

void NTRIPManager::_retry(std::chrono::milliseconds retryAfter)
{
    if (_shouldWaitForVehicle()) {
        _waitForVehicle();
        _stopStreaming();
        return;
    }
    const bool noNetwork = _shouldWaitForNetwork();
    _supervisor.failed(retryAfter, noNetwork);
    _waitingFor = noNetwork ? Wait::Network : Wait::Delay;
    _stopStreaming();
    if (connectionStatus() == ConnectionStatus::Error) {
        _halt(StopReason::GaveUp);
    } else if (const auto delay = _supervisor.retryDelay()) {
        qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << delay->count() << "ms (attempt"
                                 << (_supervisor.attempts() + 1) << ")";
        _reconnectTask.schedule(*delay, [this]() { _onRetryDue(); });
    }
}

void NTRIPManager::_onRetryDue()
{
    if (_shouldWaitForVehicle()) {
        _waitForVehicle();
    } else {
        const bool noNetwork = _shouldWaitForNetwork();
        _supervisor.delayEnded(noNetwork);
        if (noNetwork) {
            _waitingFor = Wait::Network;
        } else {
            _openSession();
        }
    }
    _publish();
}

void NTRIPManager::_gateOpened(Wait gate)
{
    if (!_waitsFor(gate)) {
        return;
    }
    _supervisor.gateOpened();
    _openSession();
    _publish();
}

bool NTRIPManager::_dataCapReached() const
{
    const quint64 megabyte = 1024 * 1024;
    return _configuration.dataCapMB > 0 && _linkBytes >= _configuration.dataCapMB * megabyte;
}

void NTRIPManager::_stopAtDataCap()
{
    qCWarning(NTRIPManagerLog) << "NTRIP stopped at the data limit of" << _configuration.dataCapMB << "MB";
    _halt(StopReason::DataLimit);
}

void NTRIPManager::_onDataReceived(qint64 bytes)
{
    if (bytes <= 0) {
        return;
    }
    _linkBytes += static_cast<quint64>(bytes);
    if (_dataCapReached()) {
        _stopAtDataCap();
        _publish();
    }
}

bool NTRIPManager::_shouldWaitForVehicle() const
{
    return _configuration.requireVehicle && _vehicles && !_vehicles->present();
}

void NTRIPManager::_waitForVehicle()
{
    // A reconnect that was due is not. To the supervisor the vehicle is a closed gate, behind which the connection
    // starts over.
    _reconnectTask.cancel();
    const bool gateClosed = true;
    _supervisor.start(gateClosed);
    _useEnteredCaster();
    _waitingFor = Wait::Vehicle;
}

void NTRIPManager::_onVehiclePresenceChanged(bool present)
{
    if (_shutdown) {
        return;
    }
    if (present) {
        _vehicleHoldTask.cancel();
        _gateOpened(Wait::Vehicle);
        return;
    }
    if (!_configuration.requireVehicle || connectionStatus() == ConnectionStatus::Disconnected ||
        connectionStatus() == ConnectionStatus::Error) {
        return;
    }
    // A link that drops and returns within the hold leaves the stream as it is.
    _vehicleHoldTask.schedule(VEHICLE_HOLD, [this]() {
        if (_shouldWaitForVehicle() && !_waitsFor(Wait::Vehicle)) {
            _waitForVehicle();
            _stopStreaming();
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

void NTRIPManager::_openSession()
{
    _reconnectTask.cancel();
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
        _lastFailureDetail = err;
        _halt(StopReason::Failure);
        return;
    }

    qCDebug(NTRIPManagerLog) << "Starting NTRIP transport: host=" << connection.host << " port=" << connection.port
                             << " mount=" << connection.mountpoint;

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
    if (connectionStatus() == ConnectionStatus::Connecting) {
        _supervisor.connected();
        _stats.start();
        // Last, because a position reported at once publishes this state.
        _ggaReporter.start(_transport);
    }
    _publish();
}

void NTRIPManager::_onTransportError(const NTRIPFailure& failure)
{
    const auto code = failure.code;
    const auto& detail = failure.detail;
    if (connectionStatus() != ConnectionStatus::Connecting && connectionStatus() != ConnectionStatus::Connected) {
        return;
    }
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << code << detail;
    _lastFailureDetail = detail;

    const bool retryable = isRetryable(code, _streamServed);
    // What is wrong with the entered caster is the user's to see; a saved caster that fails so is left out instead.
    if (!_isEnabled() || (!retryable && !_savedCaster)) {
        _halt(StopReason::Failure);
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
    _retry(nextCaster ? std::chrono::milliseconds{} : failure.retryAfter);
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
    _supervisor.served();
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
        (connectionStatus() == ConnectionStatus::Connecting || connectionStatus() == ConnectionStatus::Connected);

    if (!_isEnabled()) {
        _stop();
    } else if (_dataCapReached()) {
        // The limit was set below what has arrived. A connection that ended at it stays ended, whatever else is
        // changed: only Connect, Retry or a limit that leaves room starts it.
        if (connectionStatus() != ConnectionStatus::Error) {
            _stopAtDataCap();
        }
    } else if (!isActive) {
        // Disconnected / Error / Reconnecting: start fresh. The connecting
        // path re-reads settings, so the new values take effect there.
        _start();
    } else if (_shouldWaitForVehicle()) {
        // The option was turned on with no vehicle connected.
        _waitForVehicle();
        _stopStreaming();
    } else if (_savedCaster || newConfig.connection != _running.connection) {
        qCDebug(NTRIPManagerLog) << "NTRIP transport-affecting setting changed, reconnecting";
        _useEnteredCaster();
        _supervisor.reopen();
        _openSession();
    }
    _publish();
}
