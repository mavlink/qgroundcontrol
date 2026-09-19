#include "NTRIPManager.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QtMath>

#include "Fact.h"
#include "GPSCorrectionManager.h"
#include "MultiVehicleManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"
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
    return _correctionManager ? _correctionManager->rtcmMavlink() : nullptr;
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
        const auto configureGga = [this]() {
            _ggaProvider.configure(
                {static_cast<NTRIPGgaProvider::PositionSource>(
                     _settings->ntripGgaPositionSource()->rawValue().toUInt()),
                 std::chrono::milliseconds(_settings->ntripGgaIntervalSec()->rawValue().toUInt() * qint64(1000))});
        };
        configureGga();
        connect(_settings->ntripGgaPositionSource(), &Fact::rawValueChanged, this, configureGga);
        connect(_settings->ntripGgaIntervalSec(), &Fact::rawValueChanged, this, configureGga);
    }

    if (_settings) {
        _onSettingChanged();
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

NTRIPConfiguration NTRIPManager::_configFromSettings() const
{
    const auto read = [](Fact* fact, const QVariant& fallback) { return fact ? fact->rawValue() : fallback; };
    NTRIPConfiguration config;
    auto& connection = config.connection;
    connection.host = read(_settings->ntripServerHostAddress(), connection.host).toString();
    connection.port = read(_settings->ntripServerPort(), connection.port).toInt();
    connection.username = read(_settings->ntripUsername(), connection.username).toString();
    connection.password = read(_settings->ntripPassword(), connection.password).toString();
    connection.mountpoint = read(_settings->ntripMountpoint(), connection.mountpoint).toString();
    connection.useTls = read(_settings->ntripUseTls(), connection.useTls).toBool();
    connection.allowSelfSignedCerts =
        read(_settings->ntripAllowSelfSignedCerts(), connection.allowSelfSignedCerts).toBool();
    config.filter.whitelist = read(_settings->ntripWhitelist(), config.filter.whitelist).toString();
    auto& udpForward = config.udpForward;
    udpForward.enabled = read(_settings->ntripUdpForwardEnabled(), udpForward.enabled).toBool();
    udpForward.address = read(_settings->ntripUdpTargetAddress(), udpForward.address).toString();
    udpForward.port = static_cast<quint16>(read(_settings->ntripUdpTargetPort(), udpForward.port).toUInt());
    return config;
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
    _sourceTableController.fetch(_configFromSettings().connection, sortCoord);
}

// -----------------------------------------------------------------------------
// State machine
// -----------------------------------------------------------------------------

bool NTRIPManager::_dispatch(Event ev, const QString& detail, std::chrono::milliseconds retryAfter)
{
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
    const QPointer<NTRIPManager> guard(this);
    const quint64 revision = ++_stateRevision;
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
    if (!guard || _stateRevision != revision) {
        return;
    }
    if (msgChanged) {
        emit statusMessageChanged();
    }
    if (!guard || _stateRevision != revision) {
        return;
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
    const QPointer<NTRIPManager> guard(this);
    const quint64 revision = _stateRevision;
    const auto current = [this, guard, revision]() { return guard && _stateRevision == revision; };
    switch (to) {
        case ConnectionStatus::Disconnected:
        case ConnectionStatus::Error:
            _cancelReconnect();
            _teardownTransport();
            if (!current()) {
                return;
            }
            _ggaProvider.stop();
            if (!current()) {
                return;
            }
            _stats.stop();
            if (!current()) {
                return;
            }
            _applyUdpForwarderConfig({});
            if (!current()) {
                return;
            }
            _setSecurityWarning({});
            if (!current()) {
                return;
            }
            _runningConfig = {};
            break;

        case ConnectionStatus::Connecting:
            _cancelReconnect();
            _setSecurityWarning({});
            if (!current()) {
                return;
            }
            _teardownTransport();
            if (current()) {
                _startTransport();
            }
            break;

        case ConnectionStatus::Connected:
            _resetReconnectAttempts();
            _casterStatus = CasterStatus::CasterConnected;
            emit casterStatusChanged(_casterStatus);
            if (!current()) {
                return;
            }
            _ggaProvider.start(_transport);
            if (current()) {
                _stats.start();
            }
            break;

        case ConnectionStatus::Reconnecting:
            _teardownTransport();
            if (!current()) {
                return;
            }
            _ggaProvider.stop();
            if (!current()) {
                return;
            }
            _stats.stop();
            if (current()) {
                _scheduleReconnect(retryAfter);
            }
            break;

    }
}

// -----------------------------------------------------------------------------
// Entry-action helpers
// -----------------------------------------------------------------------------

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
    _reconnectTimer.setInterval(backoff);
    _reconnectTimer.start();
}

void NTRIPManager::_startTransport()
{
    const QPointer<NTRIPManager> guard(this);
    const quint64 revision = _stateRevision;
    const auto sameState = [this, guard, revision]() { return guard && _stateRevision == revision; };
    if (!_settings) {
        _dispatch(Event::ConfigInvalid, tr("Settings unavailable"));
        return;
    }

    const NTRIPConfiguration config = _configFromSettings();
    const auto& connection = config.connection;

    _applyUdpForwarderConfig(config.udpForward);
    if (!sameState()) {
        return;
    }

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
        emit statusMessageChanged();
    }
    if (!sameState()) {
        return;
    }

    _stats.reset();
    if (!sameState()) {
        return;
    }
    _runningConfig = config;

    if (_injectedTransport) {
        _transport = _injectedTransport;
        _injectedTransport = nullptr;
    } else {
        _transport = new NTRIPHttpTransport(config.connection, config.filter, this);
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
        if (!sameState() || !transport || _transport != transport) {
            return;
        }
        _correctionRegistration = std::move(registration);
    }
    const auto token = _correctionRegistration.token();
    const auto current = [this, guard, transport, token, registered = !correctionManager.isNull()]() {
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
        [this, current, correctionManager, token](const RTCMFrameDecoder::Result& frame) {
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
    const QPointer<NTRIPManager> guard(this);
    const quint64 revision = _stateRevision;
    if (_connectionStatus != ConnectionStatus::Connecting && _connectionStatus != ConnectionStatus::Connected) {
        return;
    }
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << static_cast<int>(code) << detail;

    const CasterStatus caster =
        (code == NTRIPError::NoLocation) ? CasterStatus::CasterNoLocation : CasterStatus::CasterError;
    if (_casterStatus != caster) {
        _casterStatus = caster;
        emit casterStatusChanged(_casterStatus);
    }
    if (!guard || _stateRevision != revision) {
        return;
    }

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

void NTRIPManager::_rtcmDataReceived(const RTCMFrameDecoder::Result& frame)
{
    const QPointer<NTRIPManager> guard(this);
    const quint64 revision = _stateRevision;
    _stats.recordMessage(frame.data.size(), frame.messageId, frame.receivedAtMs);
    if (!guard || _stateRevision != revision) {
        return;
    }
    if (!_correctionManager) {
        qCWarning(NTRIPManagerLog) << "Correction manager not ready; dropping" << frame.data.size() << "bytes";
    }
    if (_connectionStatus != ConnectionStatus::Connected) {
        _dispatch(Event::RTCMBeforeConnected);
    }
}

bool NTRIPManager::_isEnabled() const
{
    return _settings && _settings->ntripServerConnectEnabled() &&
           _settings->ntripServerConnectEnabled()->rawValue().toBool();
}

void NTRIPManager::_onSettingChanged()
{
    const QPointer<NTRIPManager> guard(this);
    const quint64 revision = _stateRevision;
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

    const NTRIPConfiguration newConfig = _configFromSettings();

    if (newConfig.connection != _runningConfig.connection) {
        qCDebug(NTRIPManagerLog) << "NTRIP transport-affecting setting changed, reconnecting";
        _dispatch(Event::HotReconfigure);
        return;
    }

    if (newConfig.udpForward != _runningConfig.udpForward) {
        qCDebug(NTRIPManagerLog) << "NTRIP UDP forward settings changed, reconfiguring in place";
        _applyUdpForwarderConfig(newConfig.udpForward);
    }
    if (!guard || _stateRevision != revision) {
        return;
    }

    if (newConfig.filter != _runningConfig.filter && _transport) {
        qCDebug(NTRIPManagerLog) << "NTRIP RTCM whitelist changed, applying to live parser";
        _transport->setRtcmWhitelist(newConfig.filter.messageIds());
    }
    if (guard && _stateRevision == revision) {
        _runningConfig = newConfig;
    }
}

void NTRIPManager::_applyUdpForwarderConfig(const NTRIPUdpForwardConfig& config)
{
    if (_correctionManager) {
        _correctionManager->configureNtripUdpOutput(config.enabled, config.address, config.port);
    }
}
