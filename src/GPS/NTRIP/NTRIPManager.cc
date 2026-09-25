#include "NTRIPManager.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QtMath>

#include "GPSCorrectionManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIP.NTRIPManager")

// -----------------------------------------------------------------------------
// Transition table
// -----------------------------------------------------------------------------
// Single source of truth for all legal state transitions. Events not listed for
// the current state are silently ignored (logged at debug). Entry actions are
// implemented in _onEnterState() below.
namespace {

using CS = NTRIPManager::ConnectionStatus;
using Ev = NTRIPManager::Event;

struct TransitionRow
{
    CS from;
    Ev event;
    CS to;
};

constexpr TransitionRow kTransitions[] = {
    // Disconnected: user or settings can kick us into Connecting.
    {CS::Disconnected, Ev::StartRequested, CS::Connecting},

    // Connecting: transport handshake outcome.
    {CS::Connecting, Ev::TransportConnected, CS::Connected},
    {CS::Connecting, Ev::RTCMBeforeConnected, CS::Connected},
    {CS::Connecting, Ev::TransportError, CS::Reconnecting},
    {CS::Connecting, Ev::TransportFatalError, CS::Error},
    {CS::Connecting, Ev::ConfigInvalid, CS::Error},
    {CS::Connecting, Ev::StopRequested, CS::Disconnected},

    // Connected: streaming; may lose link or be stopped.
    {CS::Connected, Ev::TransportError, CS::Reconnecting},
    {CS::Connected, Ev::TransportFatalError, CS::Error},
    {CS::Connected, Ev::StopRequested, CS::Disconnected},
    {CS::Connected, Ev::HotReconfigure, CS::Connecting},

    // Self-transition: a transport-affecting setting changed mid-handshake.
    // Re-runs the Connecting entry action (teardown + restart) without a state
    // change so the in-flight attempt picks up the new config.
    {CS::Connecting, Ev::HotReconfigure, CS::Connecting},

    // Reconnecting: backoff timer owns when we try again, but user/settings
    // can short-circuit it.
    {CS::Reconnecting, Ev::ReconnectDue, CS::Connecting},
    {CS::Reconnecting, Ev::ReconnectGaveUp, CS::Error},
    {CS::Reconnecting, Ev::StopRequested, CS::Disconnected},
    {CS::Reconnecting, Ev::StartRequested, CS::Connecting},

    // Error: user-visible fault. User retry or stop both resolve it.
    {CS::Error, Ev::StartRequested, CS::Connecting},
    {CS::Error, Ev::StopRequested, CS::Disconnected},
};

bool isRetryable(NTRIPError error)
{
    switch (error) {
        case NTRIPError::AuthFailed:
        case NTRIPError::InvalidConfig:
        // Certificate verification fails the same way until the user changes the TLS settings.
        case NTRIPError::SslError:
            return false;
        default:
            return true;
    }
}

}  // namespace

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

NTRIPManager::NTRIPManager(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _settingsDebounceTask(_scheduler, this)
    , _reconnectTask(_scheduler, this)
    , _ggaProvider(this, _scheduler)
    , _sourceTableController(this, _scheduler)
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager created";

    connect(&_ggaProvider, &NTRIPGgaProvider::sourceChanged, this,
            [this]() { _notifications.emitSignal(this, &NTRIPManager::ggaSourceChanged); });

    connect(&_sourceTableController, &NTRIPSourceTableController::mountpointSelected, this,
            &NTRIPManager::mountpointChosen);

    // DirectConnection: queued slot may not dispatch before destruction during quit.
    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::DirectConnection);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager destroyed";
    _notifications.close();
    shutdown();
}

void NTRIPManager::setCorrectionManager(GPSCorrectionManager* manager)
{
    if (_correctionManager == manager) {
        return;
    }
    if (_initialized || _transport) {
        qCWarning(NTRIPManagerLog) << "Inject the correction manager before initializing NTRIP";
        return;
    }
    _correctionRegistration.reset();
    _correctionManager = manager;
}

void NTRIPManager::init()
{
    if (_initialized || _shutdown) {
        qCWarning(NTRIPManagerLog) << "NTRIPManager::init() called more than once";
        return;
    }
    _initialized = true;
    _ggaProvider.configure(_configuration.gga);
    _applyConfiguration();
}

