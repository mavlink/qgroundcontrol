#include "NTRIPManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QtMath>
#include <chrono>

#include "Fact.h"
#include "MultiVehicleManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "RTCMUdpInput.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIPManager")

Q_APPLICATION_STATIC(NTRIPManager, _ntripManagerInstance);

NTRIPManager* NTRIPManager::instance()
{
    return _ntripManagerInstance();
}

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
            return false;
        default:
            return true;
    }
}

}  // namespace

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

NTRIPManager::NTRIPManager(QObject* parent) : QObject(parent)
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager created";

    _settingsDebounceTimer.setSingleShot(true);
    _settingsDebounceTimer.setInterval(kSettingsDebounceMs);
    connect(&_settingsDebounceTimer, &QChronoTimer::timeout, this, &NTRIPManager::_onSettingChanged);

    connect(&_ggaProvider, &NTRIPGgaProvider::sourceChanged, this, &NTRIPManager::ggaSourceChanged);

    connect(&_sourceTableController, &NTRIPSourceTableController::mountpointSelected, this,
            [this](const QString& mountpoint) {
                if (_settings && _settings->ntripMountpoint()) {
                    _settings->ntripMountpoint()->setRawValue(mountpoint);
                }
            });

    _reconnectTimer.setSingleShot(true);
    _reconnectTimer.callOnTimeout(this, [this]() { _dispatch(Event::ReconnectDue); });

    // DirectConnection: queued slot may not dispatch before destruction during quit.
    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::DirectConnection);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager destroyed";
    stopNTRIP();
}

RTCMMavlink* NTRIPManager::rtcmMavlink() const
{
    return _rtcmMavlink;
}

void NTRIPManager::setRtcmMavlink(RTCMMavlink* mavlink)
{
    _rtcmMavlink = mavlink;
}

void NTRIPManager::init()
{
    if (_initialized) {
        qCWarning(NTRIPManagerLog) << "NTRIPManager::init() called more than once";
        return;
    }
    _initialized = true;

    _settings = SettingsManager::instance()->ntripSettings();
    if (!_settings) {
        qCCritical(NTRIPManagerLog) << "init: NTRIPSettings unavailable — SettingsManager not ready?";
    } else {
        const Fact* facts[] = {
            _settings->ntripServerConnectEnabled(),
            _settings->ntripServerHostAddress(),
            _settings->ntripServerPort(),
            _settings->ntripUsername(),
            _settings->ntripPassword(),
            _settings->ntripMountpoint(),
            _settings->ntripWhitelist(),
            _settings->ntripUseTls(),
            _settings->ntripAllowSelfSignedCerts(),
            _settings->ntripUdpForwardEnabled(),
            _settings->ntripUdpTargetAddress(),
            _settings->ntripUdpTargetPort(),
        };
        for (const auto* fact : facts) {
            if (fact) {
                connect(fact, &Fact::rawValueChanged, this, [this]() { _settingsDebounceTimer.start(); });
            }
        }
    }

    _ggaProvider.init(_settings);

    // RTCMMavlink may be injected for tests; otherwise own one so RTCM corrections
    // reach connected vehicles. Mirrors the legacy self-wiring behavior.
    if (!_rtcmMavlink) {
        QObject* parentObj = qgcApp() ? static_cast<QObject*>(qgcApp()) : static_cast<QObject*>(this);
        _rtcmMavlink = new RTCMMavlink(parentObj);
        _rtcmMavlink->setObjectName(QStringLiteral("RTCMMavlink"));
    }

    _setupRtcmUdpInput();

    if (_settings) {
        _onSettingChanged();
    }
}

