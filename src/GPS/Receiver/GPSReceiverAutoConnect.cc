#include "GPSReceiverAutoConnect.h"

#include <QtCore/QScopeGuard>

#include <algorithm>
#include <utility>

#include "GPSReceiverTransportFactory.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSReceiverAutoConnectLog, "GPS.Receiver.GPSReceiverAutoConnect")

GPSReceiverAutoConnect::GPSReceiverAutoConnect(GPSReceiverSession* receiver, GPSSourceHealth* health, QObject* parent,
                                               RuntimeScheduler* scheduler)
    : QObject(parent)
    , _receiver(receiver)
    , _health(health)
    , _control(GPSConnectionControl::NotificationPolicy::Immediate, this, scheduler,
               {.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Serial, .discoverSerialDevice = true},
                .configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure,
                .receiver = {.base = {.surveyInAccMeters = 2.0, .surveyInDurationSecs = 180}}})
{
    qCDebug(GPSReceiverAutoConnectLog) << this;
    if (_receiver) {
        connect(_receiver, &GPSReceiverSession::stateChanged, this, &GPSReceiverAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSReceiverSession::attemptChanged, this, &GPSReceiverAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSReceiverSession::connectionErrorDetail, this, &GPSReceiverAutoConnect::stateChanged);
        connect(_receiver, &GPSReceiverSession::capabilitiesUpdated, this, &GPSReceiverAutoConnect::stateChanged);
    }
    connect(&_control, &GPSConnectionControl::changed, this, &GPSReceiverAutoConnect::stateChanged);
    connect(&_control, &GPSConnectionControl::changed, this, &GPSReceiverAutoConnect::networkAutoConnectPausedChanged);
    connect(this, &GPSReceiverAutoConnect::networkActiveChanged, this, &GPSReceiverAutoConnect::stateChanged);
    connect(this, &GPSReceiverAutoConnect::stateChanged, this, &GPSReceiverAutoConnect::_scheduleUpdate);
}

GPSReceiverAutoConnect::~GPSReceiverAutoConnect()
{
    qCDebug(GPSReceiverAutoConnectLog) << this;
    _control.cancelUpdate();
    blockSignals(true);
    if (_receiver) {
        _receiver->disconnect(this);
        _receiver->disconnect(&_control.connection());
        _receiver->stop();
    }
}

void GPSReceiverAutoConnect::setProfile(const GPSReceiverProfile& profile, bool restart)
{
    const bool changed = _control.changeProfile(profile);
    if (!changed) {
        return;
    }
    _control.setStopped(false);
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (restart || _control.connection().state() == GPSConnectionState::AwaitingChange) {
        _stop(revision);
        if (guard && revision == _control.revision()) {
            _control.connection().resetIntent();
        }
    }
    if (guard && revision == _control.revision() && changed) {
        emit stateChanged();
    }
}

void GPSReceiverAutoConnect::setAutoConnect(bool enabled)
{
    if (!_control.changeAutomatic(enabled)) {
        return;
    }
    _control.setStopped(false);
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _control.connection().resetIntent();
    if (!guard || revision != _control.revision()) {
        return;
    }
    if (!enabled) {
        _stop(revision);
    } else {
        _control.connection().updateIntent(true);
    }
}

