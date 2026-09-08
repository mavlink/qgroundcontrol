#include "RTKAutoConnect.h"

#include <QtCore/QSet>

#include <iterator>

#include "AutoConnectSettings.h"
#include "GPSRtk.h"

RTKAutoConnect::RTKAutoConnect(AutoConnectSettings* settings, GPSRtk* receiver, SerialPortManager* serialPorts,
                               QObject* parent)
    : QObject(parent), _settings(settings), _receiver(receiver), _serialPorts(serialPorts)
{}

void RTKAutoConnect::stop()
{
    _waitingPorts.clear();
    if (!_autoConnectedPort.isEmpty()) {
        _autoConnectedPort.clear();
        emit disconnectRequested();
    }
}

void RTKAutoConnect::update()
{
    if (!_settings || !_receiver || !_serialPorts) {
        return;
    }
    if (!_settings->autoConnectRTKGPS()->rawValue().toBool()) {
        stop();
        return;
    }
    const auto ports = _serialPorts->availablePorts();
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    const QString nmeaPort = _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
                                 ? _settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
                                 : QString();
    if (!_autoConnectedPort.isEmpty() && (!present.contains(_autoConnectedPort) || _autoConnectedPort == nmeaPort)) {
        stop();
    }
    for (auto it = _waitingPorts.begin(); it != _waitingPorts.end();) {
        it = !present.contains(it.key()) ? _waitingPorts.erase(it) : std::next(it);
    }
    if (!_autoConnectedPort.isEmpty() || _receiver->connected()) {
        return;
    }
    for (const auto& port : ports) {
        if (!port.autoConnectAllowed || port.boardType != QGCSerialPortInfo::BoardTypeRTKGPS || port.bootloader ||
            port.systemLocation == nmeaPort || !_serialPorts->canReservePort(port.systemLocation)) {
            _waitingPorts.remove(port.systemLocation);
            continue;
        }
        auto it = _waitingPorts.find(port.systemLocation);
        if (it == _waitingPorts.end()) {
            _waitingPorts[port.systemLocation].start();
        } else if (it->elapsed() >= _connectDelayMs) {
            _autoConnectedPort = port.systemLocation;
            _waitingPorts.clear();
            emit connectRequested(port.systemLocation, port.boardName);
            return;
        }
    }
}
