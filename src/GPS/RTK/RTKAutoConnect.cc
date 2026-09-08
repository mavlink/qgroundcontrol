#include "RTKAutoConnect.h"

#include <QtCore/QUrl>

#include <utility>

#include "AutoConnectSettings.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "TcpGPSTransport.h"
#include "UdpGPSTransport.h"

QGC_LOGGING_CATEGORY(RTKAutoConnectLog, "GPS.RTK.RTKAutoConnect")

RTKAutoConnect::RTKAutoConnect(GPSRtk* receiver, AutoConnectSettings* settings, RTKSettings* rtkSettings,
                               QObject* parent)
    : QObject(parent)
    , _receiver(receiver)
    , _settings(settings)
    , _rtkSettings(rtkSettings)
    , _connection(this)
{
    qCDebug(RTKAutoConnectLog) << this;
    if (_receiver) {
        connect(this, &RTKAutoConnect::disconnectRequested, _receiver, &GPSRtk::disconnectGPS);
        connect(_receiver, &GPSRtk::receiverStateChanged, this, &RTKAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSRtk::connectedChanged, this, &RTKAutoConnect::_updateReceiverState);
        connect(_receiver, &GPSRtk::configurationStarted, &_connection, &GPSConnectionState::configuring);
        connect(_receiver, &GPSRtk::connectionFailed, &_connection, &GPSConnectionState::failed);
    }
    connect(&_connection, &GPSConnectionState::changed, this, &RTKAutoConnect::stateChanged);
    connect(&_connection, &GPSConnectionState::changed, this, &RTKAutoConnect::networkAutoConnectPausedChanged);
    connect(this, &RTKAutoConnect::networkActiveChanged, this, &RTKAutoConnect::stateChanged);
    if (_rtkSettings) {
        for (Fact* fact :
             {_rtkSettings->connectionType(), _rtkSettings->serialDevice(), _rtkSettings->networkReceiverType()}) {
            connect(fact, &Fact::rawValueChanged, this, [this]() {
                stop();
                _connection.resetIntent();
                emit stateChanged();
            });
        }
    }
    if (_settings) {
        connect(_settings->autoConnectRTKGPS(), &Fact::rawValueChanged, this, [this](const QVariant& enabled) {
            if (_serialSelected()) {
                _connection.resetIntent();
                _connection.updateIntent(enabled.toBool());
            }
            if (!enabled.toBool() && !networkActive()) {
                stop();
            }
            emit stateChanged();
        });
        connect(_settings->autoConnectNetworkRTKGPS(), &Fact::rawValueChanged, this, [this](const QVariant& enabled) {
            if (!_serialSelected()) {
                _connection.resetIntent();
                _connection.updateIntent(enabled.toBool());
            }
            if (!enabled.toBool() && networkActive()) {
                stop();
            }
        });
    }
}

RTKAutoConnect::~RTKAutoConnect()
{
    qCDebug(RTKAutoConnectLog) << this;
}

bool RTKAutoConnect::_serialSelected() const
{
    return !_rtkSettings || _rtkSettings->connectionType()->rawValue().toInt() == RTKSettings::Serial;
}

bool RTKAutoConnect::autoConnectPaused() const
{
    if (!_settings) {
        return false;
    }
    return _connection.paused() && (_serialSelected() ? _settings->autoConnectRTKGPS()->rawValue().toBool()
                                                      : _settings->autoConnectNetworkRTKGPS()->rawValue().toBool());
}

