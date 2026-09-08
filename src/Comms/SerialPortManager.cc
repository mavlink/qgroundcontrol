#include "SerialPortManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QSet>

#include <iterator>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SerialPortManagerLog, "Comms.SerialPortManager")
Q_APPLICATION_STATIC(SerialPortManager, _serialPortManager)

SerialPortManager::SerialPortManager(QObject* parent, Enumerator enumerator)
    : QObject(parent), _enumerator(enumerator ? std::move(enumerator) : &_enumeratePorts)
{}

SerialPortManager* SerialPortManager::instance()
{
    return _serialPortManager();
}

QList<SerialPortManager::Port> SerialPortManager::_enumeratePorts()
{
    QList<Port> ports;
    QSet<QString> seenDevices;
    for (const QGCSerialPortInfo& info : QGCSerialPortInfo::availablePorts()) {
        Port port;
        if (info.hasVendorIdentifier() && info.hasProductIdentifier() && !info.serialNumber().isEmpty() &&
            info.serialNumber() != QStringLiteral("0")) {
            const QString key = QStringLiteral("%1:%2:%3")
                                    .arg(info.vendorIdentifier())
                                    .arg(info.productIdentifier())
                                    .arg(info.serialNumber());
            // Composite boards expose one MAVLink port; retain explicitly identified NMEA interfaces.
            if (seenDevices.contains(key) && !info.description().contains(QStringLiteral("NMEA"))) {
                port.autoConnectAllowed = false;
            }
            seenDevices.insert(key);
        }
        port.systemLocation = info.systemLocation().trimmed();
        port.portName = info.portName().trimmed();
        (void) info.getBoardInfo(port.boardType, port.boardName);
        port.bootloader = info.isBootloader();
        ports.append(port);
    }
    return ports;
}

QList<SerialPortManager::Port> SerialPortManager::availablePorts()
{
    // Java USB enumeration leaks handles while a port is occupied. Preserve the last
    // snapshot instead of reporting an empty inventory and falsely declaring an unplug.
    if ((_singlePortOnly && anyPortReserved()) || (_scanTimer.isValid() && _scanTimer.elapsed() < 1000)) {
        return _ports;
    }
    _ports = _enumerator();
    _scanTimer.restart();
    return _ports;
}

SerialPortManager::ReservationPtr SerialPortManager::reservePort(const QString& systemLocation)
{
    const QString port = systemLocation.trimmed();
    if (!canReservePort(port)) {
        return {};
    }
    for (auto it = _reservations.begin(); it != _reservations.end();) {
        it = it.value().expired() ? _reservations.erase(it) : std::next(it);
    }
    auto reservation = std::make_shared<const Reservation>(Reservation{port});
    _reservations.insert(port, reservation);
    return reservation;
}

bool SerialPortManager::isPortReserved(const QString& systemLocation) const
{
    return !_reservations.value(systemLocation.trimmed()).expired();
}

bool SerialPortManager::anyPortReserved() const
{
    for (const auto& reservation : _reservations) {
        if (!reservation.expired()) {
            return true;
        }
    }
    return false;
}

bool SerialPortManager::canReservePort(const QString& systemLocation) const
{
    return !systemLocation.trimmed().isEmpty() && !isPortReserved(systemLocation) &&
           !(_singlePortOnly && anyPortReserved());
}
