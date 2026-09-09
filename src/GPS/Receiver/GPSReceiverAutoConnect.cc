#include "GPSReceiverAutoConnect.h"

#include <QtCore/QUrl>

#include <utility>

#include "QGCLoggingCategory.h"
#include "TcpGPSTransport.h"
#include "UdpGPSTransport.h"

QGC_LOGGING_CATEGORY(GPSReceiverAutoConnectLog, "GPS.Receiver.GPSReceiverAutoConnect")

GPSReceiverAutoConnect::GPSReceiverAutoConnect(GPSReceiverSession* receiver, GPSSourceHealth* health, QObject* parent,
                                               GPSConnectionState* sharedState)
    : QObject(parent)
    , _receiver(receiver)
    , _health(health)
    , _ownedConnection(this)
    , _connection(sharedState ? *sharedState : _ownedConnection)
{
    qCDebug(GPSReceiverAutoConnectLog) << this;
    if (_receiver) {
        connect(_receiver, &GPSReceiverSession::stateChanged, this, &GPSReceiverAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSReceiverSession::receiverReady, this, &GPSReceiverAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSReceiverSession::configurationStarted, &_connection, &GPSConnectionState::configuring);
        connect(_receiver, &GPSReceiverSession::connectionError, &_connection, &GPSConnectionState::failed);
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

void GPSReceiverAutoConnect::setConfig(const GPSConnectionConfig& config, bool restart)
{
    _config = config;
    if (restart) {
        const QPointer<GPSReceiverAutoConnect> guard(this);
        stop();
        if (guard) {
            _connection.resetIntent();
        }
    }
}

void GPSReceiverAutoConnect::setAutoConnect(bool enabled)
{
    if (_automatic == enabled) {
        return;
    }
    _automatic = enabled;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _connection.resetIntent();
    if (!guard) {
        return;
    }
    if (!enabled) {
        stop();
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
    const QPointer<GPSReceiverAutoConnect> guard(this);
    stop();
    if (!guard || !_captureConfig()) {
        return false;
    }
    _connection.requestConnect();
    if (!guard) {
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
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _connection.pause();
    if (guard) {
        stop();
    }
}

bool GPSReceiverAutoConnect::connectNetwork()
{
    const auto config = _config;
    if (config.transport == GPSConnectionConfig::Serial || !config.validationError().isEmpty()) {
        return false;
    }
    QUrl endpoint;
    endpoint.setScheme(config.transport == GPSConnectionConfig::Udp ? QStringLiteral("udp") : QStringLiteral("tcp"));
    endpoint.setHost(config.host);
    return connectReceiver(
        config, [config, host = endpoint.host()](const std::atomic_bool& stop) -> std::unique_ptr<GPSTransport> {
            if (config.transport == GPSConnectionConfig::Udp) {
                return std::make_unique<UdpGPSTransport>(host, static_cast<quint16>(config.port), stop,
                                                         static_cast<quint16>(config.localPort));
            }
            return std::make_unique<TcpGPSTransport>(host, static_cast<quint16>(config.port), stop);
        });
}

bool GPSReceiverAutoConnect::connectNetwork(GPSType type, GPSProvider::TransportFactory factory)
{
    auto config = _config;
    config.receiverType = type;
    config.transport = GPSConnectionConfig::Tcp;
    // Injected factories supply the endpoint; these defaults permit transport-independent tests.
    if (config.host.isEmpty()) {
        config.host = QStringLiteral("localhost");
    }
    if (config.port == 0) {
        config.port = 1;
    }
    return connectReceiver(config, std::move(factory));
}

bool GPSReceiverAutoConnect::connectReceiver(const GPSConnectionConfig& config, GPSProvider::TransportFactory factory)
{
    if (!_receiver || !factory || _transportFactory || !config.validationError().isEmpty()) {
        return false;
    }
    const QPointer<GPSReceiverAutoConnect> guard(this);
    stop();
    if (!guard) {
        return false;
    }
    _sessionConfig = config;
    _transportFactory = std::move(factory);
    _connection.requestConnect();
    if (!guard) {
        return false;
    }
    _startReceiver();
    if (guard) {
        emit networkActiveChanged();
    }
    return true;
}

bool GPSReceiverAutoConnect::_captureConfig()
{
    if (const QString error = _config.validationError(); !error.isEmpty()) {
        qCDebug(GPSReceiverAutoConnectLog) << error;
        return false;
    }
    _sessionConfig = _config;
    return true;
}

bool GPSReceiverAutoConnect::networkActive() const
{
    return _transportFactory && _sessionConfig && _sessionConfig->transport != GPSConnectionConfig::Serial;
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
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (_receiver->stopping() && !_receiver->hasReceiver()) {
        _connection.stopping();
    } else if (_connection.state() == GPSConnectionState::Stopping) {
        _connection.stopped();
    }
    if (!guard || !_receiver) {
        return;
    }
    if (_receiver->ready()) {
        _connection.ready();
    } else if (!_receiver->hasReceiver() && !_receiver->stopping() &&
               (_connection.state() == GPSConnectionState::Connecting ||
                _connection.state() == GPSConnectionState::Configuring ||
                _connection.state() == GPSConnectionState::Ready)) {
        _connection.failed();
    }
}

void GPSReceiverAutoConnect::stopAttempt()
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
        if (!guard) {
            return;
        }
        _receiver->stop();
    }
    if (!guard) {
        return;
    }
    if (hadSession) {
        emit disconnectRequested();
    }
    if (!guard) {
        return;
    }
    if (!_receiver || (!_receiver->hasReceiver() && !_receiver->stopping())) {
        _connection.stopped();
    }
    if (guard && wasNetworkActive) {
        emit networkActiveChanged();
    }
}

void GPSReceiverAutoConnect::stop()
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _connection.stop();
    if (guard) {
        stopAttempt();
    }
}

void GPSReceiverAutoConnect::_startReceiver()
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (_sessionConfig && _receiver && !_receiver->hasReceiver() && !_receiver->stopping() &&
        _connection.beginAttempt() && guard && _sessionConfig && _transportFactory && _receiver) {
        _receiver->start(_sessionConfig->receiverType, _transportFactory, _sessionConfig->receiver);
    }
}

bool GPSReceiverAutoConnect::_retryReady()
{
    const QPointer<GPSReceiverAutoConnect> guard(this);
    _updateReceiverState();
    if (!guard || !_receiver || _receiver->hasReceiver() || _receiver->stopping()) {
        return false;
    }
    return _connection.canAttempt();
}

void GPSReceiverAutoConnect::update()
{
    if (!_receiver) {
        return;
    }
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (!_connection.updateIntent(_automatic)) {
        if (guard) {
            stop();
        }
        return;
    }
    if (!guard) {
        return;
    }
    if (_transportFactory) {
        if (_retryReady() && guard) {
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
