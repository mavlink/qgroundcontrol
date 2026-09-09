#include "GPSReceiverAutoConnect.h"

#include <utility>

#include "GPSReceiverTransportFactory.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverAutoConnectLog, "GPS.Receiver.GPSReceiverAutoConnect")

GPSReceiverAutoConnect::GPSReceiverAutoConnect(GPSReceiverSession* receiver, GPSSourceHealth* health, QObject* parent)
    : QObject(parent)
    , _receiver(receiver)
    , _health(health)
    , _connection(this)
{
    qCDebug(GPSReceiverAutoConnectLog) << this;
    if (_receiver) {
        connect(_receiver, &GPSReceiverSession::stateChanged, this, &GPSReceiverAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSReceiverSession::attemptChanged, this, &GPSReceiverAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSReceiverSession::connectionErrorDetail, this, &GPSReceiverAutoConnect::stateChanged);
        connect(_receiver, &GPSReceiverSession::capabilitiesUpdated, this, &GPSReceiverAutoConnect::stateChanged);
    }
    connect(&_connection, &GPSConnectionState::changed, this, &GPSReceiverAutoConnect::stateChanged);
    connect(&_connection, &GPSConnectionState::changed, this, &GPSReceiverAutoConnect::networkAutoConnectPausedChanged);
    connect(this, &GPSReceiverAutoConnect::networkActiveChanged, this, &GPSReceiverAutoConnect::stateChanged);
}

GPSReceiverAutoConnect::~GPSReceiverAutoConnect()
{
    qCDebug(GPSReceiverAutoConnectLog) << this;
    disconnect(this, nullptr, nullptr, nullptr);
    if (_receiver) {
        _receiver->disconnect(this);
        _receiver->disconnect(&_connection);
        _receiver->stop();
    }
}

void GPSReceiverAutoConnect::setProfile(const GPSReceiverProfile& profile, bool restart)
{
    const auto normalized = profile.normalized();
    const bool changed = _profile != normalized;
    if (!changed) {
        return;
    }
    _profile = normalized;
    const quint64 revision = ++_commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (restart && changed) {
        _stop(revision);
        if (guard && revision == _commandRevision) {
            _connection.resetIntent();
        }
    }
    if (guard && revision == _commandRevision && changed) {
        emit stateChanged();
    }
}

void GPSReceiverAutoConnect::setAutoConnect(bool enabled)
{
    if (_automatic == enabled) {
        return;
    }
    _automatic = enabled;
    const quint64 revision = ++_commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _connection.resetIntent();
    if (!guard || revision != _commandRevision) {
        return;
    }
    if (!enabled) {
        _stop(revision);
    } else {
        _connection.updateIntent(true);
    }
}

