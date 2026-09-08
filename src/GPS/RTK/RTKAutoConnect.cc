#include "RTKAutoConnect.h"

#include <QtCore/QUrl>

#include <algorithm>
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
{
    qCDebug(RTKAutoConnectLog) << this;
    if (_receiver) {
        connect(this, &RTKAutoConnect::disconnectRequested, _receiver, &GPSRtk::disconnectGPS);
    }
    connect(this, &RTKAutoConnect::networkActiveChanged, this, &RTKAutoConnect::stateChanged);
    connect(this, &RTKAutoConnect::networkAutoConnectPausedChanged, this, &RTKAutoConnect::stateChanged);
    if (_rtkSettings) {
        for (Fact* fact :
             {_rtkSettings->connectionType(), _rtkSettings->serialDevice(), _rtkSettings->networkReceiverType()}) {
            connect(fact, &Fact::rawValueChanged, this, [this]() {
                stop();
                _serialPaused = false;
                _setNetworkAutoConnectPaused(false);
                emit stateChanged();
            });
        }
    }
    if (_settings) {
        connect(_settings->autoConnectRTKGPS(), &Fact::rawValueChanged, this, [this](const QVariant& enabled) {
            _serialPaused = false;
            if (!enabled.toBool() && !networkActive()) {
                stop();
            }
            emit stateChanged();
        });
        connect(_settings->autoConnectNetworkRTKGPS(), &Fact::rawValueChanged, this, [this](const QVariant& enabled) {
            _setNetworkAutoConnectPaused(false);
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
    return _serialSelected() ? _serialPaused && _settings->autoConnectRTKGPS()->rawValue().toBool()
                             : _networkAutoConnectPaused && _settings->autoConnectNetworkRTKGPS()->rawValue().toBool();
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
    _serialPaused = false;
    _serialRequested = true;
    emit stateChanged();
    update();
    return true;
#else
    return false;
#endif
}

void RTKAutoConnect::disconnectSelected()
{
    _serialPaused = true;
    _setNetworkAutoConnectPaused(true);
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
    _setNetworkAutoConnectPaused(false);
    _networkType = type;
    _networkFactory = std::move(factory);
    _startNetwork();
    emit networkActiveChanged();
    return true;
}

void RTKAutoConnect::disconnectNetwork()
{
    _setNetworkAutoConnectPaused(true);
    if (networkActive()) {
        stop();
    }
}

void RTKAutoConnect::_setNetworkAutoConnectPaused(bool paused)
{
    if (_networkAutoConnectPaused != paused) {
        _networkAutoConnectPaused = paused;
        emit networkAutoConnectPausedChanged();
    }
}

void RTKAutoConnect::stop()
{
    const bool wasActive = active();
    const bool wasNetworkActive = networkActive();
    _serialRequested = false;
    bool hadSession = wasNetworkActive;
    _networkFactory = {};
#ifndef QGC_NO_SERIAL_LINK
    hadSession = hadSession || !_autoConnectedPort.isEmpty();
    _autoConnectedPort.clear();
    _waitingPorts.clear();
#endif
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = 1000;
    if (hadSession) {
        emit disconnectRequested();
    }
    if (wasNetworkActive) {
        emit networkActiveChanged();
    } else if (wasActive) {
        emit stateChanged();
    }
}

void RTKAutoConnect::_startNetwork()
{
    _receiver->connectReceiver(_networkType, _networkFactory);
}

bool RTKAutoConnect::_retryReady()
{
    if (_receiver->hasReceiver()) {
        _retryDeadline = QDeadlineTimer::Forever;
        if (_receiver->connected()) {
            _retryDelayMs = 1000;
        }
        return false;
    }
    if (_retryDeadline.isForever()) {
        _retryDeadline.setRemainingTime(_retryDelayMs);
    }
    return _retryDeadline.hasExpired();
}

void RTKAutoConnect::_retryStarted()
{
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = (std::min) (_retryDelayMs * 2, kMaxRetryDelayMs);
}

void RTKAutoConnect::update()
{
    if (!_receiver) {
        return;
    }
    if (!_serialSelected() && !networkActive() && !_networkAutoConnectPaused && _settings &&
        _settings->autoConnectNetworkRTKGPS()->rawValue().toBool()) {
        connectNetwork();
    }
    if (networkActive()) {
        if (_retryReady()) {
            _retryStarted();
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
