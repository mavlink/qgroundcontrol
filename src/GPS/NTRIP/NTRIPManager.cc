#include "NTRIPManager.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPTransport.h"
#include "NTRIPSettings.h"
#include "Fact.h"
#include "GPSEvent.h"
#include "GPSManager.h"
#include "QGCLoggingCategory.h"
#include "RTCMRouter.h"
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

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (settings) {
        auto connectSetting = [this](Fact* fact) {
            if (fact) {
                connect(fact, &Fact::rawValueChanged, this, [this]() {
                    _settingsDebounceTimer.start();
                });
            }
        };
        connectSetting(settings->ntripServerConnectEnabled());
        connectSetting(settings->ntripServerHostAddress());
        connectSetting(settings->ntripServerPort());
        connectSetting(settings->ntripUsername());
        connectSetting(settings->ntripPassword());
        connectSetting(settings->ntripMountpoint());
        connectSetting(settings->ntripWhitelist());
        connectSetting(settings->ntripUseTls());
        connectSetting(settings->ntripUdpForwardEnabled());
        connectSetting(settings->ntripUdpTargetAddress());
        connectSetting(settings->ntripUdpTargetPort());
    }

    _settingsDebounceTimer.setSingleShot(true);
    _settingsDebounceTimer.setInterval(kSettingsDebounceMs);
    connect(&_settingsDebounceTimer, &QTimer::timeout, this, &NTRIPManager::_onSettingChanged);

    QTimer::singleShot(0, this, &NTRIPManager::_onSettingChanged);

    connect(&_ggaProvider, &NTRIPGgaProvider::sourceChanged, this, &NTRIPManager::ggaSourceChanged);
    connect(&_reconnectPolicy, &NTRIPReconnectPolicy::reconnectRequested, this, [this]() {
        if (!_transport) {
            startNTRIP();
        }
    });

    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::QueuedConnection);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager destroyed";
    stopNTRIP();
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
    if (_opState != from) {
        qCCritical(NTRIPManagerLog) << "Invalid state transition: expected"
                                    << static_cast<int>(from) << "actual" << static_cast<int>(_opState)
                                    << "target" << static_cast<int>(to);
        return false;
    }
    _opState = to;
    return true;
}

void NTRIPManager::startNTRIP()
{
    if (!_transition(OperationState::Idle, OperationState::Starting)) {
        return;
    }

    _reconnectPolicy.cancel();

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (!settings) {
        _opState = OperationState::Idle;
        return;
    }

    NTRIPTransportConfig config = NTRIPTransportConfig::fromSettings(*settings);

    if (settings->ntripUdpForwardEnabled() && settings->ntripUdpForwardEnabled()->rawValue().toBool()) {
        const QString udpAddr = settings->ntripUdpTargetAddress()->rawValue().toString();
        const quint16 udpPort = static_cast<quint16>(settings->ntripUdpTargetPort()->rawValue().toUInt());
        _udpForwarder.configure(udpAddr, udpPort);
    }

    if (!config.isValid()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid: host=" << config.host << " port=" << config.port;
        _setStatus(ConnectionStatus::Error,
                   config.host.isEmpty() ? tr("No host address") : tr("Invalid port"));
        _opState = OperationState::Idle;
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

    connect(_transport, &NTRIPTransport::error,
            this, &NTRIPManager::_transportError);

    connect(_transport, &NTRIPTransport::connected, this, [this]() {
        _reconnectPolicy.resetAttempts();
        _casterStatus = CasterStatus::CasterConnected;
        emit casterStatusChanged(_casterStatus);

        _setStatus(ConnectionStatus::Connected, tr("Connected"));
        GPSManager::instance()->logEvent(GPSEvent::info(GPSEvent::Source::NTRIP, tr("Connected to caster")));

        _ggaProvider.start(_transport);
        _stats.start();
    });

    connect(_transport, &NTRIPTransport::RTCMDataUpdate, this, &NTRIPManager::_rtcmDataReceived);

    _transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP started";

    _opState = OperationState::Running;
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
    _opState = OperationState::Stopping;

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

    _opState = OperationState::Idle;
}

void NTRIPManager::_transportError(NTRIPError code, const QString& detail)
{
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << static_cast<int>(code) << detail;
    GPSManager::instance()->logEvent(GPSEvent::error(GPSEvent::Source::NTRIP, detail));
    _setStatus(ConnectionStatus::Error, detail);

    if (code == NTRIPError::NoLocation) {
        _casterStatus = CasterStatus::CasterNoLocation;
    } else {
        _casterStatus = CasterStatus::CasterError;
    }
    emit casterStatusChanged(_casterStatus);

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    bool shouldRun = settings && settings->ntripServerConnectEnabled() &&
                     settings->ntripServerConnectEnabled()->rawValue().toBool();

    // Tear down the current transport before deciding whether to reconnect
    stopNTRIP();

    if (!shouldRun) {
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

    RTCMRouter *router = GPSManager::instance()->rtcmRouter();
    if (router) {
        router->routeAll(data);

        if (_connectionStatus != ConnectionStatus::Connected) {
            _setStatus(ConnectionStatus::Connected, tr("Connected"));
        }
    } else {
        qCWarning(NTRIPManagerLog) << "RTCMRouter not ready; dropping" << data.size() << "bytes";
    }

    _udpForwarder.forward(data);
}

void NTRIPManager::_onSettingChanged()
{
    if (_opState == OperationState::Starting || _opState == OperationState::Stopping) {
        return;
    }

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    bool shouldRun = settings && settings->ntripServerConnectEnabled() &&
                     settings->ntripServerConnectEnabled()->rawValue().toBool();
    bool isRunning = (_transport != nullptr);

    if (shouldRun && isRunning && settings) {
        NTRIPTransportConfig config = NTRIPTransportConfig::fromSettings(*settings);
        const bool udpFwd = settings->ntripUdpForwardEnabled()->rawValue().toBool();
        const QString udpAddr = settings->ntripUdpTargetAddress()->rawValue().toString();
        const quint16 udpPort = static_cast<quint16>(settings->ntripUdpTargetPort()->rawValue().toUInt());

        const bool udpChanged = udpFwd != _udpForwarder.isEnabled()
                             || udpAddr != _udpForwarder.address()
                             || udpPort != _udpForwarder.port();

        if (config != _runningConfig || udpChanged) {
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
    NTRIPSettings *settings = SettingsManager::instance()->ntripSettings();
    if (!settings) {
        return;
    }
    _sourceTableController.fetch(NTRIPTransportConfig::fromSettings(*settings));
}