bool GPSReceiverAutoConnect::connectSelected()
{
    _control.setStopped(false);
    if (!_serialSelected()) {
        return connectNetwork();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (!_serialPorts || !_receiver) {
        return false;
    }
    const quint64 revision = _control.beginCommand();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _stop(revision);
    if (!guard || revision != _control.revision() || !_captureConfig()) {
        return false;
    }
    _control.connection().requestConnect();
    if (!guard || revision != _control.revision()) {
        return false;
    }
    update();
    return true;
#else
    return false;
#endif
}

void GPSReceiverAutoConnect::disconnectSelected()
{
    const quint64 revision = _control.beginCommand();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _control.connection().pause();
    if (guard && revision == _control.revision()) {
        _stop(revision);
    }
}

bool GPSReceiverAutoConnect::connectNetwork()
{
    const auto profile = _control.profile();
    if (profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial || !profile.validationError().isEmpty()) {
        return false;
    }
    return connectReceiver(profile, GPSReceiverTransportFactory::network(profile));
}

bool GPSReceiverAutoConnect::connectNetwork(GPSType type, GPSProvider::TransportFactory factory)
{
    auto profile = _control.profile();
    profile.driverType = type;
    profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
    // Injected factories supply the endpoint; these defaults permit transport-independent tests.
    if (profile.endpoint.host.isEmpty()) {
        profile.endpoint.host = QStringLiteral("localhost");
    }
    if (profile.endpoint.port == 0) {
        profile.endpoint.port = 1;
    }
    return connectReceiver(profile, std::move(factory));
}

bool GPSReceiverAutoConnect::connectReceiver(const GPSReceiverProfile& profile, GPSProvider::TransportFactory factory)
{
    if (_control.suspended() || !_receiver || !factory || _transportFactory || !profile.validationError().isEmpty()) {
        return false;
    }
    _control.setStopped(false);
    const auto requestedProfile = profile.normalized();
    const quint64 revision = _control.beginCommand();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _stop(revision);
    if (!guard || revision != _control.revision()) {
        return false;
    }
    _sessionConfig = requestedProfile;
    _transportFactory = std::move(factory);
    _control.connection().requestConnect();
    if (!guard || revision != _control.revision()) {
        return false;
    }
    _startReceiver();
    if (guard && revision == _control.revision()) {
        emit networkActiveChanged();
    }
    return true;
}

bool GPSReceiverAutoConnect::_captureConfig()
{
    if (const QString error = _control.profile().validationError(); !error.isEmpty()) {
        qCDebug(GPSReceiverAutoConnectLog) << error;
        return false;
    }
    _sessionConfig = _control.profile();
    return true;
}

bool GPSReceiverAutoConnect::networkActive() const
{
    return _transportFactory && _sessionConfig &&
           _sessionConfig->endpoint.kind != GPSReceiverProfile::Endpoint::Kind::Serial;
}

void GPSReceiverAutoConnect::disconnectNetwork()
{
    if (networkActive() || !_serialSelected()) {
        disconnectSelected();
    }
}

void GPSReceiverAutoConnect::_updateReceiverState()
{
    if (!_receiver) {
        return;
    }
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (_receiver->stopping() && !_receiver->hasReceiver()) {
        _control.connection().stopping();
    } else if (_control.connection().state() == GPSConnectionState::Stopping) {
        _control.connection().stopped();
    }
    if (!guard || !_receiver || revision != _control.revision()) {
        return;
    }
    switch (_receiver->attempt().phase) {
        case GPSReceiverAttempt::Phase::Configuring:
            _control.connection().configuring();
            break;
        case GPSReceiverAttempt::Phase::Ready:
            _control.connection().ready();
            break;
        case GPSReceiverAttempt::Phase::Failed:
            if (!_receiver->stopping() && _handledTerminalAttempt != _receiver->attempt().generation) {
                _handledTerminalAttempt = _receiver->attempt().generation;
                const auto& failure = _receiver->attempt().failure;
                const auto disposition = failure ? failure->retry : GPSRetryDisposition::Retry;
                _control.connection().failed(disposition);
            }
            break;
        case GPSReceiverAttempt::Phase::Cancelled:
            if (!_receiver->stopping() && _handledTerminalAttempt != _receiver->attempt().generation) {
                _handledTerminalAttempt = _receiver->attempt().generation;
                _control.connection().stopped();
            }
            break;
        case GPSReceiverAttempt::Phase::Idle:
        case GPSReceiverAttempt::Phase::Connecting:
            break;
    }
}

void GPSReceiverAutoConnect::stopAttempt()
{
    _stopAttempt(_control.beginCommand());
}

void GPSReceiverAutoConnect::_stopAttempt(quint64 revision)
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    const bool wasNetworkActive = networkActive();
    bool hadSession = static_cast<bool>(_transportFactory);
    _transportFactory = {};
    _sessionConfig.reset();
#ifndef QGC_NO_SERIAL_LINK
    hadSession = hadSession || !_autoConnectedPort.isEmpty();
    _autoConnectedPort.clear();
    _waitingPorts.clear();
#endif
    if (_receiver && (_receiver->hasReceiver() || _receiver->stopping())) {
        _control.connection().stopping();
        if (!guard || revision != _control.revision()) {
            return;
        }
        if (_receiver) {
            _receiver->stop();
        }
    }
    if (!guard || revision != _control.revision()) {
        return;
    }
    if (hadSession) {
        emit disconnectRequested();
    }
    if (!guard || revision != _control.revision()) {
        return;
    }
    if (!_receiver || (!_receiver->hasReceiver() && !_receiver->stopping())) {
        _control.connection().stopped();
    }
    if (guard && revision == _control.revision() && wasNetworkActive) {
        emit networkActiveChanged();
    }
}

