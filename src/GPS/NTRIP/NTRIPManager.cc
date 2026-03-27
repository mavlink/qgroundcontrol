#include "NTRIPManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSettings.h"
#include "Fact.h"
#include "GPSEvent.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "SettingsManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIPManager")

Q_APPLICATION_STATIC(NTRIPManager, _ntripManagerInstance);

NTRIPManager* NTRIPManager::instance()
{
    return _ntripManagerInstance();
}

NTRIPManager::NTRIPManager(QObject* parent)
    : QObject(parent)
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager created";

    _settings = SettingsManager::instance()->ntripSettings();
    if (_settings) {
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
        for (const auto *fact : facts) {
            if (fact) {
                connect(fact, &Fact::rawValueChanged, this, [this]() {
                    _settingsDebounceTimer.start();
                });
            }
        }
    }

    _settingsDebounceTimer.setSingleShot(true);
    _settingsDebounceTimer.setInterval(kSettingsDebounceMs);
    connect(&_settingsDebounceTimer, &QTimer::timeout, this, &NTRIPManager::_onSettingChanged);

    connect(&_ggaProvider, &NTRIPGgaProvider::sourceChanged, this, &NTRIPManager::ggaSourceChanged);
    connect(&_reconnectPolicy, &NTRIPReconnectPolicy::reconnectRequested, this, [this]() {
        if (!_transport) {
            startNTRIP();
        }
    });

    connect(&_reconnectPolicy, &NTRIPReconnectPolicy::gaveUp, this, [this]() {
        _setStatus(ConnectionStatus::Error, tr("Gave up after %1 reconnect attempts").arg(NTRIPReconnectPolicy::kMaxReconnectAttempts));
        _transition(OperationState::Starting, OperationState::Idle);
    });

    // DirectConnection: aboutToQuit is emitted just before the event loop exits, so a
    // queued call may never dispatch before destruction. Running stopNTRIP inline from
    // the app's quit notifier is safe — it only touches members on the GUI thread.
    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::DirectConnection);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager destroyed";
    stopNTRIP();
}

void NTRIPManager::init()
{
    // Called once from QGCApplication::_initForNormalAppBoot after its dependencies
    // (SettingsManager, GPSManager) are constructed. Replaces the earlier
    // QTimer::singleShot(0) trick, which relied on undefined singleton init ordering.
    if (_initialized) {
        qCWarning(NTRIPManagerLog) << "NTRIPManager::init() called more than once";
        return;
    }
    _initialized = true;

    if (_settings) {
        _onSettingChanged();
    } else {
        qCWarning(NTRIPManagerLog) << "NTRIP settings not available at init";
    }
}

void NTRIPManager::_setStatus(ConnectionStatus status, const QString& msg)
{
    bool changed = false;

    if (_connectionStatus != status) {
        _connectionStatus = status;
        changed = true;
        emit connectionStatusChanged();
    }

    if (_statusMessage != msg) {
        _statusMessage = msg;
        changed = true;
        emit statusMessageChanged();
    }

    if (changed) {
        qCDebug(NTRIPManagerLog) << "NTRIP status:" << static_cast<int>(status) << msg;
    }
}

bool NTRIPManager::_transition(OperationState from, OperationState to)
{
    // Valid transitions: Idle→Starting, Starting→Running, Starting→Idle (config fail),
    // Running→Stopping, Starting→Stopping, Stopping→Idle
    static constexpr struct { OperationState from, to; } kValidTransitions[] = {
        {OperationState::Idle,     OperationState::Starting},
        {OperationState::Starting, OperationState::Running},
        {OperationState::Starting, OperationState::Idle},
        {OperationState::Starting, OperationState::Stopping},
        {OperationState::Running,  OperationState::Stopping},
        {OperationState::Stopping, OperationState::Idle},
    };

    if (_opState != from) {
        qCCritical(NTRIPManagerLog) << "Invalid state transition: expected"
                                    << static_cast<int>(from) << "actual" << static_cast<int>(_opState)
                                    << "target" << static_cast<int>(to);
        return false;
    }

    for (const auto &t : kValidTransitions) {
        if (t.from == from && t.to == to) {
            _opState = to;
            return true;
        }
    }

    qCCritical(NTRIPManagerLog) << "Illegal state transition:"
                                << static_cast<int>(from) << "→" << static_cast<int>(to);
    return false;
}

