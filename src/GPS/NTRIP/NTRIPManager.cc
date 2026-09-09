#include "NTRIPManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>

#include "Fact.h"
#include "MultiVehicleManager.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIP.NTRIPManager")
Q_APPLICATION_STATIC(NTRIPManager, _ntripManagerInstance);

NTRIPManager* NTRIPManager::instance()
{
    return _ntripManagerInstance();
}

NTRIPManager::NTRIPManager(QObject* parent)
    : QObject(parent)
    , _session([](const NTRIPTransportConfig& config, QObject* owner) { return new NTRIPHttpTransport(config, owner); },
               this)
{
    qCDebug(NTRIPManagerLog) << this;
    _settingsDebounceTimer.setSingleShot(true);
    _settingsDebounceTimer.setInterval(std::chrono::milliseconds{250});
    connect(&_settingsDebounceTimer, &QChronoTimer::timeout, this, &NTRIPManager::_onSettingChanged);
    connect(&_session, &NTRIPSession::stateChanged, this, &NTRIPManager::_onSessionState);
    connect(&_session, &NTRIPSession::streamStarted, this, &NTRIPManager::correctionSessionStarted);
    connect(&_session, &NTRIPSession::streamEnded, this, [this](quint64 attemptId) {
        const QPointer<NTRIPManager> guard(this);
        const auto revision = _revision;
        _ggaProvider.stop();
        if (!guard || revision != _revision) {
            return;
        }
        _stats.stop();
        if (guard && revision == _revision) {
            emit correctionSessionEnded(attemptId);
        }
    });
    connect(&_session, &NTRIPSession::streamConnected, this, [this]() {
        _stats.start();
        const QPointer<NTRIPSession> session = &_session;
        _ggaProvider.start([session](const QByteArray& sentence) {
            if (session) {
                session->sendNMEA(sentence);
            }
        });
    });
    connect(&_session, &NTRIPSession::bytesReceived, &_stats, &NTRIPConnectionStats::recordNetworkBytes);
    connect(&_session, &NTRIPSession::correctionReceived, this, &NTRIPManager::_onCorrection);
    connect(&_session, &NTRIPSession::correctionRejected, this, &NTRIPManager::correctionRejectedAt);
    connect(&_session, &NTRIPSession::plaintextCredentialsWarning, this, &NTRIPManager::_onPlaintextCredentialsWarning);
    connect(&_session, &NTRIPSession::failureOccurred, this, [this](const NTRIPFailure& failure) {
        qCWarning(NTRIPManagerLog) << "NTRIP error:" << static_cast<int>(failure.code) << failure.detail;
        const auto status =
            failure.code == NTRIPError::NoLocation ? CasterStatus::CasterNoLocation : CasterStatus::CasterError;
        if (_casterStatus != status) {
            _casterStatus = status;
            emit casterStatusChanged(status);
        }
    });
    connect(&_ggaProvider, &NTRIPGgaProvider::sourceChanged, this, &NTRIPManager::ggaSourceChanged);
    connect(&_sourceTableController, &NTRIPSourceTableController::plaintextCredentialsWarning, this,
            &NTRIPManager::_onPlaintextCredentialsWarning);
    connect(&_sourceTableController, &NTRIPSourceTableController::mountpointSelected, this,
            [this](const QString& mountpoint) {
                if (_settings) {
                    _settings->ntripMountpoint()->setRawValue(mountpoint);
                }
            });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::DirectConnection);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << this;
    _session.disconnect(this);
    _session.stop();
    _ggaProvider.stop();
}

void NTRIPManager::init()
{
    if (_initialized) {
        return;
    }
    _initialized = true;
    _settings = SettingsManager::instance()->ntripSettings();
    if (!_settings) {
        return;
    }
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
    };
    for (const auto* fact : facts) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { _settingsDebounceTimer.start(); });
    }
    _ggaProvider.init(_settings);
    _onSettingChanged();
}

