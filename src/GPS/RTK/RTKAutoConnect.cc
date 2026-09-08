#include "RTKAutoConnect.h"

#include <QtCore/QUrl>

#include <algorithm>
#include <utility>

#include "AutoConnectSettings.h"
#include "GPSRtk.h"
#include "RTKSettings.h"
#include "TcpGPSTransport.h"

RTKAutoConnect::RTKAutoConnect(GPSRtk* receiver, AutoConnectSettings* settings, RTKSettings* rtkSettings,
                               QObject* parent)
    : QObject(parent), _receiver(receiver), _settings(settings), _rtkSettings(rtkSettings)
{
    if (_receiver) {
        connect(this, &RTKAutoConnect::disconnectRequested, _receiver, &GPSRtk::disconnectGPS);
    }
    if (_settings) {
        connect(_settings->autoConnectNetworkRTKGPS(), &Fact::rawValueChanged, this, [this](const QVariant& enabled) {
            _setNetworkAutoConnectPaused(false);
            if (!enabled.toBool() && networkActive()) {
                stop();
            }
        });
    }
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
    QUrl endpoint;
    endpoint.setScheme(QStringLiteral("tcp"));
    endpoint.setHost(host);
    if (host.isEmpty() || !endpoint.isValid() || endpoint.host().isEmpty() || port < 1 || port > 65535 || type < 0 ||
        type > 3) {
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
    return connectNetwork(receiverType, [host = endpoint.host(), port](const std::atomic_bool& stop) {
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
    const bool wasNetworkActive = networkActive();
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
    if (!networkActive() && !_networkAutoConnectPaused && _settings &&
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
    _updateSerial();
#endif
}