bool RTKAutoConnect::connectSelected()
{
    if (!_serialSelected()) {
        return connectNetwork();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (!_serialPorts || !_receiver) {
        return false;
    }
    stop();
    _connection.requestConnect();
    emit stateChanged();
    update();
    return true;
#else
    return false;
#endif
}

void RTKAutoConnect::disconnectSelected()
{
    _connection.pause();
    stop();
    emit stateChanged();
}

bool RTKAutoConnect::connectNetwork()
{
    if (!_rtkSettings) {
        return false;
    }
    auto* settings = _rtkSettings;
    const QString host = settings->networkBaseHost()->rawValue().toString().trimmed();
    const int port = settings->networkBasePort()->rawValue().toInt();
    const int type = settings->networkReceiverType()->rawValue().toInt();
    const int connection = settings->connectionType()->rawValue().toInt();
    const int localPort = settings->udpLocalPort()->rawValue().toInt();
    const bool udp = connection == RTKSettings::Udp;
    QUrl endpoint;
    endpoint.setScheme(udp ? QStringLiteral("udp") : QStringLiteral("tcp"));
    endpoint.setHost(host);
    if (host.isEmpty() || !endpoint.isValid() || endpoint.host().isEmpty() || port < 1 || port > 65535 || type < 0 ||
        type > 3 || connection < RTKSettings::Serial || connection > RTKSettings::Udp ||
        (udp && (localPort < 0 || localPort > 65535))) {
        return false;
    }

    GPSType receiverType = GPSType::u_blox;
    switch (type) {
        case 0:
            receiverType = GPSType::u_blox;
            break;
        case 1:
            receiverType = GPSType::trimble;
            break;
        case 2:
            receiverType = GPSType::septentrio;
            break;
        case 3:
            receiverType = GPSType::femto;
            break;
    }
    return connectNetwork(
        receiverType,
        [host = endpoint.host(), port, udp, localPort](const std::atomic_bool& stop) -> std::unique_ptr<GPSTransport> {
            if (udp) {
                return std::make_unique<UdpGPSTransport>(host, static_cast<quint16>(port), stop,
                                                         static_cast<quint16>(localPort));
            }
            return std::make_unique<TcpGPSTransport>(host, static_cast<quint16>(port), stop);
        });
}

bool RTKAutoConnect::connectNetwork(GPSType type, GPSProvider::TransportFactory factory)
{
    if (!_receiver || !factory || networkActive()) {
        return false;
    }
    stop();
    _connection.requestConnect();
    _networkType = type;
    _networkFactory = std::move(factory);
    _startNetwork();
    emit networkActiveChanged();
    return true;
}

void RTKAutoConnect::disconnectNetwork()
{
    if (!networkActive() && _serialSelected()) {
        return;
    }
    _connection.pause();
    if (networkActive()) {
        stop();
    }
}

void RTKAutoConnect::_updateReceiverState()
{
    if (!_receiver) {
        return;
    }
    if (_receiver->stopping() && !_receiver->hasReceiver()) {
        _connection.stopping();
    } else if (_connection.state() == GPSConnectionState::Stopping) {
        _connection.stopped();
    }
    if (_receiver->connected()) {
        _connection.ready();
    } else if (!_receiver->hasReceiver() && !_receiver->stopping() &&
               (_connection.state() == GPSConnectionState::Connecting ||
                _connection.state() == GPSConnectionState::Configuring ||
                _connection.state() == GPSConnectionState::Ready)) {
        _connection.failed();
    }
}

void RTKAutoConnect::stop()
{
    const bool wasActive = active();
    const bool wasNetworkActive = networkActive();
    _connection.stop();
    bool hadSession = wasNetworkActive;
    _networkFactory = {};
#ifndef QGC_NO_SERIAL_LINK
    hadSession = hadSession || !_autoConnectedPort.isEmpty();
    _autoConnectedPort.clear();
    _waitingPorts.clear();
#endif

    if (hadSession) {
        emit disconnectRequested();
    }
    _updateReceiverState();
    if (!_receiver || (!_receiver->hasReceiver() && !_receiver->stopping())) {
        _connection.stopped();
    }
    if (wasNetworkActive) {
        emit networkActiveChanged();
    } else if (wasActive) {
        emit stateChanged();
    }
}

void RTKAutoConnect::_startNetwork()
{
    if (!_receiver->hasReceiver() && !_receiver->stopping() && _connection.beginAttempt()) {
        _receiver->connectReceiver(_networkType, _networkFactory);
    }
}

bool RTKAutoConnect::_retryReady()
{
    _updateReceiverState();
    if (_receiver->hasReceiver() || _receiver->stopping()) {
        return false;
    }
    if (_connection.state() == GPSConnectionState::Connecting ||
        _connection.state() == GPSConnectionState::Configuring || _connection.state() == GPSConnectionState::Ready) {
        _connection.failed();
    }
    return _connection.canAttempt();
}

void RTKAutoConnect::update()
{
    if (!_receiver) {
        return;
    }
    if (!_serialSelected() && !networkActive() && !_connection.paused() && _settings &&
        _settings->autoConnectNetworkRTKGPS()->rawValue().toBool()) {
        connectNetwork();
    }
    if (networkActive()) {
        if (!_connection.updateIntent(_settings && _settings->autoConnectNetworkRTKGPS()->rawValue().toBool())) {
            stop();
            return;
        }
        if (_retryReady()) {
            _startNetwork();
        }
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_serialSelected()) {
        _updateSerial();
    }
#endif
}

GPSSourceHealth* RTKAutoConnect::health() const
{
    return _receiver ? _receiver->health() : nullptr;
}