void GPSReceiverAutoConnect::stop()
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _control.setStopped(true);
    _stop(_control.beginCommand());
    if (guard) {
        _scheduleUpdate();
    }
}

void GPSReceiverAutoConnect::_stop(quint64 revision)
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _control.connection().stop();
    if (guard && revision == _control.revision()) {
        _stopAttempt(revision);
    }
}

void GPSReceiverAutoConnect::_startReceiver()
{
    if (_sessionConfig && _transportFactory) {
        _startReceiver(*_sessionConfig, _transportFactory);
    }
}

bool GPSReceiverAutoConnect::_startReceiver(const GPSReceiverProfile& profile, GPSProvider::TransportFactory factory,
                                            std::function<void()> admitted)
{
    if (_control.suspended() || !_receiver || _receiver->hasReceiver() || _receiver->stopping()) {
        return false;
    }
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    return _control.startAttempt(
        [this, guard, revision, profile, factory = std::move(factory), admitted = std::move(admitted)]() mutable {
            const auto current = [&]() {
                return guard && revision == _control.revision() && !_control.suspended() && _receiver &&
                       !_receiver->hasReceiver() && !_receiver->stopping() && _control.connection().active() &&
                       _control.connection().state() == GPSConnectionState::Connecting;
            };
            if (!current()) {
                return false;
            }
            if (admitted) {
                admitted();
                if (!current()) {
                    return false;
                }
            }
            _receiver->start(profile, std::move(factory));
            return guard && _receiver &&
                   (_receiver->hasReceiver() || _receiver->stopping() ||
                    _control.connection().state() != GPSConnectionState::Connecting);
        });
}

bool GPSReceiverAutoConnect::_retryReady()
{
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _updateReceiverState();
    if (!guard || revision != _control.revision() || !_receiver || _receiver->hasReceiver() || _receiver->stopping()) {
        return false;
    }
    return _control.connection().canAttempt();
}

void GPSReceiverAutoConnect::update()
{
    _control.setStopped(false);
    if (!_receiver || _control.suspended()) {
        _scheduleUpdate();
        return;
    }
    const auto reschedule = qScopeGuard([guard = QPointer<GPSReceiverAutoConnect>(this)]() {
        if (guard) {
            guard->_scheduleUpdate();
        }
    });
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (!_control.connection().updateIntent(_control.automatic())) {
        if (guard && revision == _control.revision()) {
            stop();
        }
        return;
    }
    if (!guard || revision != _control.revision()) {
        return;
    }
    if (_transportFactory) {
        if (_retryReady() && guard && revision == _control.revision()) {
            _startReceiver();
        }
        return;
    }
    if (!_serialSelected()) {
        connectNetwork();
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    _updateSerial();
#endif
}

QString GPSReceiverAutoConnect::errorDetail() const
{
    return _receiver ? _receiver->errorDetail() : QString();
}

void GPSReceiverAutoConnect::setSuspended(bool suspended)
{
    if (!_control.changeSuspended(suspended)) {
        return;
    }
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (suspended && _receiver && !_receiver->hasReceiver() && !_receiver->stopping() &&
        _control.connection().state() == GPSConnectionState::Connecting) {
        _control.connection().stopped();
    }
    if (guard) {
        _scheduleUpdate();
    }
}

void GPSReceiverAutoConnect::_scheduleUpdate()
{
    _control.cancelUpdate();
    if (!_receiver || !(_sessionConfig ? *_sessionConfig : _control.profile()).validationError().isEmpty()) {
        return;
    }
    qint64 discoveryPollMs = -1;
    if (_serialSelected() && !_transportFactory) {
#ifdef QGC_NO_SERIAL_LINK
        return;
#else
        if (!_serialPorts) {
            return;
        }
        discoveryPollMs = 1000;
#endif
    }
    _control.scheduleUpdate(!_receiver->hasReceiver() && !_receiver->stopping(), discoveryPollMs, true,
                            [this]() { update(); });
}