bool NTRIPManager::_isEnabled() const
{
    return _settings && _settings->ntripServerConnectEnabled() &&
           _settings->ntripServerConnectEnabled()->rawValue().toBool();
}

void NTRIPManager::_logEvent(const GPSEvent &event)
{
    if (_eventLogger) {
        _eventLogger(event);
    }
}

void NTRIPManager::startNTRIP()
{
    if (!_transition(OperationState::Idle, OperationState::Starting)) {
        return;
    }

    _reconnectPolicy.cancel();

    if (!_settings) {
        _transition(OperationState::Starting, OperationState::Idle);
        return;
    }

    NTRIPTransportConfig config = NTRIPTransportConfig::fromSettings(*_settings);

    if (config.udpForwardEnabled) {
        if (!_udpForwarder.configure(config.udpTargetAddress, config.udpTargetPort)) {
            qCWarning(NTRIPManagerLog) << "UDP forward config invalid:" << config.udpTargetAddress << config.udpTargetPort;
            _logEvent(GPSEvent::warning(GPSEvent::Source::NTRIP,
                tr("UDP forwarding disabled: invalid address or port")));
        }
    }

    if (!config.isValid()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid: host=" << config.host << " port=" << config.port;
        _setStatus(ConnectionStatus::Error,
                   config.host.isEmpty() ? tr("No host address") : tr("Invalid port"));
        _transition(OperationState::Starting, OperationState::Idle);
        return;
    }

    qCDebug(NTRIPManagerLog) << "startNTRIP: host=" << config.host << " port=" << config.port
                             << " mount=" << config.mountpoint;

    _setStatus(ConnectionStatus::Connecting, tr("Connecting to %1:%2...").arg(config.host).arg(config.port));

    _stats.reset();

    _runningConfig = config;

    if (_transportFactory) {
        _transport = _transportFactory(config, this);
    } else {
        _transport = new NTRIPHttpTransport(config, this);
    }

    if (!_transport) {
        qCCritical(NTRIPManagerLog) << "Transport factory returned null";
        _setStatus(ConnectionStatus::Error, tr("Internal error: transport creation failed"));
        _transition(OperationState::Starting, OperationState::Idle);
        return;
    }

    connect(_transport, &NTRIPTransport::error,
            this, &NTRIPManager::_transportError);

    connect(_transport, &NTRIPTransport::connected, this, [this]() {
        _transition(OperationState::Starting, OperationState::Running);
        _reconnectPolicy.resetAttempts();
        _casterStatus = CasterStatus::CasterConnected;
        emit casterStatusChanged(_casterStatus);

        _setStatus(ConnectionStatus::Connected, tr("Connected"));
        _logEvent(GPSEvent::info(GPSEvent::Source::NTRIP, tr("Connected to caster")));

        _ggaProvider.start(_transport);
        _stats.start();
    });

    connect(_transport, &NTRIPTransport::RTCMDataUpdate, this, &NTRIPManager::_rtcmDataReceived);

    connect(_transport, &NTRIPTransport::plaintextCredentialsWarning, this, [this]() {
        _logEvent(GPSEvent::warning(GPSEvent::Source::NTRIP,
            tr("Credentials sent without TLS encryption — enable TLS in NTRIP settings")));
    });

    _transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP started";
    // State stays Starting until the connected signal fires (see lambda above)
}

