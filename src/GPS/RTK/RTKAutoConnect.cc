#include "RTKAutoConnect.h"

#include <algorithm>
#include <iterator>

#include <QtCore/QSet>

#include "AutoConnectSettings.h"
#include "GPSRtk.h"
#include "SerialPortManager.h"

RTKAutoConnect::RTKAutoConnect(AutoConnectSettings* settings, GPSRtk* receiver, SerialPortManager* serialPorts,
                               QObject* parent)
    : QObject(parent), _settings(settings), _receiver(receiver), _serialPorts(serialPorts)
{
    if (_receiver) {
        connect(_receiver.data(), &GPSRtk::manualConnectionRequested, this, &RTKAutoConnect::_resetDiscovery);
    }
}

void RTKAutoConnect::_resetDiscovery()
{
    ++_revision;
    _autoConnectedPort.clear();
    _waitingPorts.clear();
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = 1000;
}

void RTKAutoConnect::stop()
{
    const bool hadAutoConnection = !_autoConnectedPort.isEmpty();
    _resetDiscovery();
    if (hadAutoConnection) {
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
    const QPointer<RTKAutoConnect> guard(this);
    const quint64 revision = ++_revision;
    const auto ports = _serialPorts->availablePorts();
    if (!guard || revision != _revision || !_settings || !_receiver || !_serialPorts) {
        return;
    }
    if (!_settings->autoConnectRTKGPS()->rawValue().toBool()) {
        stop();
        return;
    }
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    const QString nmeaPort = _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
                                 ? _settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
                                 : QString();
    if (!_autoConnectedPort.isEmpty() && (!present.contains(_autoConnectedPort) || _autoConnectedPort == nmeaPort)) {
        stop();
        return;
    }
    for (auto it = _waitingPorts.begin(); it != _waitingPorts.end();) {
        it = !present.contains(it.key()) ? _waitingPorts.erase(it) : std::next(it);
    }
    if (_receiver->hasReceiver()) {
        _retryDeadline = QDeadlineTimer::Forever;
        if (_receiver->connected()) {
            _retryDelayMs = 1000;
        }
        return;
    }
    if (!_autoConnectedPort.isEmpty()) {
        if (_retryDeadline.isForever()) {
            _retryDeadline.setRemainingTime(_retryDelayMs);
        }
        if (!_retryDeadline.hasExpired()) {
            return;
        }
        for (const auto& port : ports) {
            if (port.systemLocation == _autoConnectedPort && !port.bootloader &&
                port.boardType == QGCSerialPortInfo::BoardTypeRTKGPS &&
                _serialPorts->canReservePort(port.systemLocation)) {
                _retryDeadline = QDeadlineTimer::Forever;
                _retryDelayMs = std::min(_retryDelayMs * 2, kMaxRetryDelayMs);
                emit connectRequested(port.systemLocation, port.boardName);
                break;
            }
        }
        return;
    }
    QSet<QString> seenDevices;
    for (const auto& port : ports) {
        if (port.boardType != QGCSerialPortInfo::BoardTypeRTKGPS || port.bootloader ||
            port.systemLocation == nmeaPort) {
            _waitingPorts.remove(port.systemLocation);
            continue;
        }
        // A labelled NMEA interface remains a receiver candidate on composite GPS devices.
        const bool duplicate = !port.physicalDeviceId.isEmpty() && seenDevices.contains(port.physicalDeviceId);
        if (!port.physicalDeviceId.isEmpty()) {
            seenDevices.insert(port.physicalDeviceId);
        }
        if ((duplicate && !port.description.contains(QStringLiteral("NMEA"))) ||
            !_serialPorts->canReservePort(port.systemLocation)) {
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