void NTRIPManager::setConfiguration(const Configuration& configuration)
{
    if (_shutdown || configuration == _configuration) {
        return;
    }
    const bool ggaChanged = configuration.gga != _configuration.gga;
    const bool streamChanged =
        configuration.enabled != _configuration.enabled || configuration.stream != _configuration.stream;
    _configuration = configuration;
    if (!_initialized) {
        return;
    }
    if (ggaChanged) {
        _ggaProvider.configure(configuration.gga);
    }
    if (streamChanged) {
        _settingsDebounceTask.schedule(kSettingsDebounceMs, [this]() { _applyConfiguration(); });
    }
}

void NTRIPManager::setGgaPositionProvider(NTRIPGgaProvider::PositionSource source,
                                          NTRIPGgaProvider::PositionProvider provider)
{
    if (_initialized || _transport) {
        qCWarning(NTRIPManagerLog) << "Inject GGA position providers before initializing NTRIP";
        return;
    }
    _ggaProvider.setPositionProvider(source, std::move(provider));
}

// -----------------------------------------------------------------------------
// Public control surface
// -----------------------------------------------------------------------------

void NTRIPManager::startNTRIP()
{
    if (_shutdown || _connectionStatus == ConnectionStatus::Connecting ||
        _connectionStatus == ConnectionStatus::Connected) {
        return;
    }
    _settingsDebounceTask.cancel();
    _cancelReconnect();
    _resetReconnectAttempts();
    _dispatch(Event::StartRequested);
}

void NTRIPManager::stopNTRIP()
{
    _settingsDebounceTask.cancel();
    _cancelReconnect();
    _dispatch(Event::StopRequested);
}

void NTRIPManager::retryNTRIP()
{
    if (_shutdown || _connectionStatus != ConnectionStatus::Error) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    const auto state = _stateRevision.current(this);
    _configuration.enabled = true;
    emit enableRequested();
    if (state.isCurrent()) {
        startNTRIP();
    }
}

void NTRIPManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    const GPSNotificationQueue::Scope publish(_notifications);
    const QPointer<NTRIPManager> guard(this);
    _sourceTableController.cancel();
    if (guard) {
        stopNTRIP();
    }
}

void NTRIPManager::fetchMountpoints()
{
    if (_shutdown) {
        return;
    }
    const QGeoCoordinate sortCoord = _sortPositionProvider ? _sortPositionProvider() : QGeoCoordinate();
    _sourceTableController.fetch(_configuration.stream.connection, sortCoord);
}

// -----------------------------------------------------------------------------
// State machine
// -----------------------------------------------------------------------------

bool NTRIPManager::_dispatch(Event ev, const QString& detail, std::chrono::milliseconds retryAfter)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    for (const auto& row : kTransitions) {
        if (row.from == _connectionStatus && row.event == ev) {
            _enterState(row.to, detail, retryAfter);
            return true;
        }
    }
    qCDebug(NTRIPManagerLog) << "NTRIP event" << static_cast<int>(ev) << "ignored in state"
                             << static_cast<int>(_connectionStatus);
    return false;
}

