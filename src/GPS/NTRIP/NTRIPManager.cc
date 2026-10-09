#include "NTRIPManager.h"

#include <chrono>
#include <utility>

#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>

#include "GPSCorrectionManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkAvailabilityMonitor.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIPManager")

namespace {

bool isRetryable(NTRIPError error)
{
    switch (error) {
        case NTRIPError::AuthFailed:
        case NTRIPError::InvalidConfig:
        case NTRIPError::RequestRejected:
        // TLS failures recur the same way until the user changes the TLS settings.
        case NTRIPError::SslError:
            return false;
        default:
            return true;
    }
}

QString correctionSourceInstance(const NTRIPConnectionConfig& connection)
{
    QUrl endpoint = connection.url();
    endpoint.setScheme(connection.useTls ? QStringLiteral("ntrips") : QStringLiteral("ntrip"));
    return endpoint.toString(QUrl::FullyEncoded);
}

}  // namespace

QVector<int> NTRIPRTCMFilterConfig::messageIds() const
{
    QVector<int> ids;
    for (const auto& token : whitelist.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int id = token.trimmed().toInt(&ok);
        if (ok && id > 0) {
            ids.append(id);
        }
    }
    return ids;
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

NTRIPManager::NTRIPManager(QObject* parent, RuntimeScheduler* scheduler, const Dependencies& dependencies)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _settingsDebounceTask(_scheduler, this)
    , _reconnectTask(_scheduler, this)
    , _ggaReporter(this, _scheduler)
    // The display agrees with routing, which stops using a stream after the same age.
    , _stats(GPSCorrectionSelector::FRESHNESS_TIMEOUT, this, _scheduler)
    , _correctionManager(dependencies.corrections)
    , _networkMonitor(dependencies.network)
    , _sourceTableController(this, _scheduler)
{
    qCDebug(NTRIPManagerLog) << this;

    connect(&_ggaReporter, &NTRIPGgaReporter::sourceChanged, this, &NTRIPManager::ggaSourceChanged);

    connect(&_sourceTableController, &NTRIPSourceTableController::certificatePinned, this,
            &NTRIPManager::_setPinnedCertificate);

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

QString NTRIPManager::connectionStatusText() const
{
    switch (_connectionStatus) {
        case ConnectionStatus::Disconnected:
            return tr("Disconnected");
        case ConnectionStatus::Connecting:
            return tr("Connecting");
        case ConnectionStatus::Connected:
            return tr("Connected");
        case ConnectionStatus::Reconnecting:
            return tr("Reconnecting");
        case ConnectionStatus::Error:
            break;
    }
    return tr("Error");
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
    const bool streamChanged = configuration.enabled != _configuration.enabled ||
                               configuration.connection != _configuration.connection ||
                               configuration.filter != _configuration.filter;
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
    if (_initialized && streamChanged) {
        _settingsDebounceTask.schedule(SETTINGS_DEBOUNCE, [this]() { _applyConfiguration(); });
    }
    if (pinForgotten) {
        emit certificatePinChanged(QString());
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

void NTRIPManager::retryNTRIP()
{
    if (_shutdown || _connectionStatus != ConnectionStatus::Error) {
        return;
    }
    _configuration.enabled = true;
    _start();
    emit enableRequested();
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
    _dispatch(Event::StartRequested);
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
    if (std::exchange(_notified.message, _statusMessage) != _statusMessage) {
        emit statusMessageChanged();
    }
    if (const QString warning = securityWarning(); std::exchange(_notified.securityWarning, warning) != warning) {
        emit securityWarningChanged();
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
            return {};  // Error always carries a detail from the caller.
    }
    return {};
}

void NTRIPManager::_onEnterState(ConnectionStatus to, std::chrono::milliseconds retryAfter)
{
    switch (to) {
        case ConnectionStatus::Disconnected:
        case ConnectionStatus::Error:
            _cancelReconnect();
            _stopStreaming();
            _running = {};
            if (to == ConnectionStatus::Disconnected) {
                _correctionSource.reset();
            }
            break;

        case ConnectionStatus::Connecting:
            _cancelReconnect();
            _openSession();
            break;

        case ConnectionStatus::Connected:
            _ggaReporter.start(_transport);
            _stats.start();
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
        if (_shouldWaitForNetwork()) {
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
}

bool NTRIPManager::_shouldWaitForNetwork() const
{
    return _networkMonitor && !_networkMonitor->available() && !_casterIsLoopback();
}

bool NTRIPManager::_casterIsLoopback() const
{
    const QString host = _configuration.connection.host.trimmed();
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

    const Configuration config = _configuration;
    const auto& connection = config.connection;

    _correctionSource.reset();
    if (_correctionManager) {
        _correctionSource =
            _correctionManager->openSource(GPSCorrectionSettings::Ntrip, correctionSourceInstance(connection));
    }

    if (const QString err = connection.streamValidationError(); !err.isEmpty()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid:" << err << "host=" << connection.host
                                   << " port=" << connection.port;
        _dispatch(Event::ConfigInvalid, err);
        return;
    }

    qCDebug(NTRIPManagerLog) << "Starting NTRIP transport: host=" << connection.host << " port=" << connection.port
                             << " mount=" << connection.mountpoint;

    // Replace the generic "Connecting..." with a host-specific message.
    _statusMessage = tr("Connecting to %1:%2...").arg(connection.host).arg(connection.port);

    _running = config;
    _stats.reset();

    NTRIPTransport* transport = _transportFactory ? _transportFactory(_configuration, this) : nullptr;
    if (!transport) {
        transport = new NTRIPHttpTransport(connection, config.filter.messageIds(), this, _scheduler);
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

    transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP transport started";
}

// -----------------------------------------------------------------------------
// Signal handlers
// -----------------------------------------------------------------------------

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

    if (_isEnabled() && isRetryable(code)) {
        const auto backoffMs = _reconnectBackoff.peek(failure.retryAfter).count();
        qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << backoffMs << "ms (attempt"
                                 << (_reconnectBackoff.attempts() + 1) << ")";
        _dispatch(Event::TransportError, tr("Reconnecting in %1s: %2").arg(backoffMs / 1000).arg(detail),
                  failure.retryAfter);
    } else {
        _dispatch(Event::TransportFatalError, detail);
    }
    _publish();
}

void NTRIPManager::_onCertificatePinned(const QString& pin)
{
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

void NTRIPManager::_onCorrectionFrame(const RTCMDecodedFrame& frame)
{
    if (_correctionSource) {
        _correctionSource->submit(frame.data, frame.receivedAtMs);
    }
    _stats.recordMessage(frame.data.size(), frame.messageId, frame.receivedAtMs);

    // Only corrections show the caster serves this stream; one that accepts and then drops must keep backing off.
    _reconnectBackoff.reset();
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
    } else if (!isActive) {
        // Disconnected / Error / Reconnecting: start fresh. The connecting
        // path re-reads settings, so the new values take effect there.
        _start();
    } else if (newConfig.connection != _running.connection) {
        qCDebug(NTRIPManagerLog) << "NTRIP transport-affecting setting changed, reconnecting";
        _dispatch(Event::HotReconfigure);
    } else {
        if (newConfig.filter != _running.filter && _transport) {
            qCDebug(NTRIPManagerLog) << "NTRIP RTCM whitelist changed, applying to live parser";
            _transport->setRtcmWhitelist(newConfig.filter.messageIds());
        }
        _running = newConfig;
    }
    _publish();
}
