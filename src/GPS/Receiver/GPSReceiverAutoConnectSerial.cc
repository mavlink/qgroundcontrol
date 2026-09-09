#include <QtCore/QSet>

#include <iterator>

#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "SerialGPSTransport.h"
#include "SerialPortManager.h"

void GPSReceiverAutoConnect::setSerialDiscovery(SerialPortManager* serialPorts)
{
    _serialPorts = serialPorts;
    if (!_serialFactory) {
        _serialFactory = [this](const QString& device) -> GPSProvider::TransportFactory {
            auto reservation = _serialPorts->reservePort(device);
            if (!reservation) {
                return {};
            }
            return [device, reservation](const std::atomic_bool& stop) {
                return std::make_unique<SerialGPSTransport>(device, stop);
            };
        };
    }
}

void GPSReceiverAutoConnect::_updateSerial()
{
    if (!_receiver || !_serialPorts) {
        return;
    }
    const quint64 revision = _commandRevision;
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (!_sessionConfig && !_captureConfig()) {
        return;
    }
    _updateReceiverState();
    if (!guard || revision != _commandRevision || !_receiver || _receiver->stopping() || !_sessionConfig) {
        return;
    }
    const QString selectedDevice = _sessionConfig->endpoint.device;
    const auto ports = _serialPorts->availablePorts();
    const auto eligible = [this, &selectedDevice](const SerialPortManager::Port& port) {
        return port.autoConnectAllowed && !port.bootloader && _serialPorts->canAutoConnectPort(port.systemLocation) &&
               (selectedDevice.isEmpty() ? port.boardType == QGCSerialPortInfo::BoardTypeRTKGPS
                                         : port.systemLocation == selectedDevice);
    };
    const auto request = [this, &selectedDevice, revision](const SerialPortManager::Port& port) {
        const QString name = selectedDevice.isEmpty() ? port.boardName : _sessionConfig->receiverName;
        const GPSType type = selectedDevice.isEmpty()
                                 ? GPSReceiverCapabilities::typeForName(name).value_or(GPSType::u_blox)
                                 : _sessionConfig->driverType;
        auto profile = *_sessionConfig;
        profile.endpoint.device = port.systemLocation;
        profile.endpoint.discoverSerialDevice = false;
        profile.driverType = type;
        profile.receiverName = name;
        const auto config = profile.receiver;
        auto factory = _serialFactory ? _serialFactory(port.systemLocation) : GPSProvider::TransportFactory{};
        const QPointer<GPSReceiverAutoConnect> lifetime(this);
        if (factory && _connection.beginAttempt() && lifetime && revision == _commandRevision) {
            emit connectRequested(port.systemLocation, name, config);
            if (lifetime && revision == _commandRevision && _receiver &&
                _connection.state() == GPSConnectionState::Connecting && _connection.active()) {
                _receiver->start(profile, std::move(factory));
            }
        }
    };
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    if (!_autoConnectedPort.isEmpty() &&
        (!present.contains(_autoConnectedPort) ||
         (!_receiver->hasReceiver() && _serialPorts->isAutoConnectExcluded(_autoConnectedPort)))) {
        // Removal retires the attempt without creating a new manual connection request.
        const auto config = _sessionConfig;
        _stopAttempt(revision);
        if (!guard || revision != _commandRevision || !_connection.active() || _connection.paused()) {
            return;
        }
        _sessionConfig = config;
        _connection.resetRetry();
    }
    for (auto it = _waitingPorts.begin(); it != _waitingPorts.end();) {
        it = !present.contains(it.key()) ? _waitingPorts.erase(it) : std::next(it);
    }
    if (_receiver->hasReceiver() || !_autoConnectedPort.isEmpty()) {
        if (!_retryReady()) {
            return;
        }
        for (const auto& port : ports) {
            if (port.systemLocation == _autoConnectedPort && eligible(port) &&
                _serialPorts->canReservePort(port.systemLocation)) {
                request(port);
                break;
            }
        }
        return;
    }
    for (const auto& port : ports) {
        if (!eligible(port) || !_serialPorts->canReservePort(port.systemLocation)) {
            _waitingPorts.remove(port.systemLocation);
            continue;
        }
        auto it = _waitingPorts.find(port.systemLocation);
        if (it == _waitingPorts.end()) {
            _waitingPorts[port.systemLocation].start();
        } else if (it->elapsed() >= _connectDelayMs) {
            _autoConnectedPort = port.systemLocation;
            _waitingPorts.clear();
            request(port);
            return;
        }
    }
}

void GPSReceiverAutoConnect::setSerialTransportFactory(SerialTransportFactory factory)
{
    _serialFactory = std::move(factory);
}
