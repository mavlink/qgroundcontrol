#include <QtCore/QSet>

#include <algorithm>
#include <iterator>

#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "GPSSerialDiscovery.h"
#include "SerialGPSTransport.h"

void GPSReceiverAutoConnect::setSerialDiscovery(GPSSerialDiscovery* serialPorts)
{
    if (_serialPorts) {
        _serialPorts->disconnect(this);
    }
    _serialPorts = serialPorts;
    if (_serialPorts) {
        connect(_serialPorts, &GPSSerialDiscovery::serialPortsChanged, this, &GPSReceiverAutoConnect::_scheduleUpdate);
    }
    _scheduleUpdate();
    if (!_serialFactory) {
        _serialFactory = [this](const QString& device) -> GPSProvider::TransportFactory {
            auto reservation = _serialPorts ? _serialPorts->reservePort(device) : nullptr;
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
    const quint64 revision = _control.revision();
    const QPointer<GPSReceiverAutoConnect> guard(this);
    if (!_sessionConfig && !_captureConfig()) {
        return;
    }
    _updateReceiverState();
    if (!guard || revision != _control.revision() || !_receiver || _receiver->stopping() || !_sessionConfig) {
        return;
    }
    const QString selectedDevice = _sessionConfig->endpoint.device;
    const auto ports = _serialPorts->availablePorts();
    if (!guard || revision != _control.revision() || !_serialPorts) {
        return;
    }
    const auto eligible = [this, &selectedDevice](const GPSSerialDiscovery::Port& port) {
        return port.autoConnectAllowed && !port.bootloader && _serialPorts->canAutoConnectPort(port.systemLocation) &&
               (selectedDevice.isEmpty()
                    ? (port.receiver && GPSReceiverCapabilities::typeForName(port.boardName).has_value())
                    : port.systemLocation == selectedDevice);
    };
    const auto request = [this, &selectedDevice, guard, revision](const GPSSerialDiscovery::Port& port) {
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
        if (guard && revision == _control.revision() && factory) {
            _startReceiver(profile, std::move(factory), [this, device = port.systemLocation, name, config]() {
                emit connectRequested(device, name, config);
            });
        }
    };
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    const bool identityChanged =
        _control.profile().endpoint.discoverSerialDevice &&
        _receiver->profile().endpoint.device == _autoConnectedPort &&
        std::any_of(ports.cbegin(), ports.cend(), [this](const auto& port) {
            return port.systemLocation == _autoConnectedPort && port.boardName != _receiver->profile().receiverName;
        });
    if (!_autoConnectedPort.isEmpty() &&
        (identityChanged || !present.contains(_autoConnectedPort) ||
         (!_receiver->hasReceiver() && _serialPorts->isAutoConnectExcluded(_autoConnectedPort)))) {
        // Removal retires the attempt without creating a new manual connection request.
        const auto config = _sessionConfig;
        _stopAttempt(revision);
        if (!guard || revision != _control.revision() || !_control.connection().active() ||
            _control.connection().paused()) {
            return;
        }
        _sessionConfig = config;
        _control.connection().resetRetry();
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
            _waitingPorts[port.systemLocation] = _control.scheduler()->nowMs();
        } else if (_control.scheduler()->nowMs() - it.value() >= _connectDelayMs) {
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