void NTRIPManager::_setupRtcmUdpInput()
{
    if (!_settings || !_settings->rtcmUdpInputPort()) {
        return;
    }

    const quint16 port = static_cast<quint16>(_settings->rtcmUdpInputPort()->rawValue().toUInt());
    _rtcmUdpInput = new RTCMUdpInput(port, this);
    connect(_rtcmUdpInput, &RTCMUdpInput::rtcmDataReceived, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);

    auto applyUdpInputSettings = [this]() {
        const quint16 inPort = static_cast<quint16>(_settings->rtcmUdpInputPort()->rawValue().toUInt());
        _rtcmUdpInput->setPort(inPort);
        _rtcmUdpInput->setValidation(_settings->rtcmUdpValidate()->rawValue().toBool());
        if (_settings->rtcmUdpInputEnabled()->rawValue().toBool()) {
            _rtcmUdpInput->start();
        } else {
            _rtcmUdpInput->stop();
        }
    };
    connect(_settings->rtcmUdpInputEnabled(), &Fact::rawValueChanged, this, applyUdpInputSettings);
    connect(_settings->rtcmUdpInputPort(), &Fact::rawValueChanged, this, applyUdpInputSettings);
    connect(_settings->rtcmUdpValidate(), &Fact::rawValueChanged, this, applyUdpInputSettings);
    applyUdpInputSettings();
}

// -----------------------------------------------------------------------------
// Public control surface
// -----------------------------------------------------------------------------

void NTRIPManager::startNTRIP()
{
    _dispatch(Event::StartRequested);
}

void NTRIPManager::stopNTRIP()
{
    _dispatch(Event::StopRequested);
}

void NTRIPManager::fetchMountpoints()
{
    if (!_settings) {
        return;
    }
    QGeoCoordinate sortCoord;
    if (MultiVehicleManager* mvm = MultiVehicleManager::instance(); mvm && mvm->activeVehicle()) {
        sortCoord = mvm->activeVehicle()->coordinate();
    }
    _sourceTableController.fetch(NTRIPTransportConfig::fromSettings(*_settings), sortCoord);
}

// -----------------------------------------------------------------------------
// State machine
// -----------------------------------------------------------------------------

bool NTRIPManager::_dispatch(Event ev, const QString& detail)
{
    for (const auto& row : kTransitions) {
        if (row.from == _connectionStatus && row.event == ev) {
            _enterState(row.to, detail);
            return true;
        }
    }
    qCDebug(NTRIPManagerLog) << "NTRIP event" << static_cast<int>(ev) << "ignored in state"
                             << static_cast<int>(_connectionStatus);
    return false;
}

void NTRIPManager::_enterState(ConnectionStatus to, const QString& detail)
{
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
        emit connectionStatusChanged();
    }
    if (msgChanged) {
        emit statusMessageChanged();
    }

    // Entry action runs on every dispatched transition, including self-transitions
    // (e.g. Connecting→Connecting on HotReconfigure). Signals stay gated above.
    _onEnterState(from, to);
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

void NTRIPManager::_onEnterState(ConnectionStatus /*from*/, ConnectionStatus to)
{
    // Per-state side effects. Teardown helpers are idempotent — safe to call
    // from any state where the resource may or may not be active.
    switch (to) {
        case ConnectionStatus::Disconnected:
            _cancelReconnect();
            _teardownTransport();
            _ggaProvider.stop();
            _stats.stop();
            _udpForwarder.stop();
            _setSecurityWarning({});
            _runningConfig = {};
            break;

        case ConnectionStatus::Connecting:
            _cancelReconnect();    // may have arrived here via Reconnecting → StartRequested
            _setSecurityWarning({});
            _teardownTransport();  // clear any lingering transport from a prior attempt
            _startTransport();     // may recursively dispatch ConfigInvalid → Error
            break;

        case ConnectionStatus::Connected:
            _resetReconnectAttempts();
            _casterStatus = CasterStatus::CasterConnected;
            emit casterStatusChanged(_casterStatus);
            _ggaProvider.start(_transport);
            _stats.start();
            break;

        case ConnectionStatus::Reconnecting:
            _teardownTransport();
            _ggaProvider.stop();
            _stats.stop();
            _scheduleReconnect();
            break;

        case ConnectionStatus::Error:
            _cancelReconnect();
            _teardownTransport();
            _ggaProvider.stop();
            _stats.stop();
            _udpForwarder.stop();
            _setSecurityWarning({});
            _runningConfig = {};
            break;
    }
}