bool GPSReceiverAutoConnect::connectSelected()
{
    if (!_serialSelected()) {
        return connectNetwork();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (!_serialPorts || !_receiver) {
        return false;
    }
    const quint64 revision = ++_commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _stop(revision);
    if (!guard || revision != _commandRevision || !_captureConfig()) {
        return false;
    }
    _connection.requestConnect();
    if (!guard || revision != _commandRevision) {
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
    const quint64 revision = ++_commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _connection.pause();
    if (guard && revision == _commandRevision) {
        _stop(revision);
    }
}

bool GPSReceiverAutoConnect::connectNetwork()
{
    const auto profile = _profile;
    if (profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial || !profile.validationError().isEmpty()) {
        return false;
    }
    return connectReceiver(profile, GPSReceiverTransportFactory::network(profile));
}

bool GPSReceiverAutoConnect::connectNetwork(GPSType type, GPSProvider::TransportFactory factory)
{
    auto profile = _profile;
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
    if (!_receiver || !factory || _transportFactory || !profile.validationError().isEmpty()) {
        return false;
    }
    const auto requestedProfile = profile.normalized();
    const quint64 revision = ++_commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _stop(revision);
    if (!guard || revision != _commandRevision) {
        return false;
    }
    _sessionConfig = requestedProfile;
    _transportFactory = std::move(factory);
    _connection.requestConnect();
    if (!guard || revision != _commandRevision) {
        return false;
    }
    _startReceiver();
    if (guard && revision == _commandRevision) {
        emit networkActiveChanged();
    }
    return true;
}

bool GPSReceiverAutoConnect::_captureConfig()
{
    if (const QString error = _profile.validationError(); !error.isEmpty()) {
        qCDebug(GPSReceiverAutoConnectLog) << error;
        return false;
    }
    _sessionConfig = _profile;
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
    const quint64 revision = _commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (_receiver->stopping() && !_receiver->hasReceiver()) {
        _connection.stopping();
    } else if (_connection.state() == GPSConnectionState::Stopping) {
        _connection.stopped();
    }
    if (!guard || !_receiver || revision != _commandRevision) {
        return;
    }
    switch (_receiver->attempt().phase) {
        case GPSReceiverAttempt::Phase::Configuring:
            _connection.configuring();
            break;
        case GPSReceiverAttempt::Phase::Ready:
            _connection.ready();
            break;
        case GPSReceiverAttempt::Phase::Failed:
            if (!_receiver->stopping() && _handledTerminalAttempt != _receiver->attempt().generation) {
                _handledTerminalAttempt = _receiver->attempt().generation;
                _connection.failed();
            }
            break;
        case GPSReceiverAttempt::Phase::Cancelled:
            if (!_receiver->stopping() && _handledTerminalAttempt != _receiver->attempt().generation) {
                _handledTerminalAttempt = _receiver->attempt().generation;
                _connection.stopped();
            }
            break;
        case GPSReceiverAttempt::Phase::Idle:
        case GPSReceiverAttempt::Phase::Connecting:
            break;
    }
}

void GPSReceiverAutoConnect::stopAttempt()
{
    _stopAttempt(++_commandRevision);
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
        _connection.stopping();
        if (!guard || revision != _commandRevision) {
            return;
        }
        if (_receiver) {
            _receiver->stop();
        }
    }
    if (!guard || revision != _commandRevision) {
        return;
    }
    if (hadSession) {
        emit disconnectRequested();
    }
    if (!guard || revision != _commandRevision) {
        return;
    }
    if (!_receiver || (!_receiver->hasReceiver() && !_receiver->stopping())) {
        _connection.stopped();
    }
    if (guard && revision == _commandRevision && wasNetworkActive) {
        emit networkActiveChanged();
    }
}

void GPSReceiverAutoConnect::stop()
{
    _stop(++_commandRevision);
}

void GPSReceiverAutoConnect::_stop(quint64 revision)
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _connection.stop();
    if (guard && revision == _commandRevision) {
        _stopAttempt(revision);
    }
}

void GPSReceiverAutoConnect::_startReceiver()
{
    const quint64 revision = _commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (_sessionConfig && _receiver && !_receiver->hasReceiver() && !_receiver->stopping() &&
        _connection.beginAttempt() && guard && revision == _commandRevision && _sessionConfig && _transportFactory &&
        _receiver) {
        _receiver->start(*_sessionConfig, _transportFactory);
    }
}

bool GPSReceiverAutoConnect::_retryReady()
{
    const quint64 revision = _commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _updateReceiverState();
    if (!guard || revision != _commandRevision || !_receiver || _receiver->hasReceiver() || _receiver->stopping()) {
        return false;
    }
    return _connection.canAttempt();
}

void GPSReceiverAutoConnect::update()
{
    if (!_receiver) {
        return;
    }
    const quint64 revision = _commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (!_connection.updateIntent(_automatic)) {
        if (guard && revision == _commandRevision) {
            stop();
        }
        return;
    }
    if (!guard || revision != _commandRevision) {
        return;
    }
    if (_transportFactory) {
        if (_retryReady() && guard && revision == _commandRevision) {
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