void NTRIPManager::stopNTRIP()
{
    if (_opState == OperationState::Stopping) {
        return;
    }
    if (_opState == OperationState::Idle) {
        if (_reconnectPolicy.isPending()) {
            _reconnectPolicy.cancel();
            _setStatus(ConnectionStatus::Disconnected, tr("Disconnected"));
        }
        return;
    }
    if (!_transition(_opState, OperationState::Stopping)) {
        qCWarning(NTRIPManagerLog) << "stopNTRIP: unexpected state" << static_cast<int>(_opState);
        return;
    }

    _reconnectPolicy.cancel();

    if (_transport) {
        _transport->disconnect(this);
        _transport->stop();
        _transport->deleteLater();
        _transport = nullptr;

        _runningConfig = {};

        _setStatus(ConnectionStatus::Disconnected, tr("Disconnected"));
        qCDebug(NTRIPManagerLog) << "NTRIP stopped";
    }

    _ggaProvider.stop();
    _stats.stop();
    _udpForwarder.stop();

    _transition(OperationState::Stopping, OperationState::Idle);
}

static bool isRetryable(NTRIPError error)
{
    switch (error) {
    case NTRIPError::AuthFailed:
    case NTRIPError::InvalidConfig:
        return false;
    default:
        return true;
    }
}

void NTRIPManager::_transportError(NTRIPError code, const QString& detail)
{
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << static_cast<int>(code) << detail;
    _logEvent(GPSEvent::error(GPSEvent::Source::NTRIP, detail));
    _setStatus(ConnectionStatus::Error, detail);

    if (code == NTRIPError::NoLocation) {
        _casterStatus = CasterStatus::CasterNoLocation;
    } else {
        _casterStatus = CasterStatus::CasterError;
    }
    emit casterStatusChanged(_casterStatus);

    // Tear down the current transport before deciding whether to reconnect
    stopNTRIP();

    if (!_isEnabled() || !isRetryable(code)) {
        return;
    }

    const int backoffMs = _reconnectPolicy.nextBackoffMs();
    qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << backoffMs << "ms (attempt"
                             << (_reconnectPolicy.attempts() + 1) << ")";
    _setStatus(ConnectionStatus::Reconnecting, tr("Reconnecting in %1s...").arg(backoffMs / 1000));
    _reconnectPolicy.scheduleReconnect();
}

void NTRIPManager::_rtcmDataReceived(const QByteArray& data)
{
    _stats.recordMessage(data.size());

    qCDebug(NTRIPManagerLog) << "NTRIP Forwarding RTCM:" << data.size() << "bytes";

    RTCMMavlink *mavlink = _rtcmMavlink;
    if (mavlink) {
        mavlink->RTCMDataUpdate(data);
        mavlink->routeToBaseStation(data);

        if (_connectionStatus != ConnectionStatus::Connected) {
            _setStatus(ConnectionStatus::Connected, tr("Connected"));
        }
    } else {
        qCWarning(NTRIPManagerLog) << "RTCMMavlink not ready; dropping" << data.size() << "bytes";
    }

    _udpForwarder.forward(data);
}

void NTRIPManager::_onSettingChanged()
{
    if (_opState == OperationState::Starting || _opState == OperationState::Stopping) {
        return;
    }

    const bool shouldRun = _isEnabled();
    const bool isRunning = (_transport != nullptr);

    if (shouldRun && isRunning && _settings) {
        const NTRIPTransportConfig config = NTRIPTransportConfig::fromSettings(*_settings);
        if (config != _runningConfig) {
            qCDebug(NTRIPManagerLog) << "NTRIP settings changed while running, restarting";
            stopNTRIP();
            startNTRIP();
            return;
        }
    }

    const bool reconnectPending = _reconnectPolicy.isPending();

    if (shouldRun && !isRunning && !reconnectPending) {
        startNTRIP();
    } else if (!shouldRun) {
        _reconnectPolicy.cancel();
        if (isRunning) {
            stopNTRIP();
        } else if (reconnectPending) {
            _reconnectPolicy.resetAttempts();
            _setStatus(ConnectionStatus::Disconnected, tr("Disconnected"));
        }
    }
}

void NTRIPManager::fetchMountpoints()
{
    if (!_settings) {
        return;
    }
    _sourceTableController.fetch(NTRIPTransportConfig::fromSettings(*_settings));
}