// -----------------------------------------------------------------------------
// Entry-action helpers
// -----------------------------------------------------------------------------

void NTRIPManager::_teardownTransport()
{
    if (!_transport) {
        return;
    }
    _transport->disconnect(this);
    _transport->stop();
    _transport->deleteLater();
    _transport = nullptr;
}

int NTRIPManager::_reconnectBackoffMs() const
{
    return qMin(kMinReconnectMs * (1 << qMin(_reconnectAttempts, 5)), kMaxReconnectMs);
}

void NTRIPManager::_scheduleReconnect()
{
    // Backoff uses the pre-increment attempt count: attempt #1 waits kMinReconnectMs,
    // #2 waits 2x, etc. Increment, then check the ceiling.
    const auto backoff = std::chrono::milliseconds{_reconnectBackoffMs()};
    ++_reconnectAttempts;
    if (_reconnectExhausted()) {
        _dispatch(Event::ReconnectGaveUp, tr("Gave up after %1 reconnect attempts").arg(kMaxReconnectAttempts));
        return;
    }
    _reconnectTimer.setInterval(backoff);
    _reconnectTimer.start();
}

void NTRIPManager::_startTransport()
{
    if (!_settings) {
        _dispatch(Event::ConfigInvalid, tr("Settings unavailable"));
        return;
    }

    NTRIPTransportConfig config = NTRIPTransportConfig::fromSettings(*_settings);

    _applyUdpForwarderConfig(config);

    if (const QString err = config.validationError(); !err.isEmpty()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid:" << err << "host=" << config.host
                                   << " port=" << config.port;
        _dispatch(Event::ConfigInvalid, err);
        return;
    }

    qCDebug(NTRIPManagerLog) << "startTransport: host=" << config.host << " port=" << config.port
                             << " mount=" << config.mountpoint;

    // Replace the generic "Connecting..." with a host-specific message.
    const QString msg = tr("Connecting to %1:%2...").arg(config.host).arg(config.port);
    if (_statusMessage != msg) {
        _statusMessage = msg;
        emit statusMessageChanged();
    }

    _stats.reset();
    _runningConfig = config;

    if (_injectedTransport) {
        _transport = _injectedTransport;
        _injectedTransport = nullptr;
    } else {
        _transport = new NTRIPHttpTransport(config, this);
    }

    // QueuedConnection: _onTransportError may tear the transport down — Direct
    // would destroy it while still inside its own signal emission (use-after-free).
    connect(_transport, &NTRIPTransport::error, this, &NTRIPManager::_onTransportError, Qt::QueuedConnection);

    // Must stay non-queued: TransportConnected is dispatched synchronously so a
    // queued `error` that tore the transport down cannot interleave a stale
    // TransportConnected into the Disconnected state. Do not make this queued.
    connect(_transport, &NTRIPTransport::connected, this, [this]() { _dispatch(Event::TransportConnected); });

    connect(_transport, &NTRIPTransport::RTCMDataUpdate, this, &NTRIPManager::_rtcmDataReceived);

    connect(_transport, &NTRIPTransport::plaintextCredentialsWarning, this, &NTRIPManager::_onPlaintextCredentialsWarning);

    _transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP transport started";
}

// -----------------------------------------------------------------------------
// Signal handlers
// -----------------------------------------------------------------------------

void NTRIPManager::_onTransportError(NTRIPError code, const QString& detail)
{
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << static_cast<int>(code) << detail;

    const CasterStatus caster =
        (code == NTRIPError::NoLocation) ? CasterStatus::CasterNoLocation : CasterStatus::CasterError;
    if (_casterStatus != caster) {
        _casterStatus = caster;
        emit casterStatusChanged(_casterStatus);
    }

    if (_isEnabled() && isRetryable(code)) {
        const int backoffMs = _reconnectBackoffMs();
        qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << backoffMs << "ms (attempt" << (_reconnectAttempts + 1)
                                 << ")";
        _dispatch(Event::TransportError, tr("Reconnecting in %1s: %2").arg(backoffMs / 1000).arg(detail));
    } else {
        _dispatch(Event::TransportFatalError, detail);
    }
}

