#include <QtCore/QSet>

#include <iterator>

#include "AutoConnectSettings.h"
#include "GPSRtk.h"
#include "RTKAutoConnect.h"
#include "SerialPortManager.h"

RTKAutoConnect::RTKAutoConnect(AutoConnectSettings* settings, GPSRtk* receiver, SerialPortManager* serialPorts,
                               QObject* parent)
    : RTKAutoConnect(receiver, settings, nullptr, parent)
{
    setSerialDiscovery(serialPorts);
}

void RTKAutoConnect::setSerialDiscovery(SerialPortManager* serialPorts)
{
    _serialPorts = serialPorts;
}

void RTKAutoConnect::_updateSerial()
{
    if (!_settings || !_receiver || !_serialPorts) {
        return;
    }
    if (!_connection.updateIntent(_settings->autoConnectRTKGPS()->rawValue().toBool())) {
        stop();
        return;
    }
    if (!_sessionConfig && !_captureConfig()) {
        return;
    }
    _updateReceiverState();
    if (_receiver->stopping()) {
        return;
    }
    const QString selectedDevice = _sessionConfig->device;
    const auto ports = _serialPorts->availablePorts();
    const auto eligible = [this, &selectedDevice](const SerialPortManager::Port& port) {
        return port.autoConnectAllowed && !port.bootloader && _serialPorts->canAutoConnectPort(port.systemLocation) &&
               (selectedDevice.isEmpty() ? port.boardType == QGCSerialPortInfo::BoardTypeRTKGPS
                                         : port.systemLocation == selectedDevice);
    };
    const auto request = [this, &selectedDevice](const SerialPortManager::Port& port) {
        const QString type = selectedDevice.isEmpty() || !_rtkSettings ? port.boardName : _sessionConfig->receiverName;
        const auto config = _sessionConfig->receiver;
        if (_connection.beginAttempt()) {
            emit connectRequested(port.systemLocation, type, config);
        }
    };
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    if (!_autoConnectedPort.isEmpty() &&
        (!present.contains(_autoConnectedPort) ||
         (!_receiver->hasReceiver() && _serialPorts->isAutoConnectExcluded(_autoConnectedPort)))) {
        // Preserve a manual connection request while the device is temporarily unavailable.
        const auto config = _sessionConfig;
        stop();
        _sessionConfig = config;
        _connection.requestConnect();
        emit stateChanged();
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