void NTRIPManager::_enterState(ConnectionStatus to, const QString& detail, std::chrono::milliseconds retryAfter)
{
    _stateRevision.invalidate();
    const ConnectionStatus from = _connectionStatus;
    const bool stateChanged = (from != to);
    const QString msg = detail.isEmpty() ? _defaultMessageFor(to) : detail;

    // Commit state + message before running entry actions so that a recursive
    // _dispatch() from inside an entry action (e.g. _startTransport → ConfigInvalid)
    // observes the already-committed state, not the stale caller value.
    _connectionStatus = to;
    const bool msgChanged = (_statusMessage != msg);
    if (msgChanged) {
        _statusMessage = msg;
    }

    if (stateChanged) {
        qCDebug(NTRIPManagerLog) << "NTRIP state" << static_cast<int>(from) << "→" << static_cast<int>(to) << msg;
        _notifications.emitSignal(this, &NTRIPManager::connectionStatusChanged);
    }
    if (msgChanged) {
        _notifications.emitSignal(this, &NTRIPManager::statusMessageChanged);
    }

    // Entry action runs on every dispatched transition, including self-transitions
    // (e.g. Connecting→Connecting on HotReconfigure). Signals stay gated above.
    _onEnterState(from, to, retryAfter);
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

void NTRIPManager::_onEnterState(ConnectionStatus /*from*/, ConnectionStatus to, std::chrono::milliseconds retryAfter)
{
    const auto state = _stateRevision.current(this);
    switch (to) {
        case ConnectionStatus::Disconnected:
        case ConnectionStatus::Error:
            _cancelReconnect();
            if (!_stopStreaming()) {
                return;
            }
            _setSecurityWarning({});
            _runningConfig = {};
            break;

        case ConnectionStatus::Connecting:
            _cancelReconnect();
            _setSecurityWarning({});
            _teardownTransport();
            if (state.isCurrent()) {
                _startTransport();
            }
            break;

        case ConnectionStatus::Connected:
            _resetReconnectAttempts();
            _ggaProvider.start(_transport);
            if (state.isCurrent()) {
                _stats.start();
            }
            break;

        case ConnectionStatus::Reconnecting:
            if (_stopStreaming()) {
                _scheduleReconnect(retryAfter);
            }
            break;
    }
}

// -----------------------------------------------------------------------------
// Entry-action helpers
// -----------------------------------------------------------------------------

bool NTRIPManager::_stopStreaming()
{
    const auto state = _stateRevision.current(this);
    _teardownTransport();
    if (!state.isCurrent()) {
        return false;
    }
    _ggaProvider.stop();
    if (!state.isCurrent()) {
        return false;
    }
    _stats.stop();
    return state.isCurrent();
}

void NTRIPManager::_teardownTransport()
{
    const auto transport = std::exchange(_transport, {});
    _correctionRegistration.reset();
    if (!transport) {
        return;
    }
    transport->disconnect(this);
    transport->stop();
    if (transport) {
        transport->deleteLater();
    }
}

int NTRIPManager::_reconnectBackoffMs(std::chrono::milliseconds retryAfter) const
{
    const int exponentialMs = qMin(kMinReconnectMs * (1 << qMin(_reconnectAttempts, 5)), kMaxReconnectMs);
    return static_cast<int>(
        std::clamp(retryAfter, std::chrono::milliseconds{exponentialMs}, std::chrono::milliseconds{300000}).count());
}

void NTRIPManager::_scheduleReconnect(std::chrono::milliseconds retryAfter)
{
    // Backoff uses the pre-increment attempt count: attempt #1 waits kMinReconnectMs,
    // #2 waits 2x, etc. Increment, then check the ceiling.
    const auto backoff = std::chrono::milliseconds{_reconnectBackoffMs(retryAfter)};
    ++_reconnectAttempts;
    if (_reconnectExhausted()) {
        _dispatch(Event::ReconnectGaveUp, tr("Gave up after %1 reconnect attempts").arg(kMaxReconnectAttempts));
        return;
    }
    _pendingReconnectDelay = backoff;
    _reconnectTask.schedule(backoff, [this]() {
        _pendingReconnectDelay = {};
        _dispatch(Event::ReconnectDue);
    });
}

void NTRIPManager::_cancelReconnect()
{
    _reconnectTask.cancel();
    _pendingReconnectDelay = {};
}

void NTRIPManager::_startTransport()
{
    const auto state = _stateRevision.current(this);
    if (_shutdown) {
        _dispatch(Event::ConfigInvalid, tr("NTRIP is shut down"));
        return;
    }

    const NTRIPConfiguration config = _configuration.stream;
    const auto& connection = config.connection;

    if (const QString err = connection.streamValidationError(); !err.isEmpty()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid:" << err << "host=" << connection.host
                                   << " port=" << connection.port;
        _dispatch(Event::ConfigInvalid, err);
        return;
    }

    qCDebug(NTRIPManagerLog) << "startTransport: host=" << connection.host << " port=" << connection.port
                             << " mount=" << connection.mountpoint;

    // Replace the generic "Connecting..." with a host-specific message.
    const QString msg = tr("Connecting to %1:%2...").arg(connection.host).arg(connection.port);
    if (_statusMessage != msg) {
        _statusMessage = msg;
        _notifications.emitSignal(this, &NTRIPManager::statusMessageChanged);
    }

    _stats.reset();
    if (!state.isCurrent()) {
        return;
    }
    _runningConfig = config;

    if (_injectedTransport) {
        _transport = _injectedTransport;
        _injectedTransport = nullptr;
    } else {
        _transport = new NTRIPHttpTransport(config.connection, config.filter, this, _scheduler);
    }

    const QPointer<NTRIPTransport> transport = _transport;
    const QPointer<GPSCorrectionManager> correctionManager = _correctionManager;
    if (correctionManager) {
        QUrl endpoint;
        endpoint.setScheme(connection.useTls ? QStringLiteral("ntrips") : QStringLiteral("ntrip"));
        endpoint.setHost(connection.host);
        endpoint.setPort(connection.port);
        endpoint.setPath(QLatin1Char('/') + connection.mountpoint);
        auto registration =
            correctionManager->registerSource(GPSCorrectionSource::Ntrip, endpoint.toString(QUrl::FullyEncoded));
        if (!state.isCurrent() || !transport || _transport != transport) {
            return;
        }
        _correctionRegistration = std::move(registration);
    }
    const auto token = _correctionRegistration.token();
    const auto current = [this, guard = QPointer<NTRIPManager>(this), transport, token,
                          registered = !correctionManager.isNull()]() {
        return guard && transport && _transport == transport && (!registered || token.valid());
    };
    // Error handling may retire the emitting transport.
    connect(
        _transport, &NTRIPTransport::error, this,
        [this, transport](const NTRIPFailure& failure) {
            if (transport && _transport == transport) {
                _onTransportError(failure);
            }
        },
        Qt::QueuedConnection);

    // Handshake state must precede subsequently queued errors.
    connect(_transport, &NTRIPTransport::connected, this, [this, current]() {
        if (current()) {
            _dispatch(Event::TransportConnected);
        }
    });

    connect(
        _transport, &NTRIPTransport::correctionFrameReceived, this,
        [this, current, correctionManager, token](const RTCMDecodedFrame& frame) {
            if (correctionManager) {
                correctionManager->acceptIngress(token.event(frame));
            }
            if (current() && frame.valid && !frame.filtered) {
                _rtcmDataReceived(frame);
            }
        },
        Qt::QueuedConnection);

    connect(_transport, &NTRIPTransport::plaintextCredentialsWarning, this, [this, transport]() {
        if (transport && _transport == transport) {
            _onPlaintextCredentialsWarning();
        }
    });

    _transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP transport started";
}

// -----------------------------------------------------------------------------
// Signal handlers
// -----------------------------------------------------------------------------

void NTRIPManager::_onTransportError(const NTRIPFailure& failure)
{
    const auto code = failure.code;
    const auto& detail = failure.detail;
    if (_connectionStatus != ConnectionStatus::Connecting && _connectionStatus != ConnectionStatus::Connected) {
        return;
    }
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << static_cast<int>(code) << detail;

    if (_isEnabled() && isRetryable(code)) {
        const int backoffMs = _reconnectBackoffMs(failure.retryAfter);
        qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << backoffMs << "ms (attempt" << (_reconnectAttempts + 1)
                                 << ")";
        _dispatch(Event::TransportError, tr("Reconnecting in %1s: %2").arg(backoffMs / 1000).arg(detail),
                  failure.retryAfter);
    } else {
        _dispatch(Event::TransportFatalError, detail);
    }
}

void NTRIPManager::_onPlaintextCredentialsWarning()
{
    _setSecurityWarning(tr("Credentials are being sent without TLS encryption."));
}

void NTRIPManager::_setSecurityWarning(const QString& warning)
{
    if (_securityWarning == warning) {
        return;
    }
    _securityWarning = warning;
    _notifications.emitSignal(this, &NTRIPManager::securityWarningChanged);
}

void NTRIPManager::_rtcmDataReceived(const RTCMDecodedFrame& frame)
{
    const auto state = _stateRevision.current(this);
    _stats.recordMessage(frame.data.size(), frame.messageId, frame.receivedAtMs);
    if (!state.isCurrent()) {
        return;
    }
    if (!_correctionManager) {
        qCWarning(NTRIPManagerLog) << "Correction manager not ready; dropping" << frame.data.size() << "bytes";
    }
    if (_connectionStatus != ConnectionStatus::Connected) {
        _dispatch(Event::RTCMBeforeConnected);
    }
}

void NTRIPManager::_applyConfiguration()
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const auto state = _stateRevision.current(this);
    if (_shutdown) {
        return;
    }

    if (!_isEnabled()) {
        _resetReconnectAttempts();
        stopNTRIP();
        return;
    }

    const bool isActive =
        (_connectionStatus == ConnectionStatus::Connecting || _connectionStatus == ConnectionStatus::Connected);

    if (!isActive) {
        // Disconnected / Error / Reconnecting: start fresh. The connecting
        // path re-reads settings, so the new values take effect there.
        startNTRIP();
        return;
    }

    const NTRIPConfiguration newConfig = _configuration.stream;

    if (newConfig.connection != _runningConfig.connection) {
        qCDebug(NTRIPManagerLog) << "NTRIP transport-affecting setting changed, reconnecting";
        _dispatch(Event::HotReconfigure);
        return;
    }

    if (newConfig.filter != _runningConfig.filter && _transport) {
        qCDebug(NTRIPManagerLog) << "NTRIP RTCM whitelist changed, applying to live parser";
        _transport->setRtcmWhitelist(newConfig.filter.messageIds());
    }
    if (state.isCurrent()) {
        _runningConfig = newConfig;
    }
}