void NTRIPManager::_onPlaintextCredentialsWarning()
{
    qCWarning(NTRIPManagerLog) << "Credentials sent without TLS encryption — enable TLS in NTRIP settings";
    _setSecurityWarning(tr("Credentials are being sent without TLS encryption."));
}

void NTRIPManager::_setSecurityWarning(const QString& warning)
{
    if (_securityWarning == warning) {
        return;
    }
    _securityWarning = warning;
    emit securityWarningChanged();
}

void NTRIPManager::_rtcmDataReceived(const QByteArray& data, int messageId)
{
    _stats.recordMessage(data.size(), messageId);

    qCDebug(NTRIPManagerLog) << "NTRIP forwarding RTCM:" << data.size() << "bytes";

    RTCMMavlink* mavlink = _rtcmMavlink;
    if (mavlink) {
        mavlink->RTCMDataUpdate(data);

        if (_connectionStatus != ConnectionStatus::Connected) {
            // RTCM arrived before we processed the connected() signal — normalize
            // through the state machine so this stays the single source of truth.
            // No-op (logged) from any state without a matching transition row.
            _dispatch(Event::RTCMBeforeConnected);
        }
    } else {
        qCWarning(NTRIPManagerLog) << "RTCMMavlink not ready; dropping" << data.size() << "bytes";
    }

    _udpForwarder.forward(data);
}

bool NTRIPManager::_isEnabled() const
{
    return _settings && _settings->ntripServerConnectEnabled() &&
           _settings->ntripServerConnectEnabled()->rawValue().toBool();
}

void NTRIPManager::_onSettingChanged()
{
    if (!_settings) {
        return;
    }

    if (!_isEnabled()) {
        // Match legacy: when disabled while Reconnecting, reset the attempt
        // counter so a future re-enable starts with a clean backoff schedule.
        if (_connectionStatus == ConnectionStatus::Reconnecting) {
            _resetReconnectAttempts();
        }
        _dispatch(Event::StopRequested);
        return;
    }

    const bool isActive =
        (_connectionStatus == ConnectionStatus::Connecting || _connectionStatus == ConnectionStatus::Connected);

    if (!isActive) {
        // Disconnected / Error / Reconnecting: start fresh. The connecting
        // path re-reads settings, so the new values take effect there.
        _dispatch(Event::StartRequested);
        return;
    }

    // Active — classify the diff to avoid unnecessary reconnects.
    // Hot (host, port, creds, mountpoint, TLS) requires a new TCP handshake.
    // Warm (UDP sink) reconfigures the sidecar in place.
    // Cold (whitelist) is pushed to the live parser; the caster doesn't care.
    const NTRIPTransportConfig newConfig = NTRIPTransportConfig::fromSettings(*_settings);

    if (newConfig.transportDiffers(_runningConfig)) {
        qCDebug(NTRIPManagerLog) << "NTRIP transport-affecting setting changed, reconnecting";
        _dispatch(Event::HotReconfigure);
        return;
    }

    if (newConfig.udpForwardDiffers(_runningConfig)) {
        qCDebug(NTRIPManagerLog) << "NTRIP UDP forward settings changed, reconfiguring in place";
        _applyUdpForwarderConfig(newConfig);
    }

    if (newConfig.whitelistDiffers(_runningConfig) && _transport) {
        qCDebug(NTRIPManagerLog) << "NTRIP RTCM whitelist changed, applying to live parser";
        _transport->setRtcmWhitelist(NTRIPTransportConfig::parseWhitelist(newConfig.whitelist));
    }

    _runningConfig = newConfig;
}

void NTRIPManager::_applyUdpForwarderConfig(const NTRIPTransportConfig& config)
{
    if (!config.udpForwardEnabled) {
        _udpForwarder.stop();
        return;
    }
    if (!_udpForwarder.configure(config.udpTargetAddress, config.udpTargetPort)) {
        qCWarning(NTRIPManagerLog) << "UDP forward config invalid:" << config.udpTargetAddress << config.udpTargetPort;
    }
}