void NTRIPManager::startNTRIP()
{
    if (!_settings) {
        _onSessionState(NTRIPSession::State::Error, tr("Settings unavailable"));
        return;
    }
    const auto config = NTRIPTransportConfig::fromSettings(*_settings);
    if (const auto error = config.streamValidationError(); !error.isEmpty()) {
        qCWarning(NTRIPManagerLog) << "NTRIP config invalid:" << error;
    }
    const QPointer<NTRIPManager> guard(this);
    const auto revision = ++_revision;
    _runningConfig = config;
    _stats.reset();
    if (!guard || revision != _revision) {
        return;
    }
    _session.start(config, _isEnabled());
}

void NTRIPManager::stopNTRIP()
{
    ++_revision;
    _settingsDebounceTimer.stop();
    _session.stop();
}

void NTRIPManager::fetchMountpoints()
{
    if (!_settings) {
        return;
    }
    QGeoCoordinate coordinate;
    if (auto* vehicles = MultiVehicleManager::instance(); vehicles && vehicles->activeVehicle()) {
        coordinate = vehicles->activeVehicle()->coordinate();
    }
    _sourceTableController.fetch(NTRIPTransportConfig::fromSettings(*_settings), coordinate);
}

void NTRIPManager::_onSessionState(NTRIPSession::State state, const QString& message)
{
    const auto revision = ++_revision;
    const auto status = static_cast<ConnectionStatus>(state);
    const bool stateChanged = _connectionStatus != status;
    const bool messageChanged = _statusMessage != message;
    _connectionStatus = status;
    _statusMessage = message;
    const QPointer<NTRIPManager> guard(this);
    if (state == NTRIPSession::State::Disconnected || state == NTRIPSession::State::Error) {
        _ggaProvider.stop();
        if (!guard || revision != _revision) {
            return;
        }
        _stats.stop();
        if (!guard || revision != _revision) {
            return;
        }
        _setSecurityWarning({});
        if (!guard || revision != _revision) {
            return;
        }
    }
    if (state == NTRIPSession::State::Connecting) {
        _setSecurityWarning({});
        if (!guard || revision != _revision) {
            return;
        }
    }
    if (state == NTRIPSession::State::Connected && _casterStatus != CasterStatus::CasterConnected) {
        _casterStatus = CasterStatus::CasterConnected;
        emit casterStatusChanged(_casterStatus);
        if (!guard || revision != _revision) {
            return;
        }
    }
    if (stateChanged) {
        emit connectionStatusChanged();
        if (!guard || revision != _revision) {
            return;
        }
    }
    if (messageChanged) {
        emit statusMessageChanged();
    }
}

void NTRIPManager::_onCorrection(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs,
                                 quint64 attemptId)
{
    if (attemptId != _session.activeAttemptId()) {
        return;
    }
    const QPointer<NTRIPManager> guard(this);
    const auto revision = _revision;
    _stats.recordValidatedFrame(filtered);
    if (!guard || revision != _revision) {
        return;
    }
    if (!filtered) {
        _stats.recordMessage(data.size(), messageId);
        if (!guard || revision != _revision) {
            return;
        }
    }
    emit correctionReceivedAt(data, messageId, filtered, receivedAtMs, attemptId);
}

void NTRIPManager::_onPlaintextCredentialsWarning()
{
    qCWarning(NTRIPManagerLog) << "Credentials sent without TLS encryption — enable TLS in NTRIP settings";
    _setSecurityWarning(tr("Credentials are being sent without TLS encryption."));
}

void NTRIPManager::_setSecurityWarning(const QString& warning)
{
    if (_securityWarning != warning) {
        _securityWarning = warning;
        emit securityWarningChanged();
    }
}

bool NTRIPManager::_isEnabled() const
{
    return _settings && _settings->ntripServerConnectEnabled()->rawValue().toBool();
}

void NTRIPManager::_onSettingChanged()
{
    if (!_settings) {
        return;
    }
    if (!_isEnabled()) {
        stopNTRIP();
        return;
    }
    const auto config = NTRIPTransportConfig::fromSettings(*_settings);
    const bool active =
        _session.state() == NTRIPSession::State::Connecting || _session.state() == NTRIPSession::State::Connected;
    if (!active || config.transportDiffers(_runningConfig)) {
        startNTRIP();
        return;
    }
    if (config.whitelistDiffers(_runningConfig)) {
        _session.setWhitelist(config.whitelist);
    }
    _runningConfig = config;
}
