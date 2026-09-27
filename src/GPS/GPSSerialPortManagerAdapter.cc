#include "GPSSerialPortManagerAdapter.h"

#include "SerialPortManager.h"

GPSSerialPortManagerAdapter::GPSSerialPortManagerAdapter(SerialPortManager* manager, QObject* parent)
    : GPSSerialPorts(parent)
    , _manager(manager)
{
    if (_manager) {
        (void) connect(_manager, &SerialPortManager::portsEnumerated, this, &GPSSerialPorts::portsEnumerated);
    }
}

QList<GPSSerialPorts::Port> GPSSerialPortManagerAdapter::ports()
{
    if (!_manager) {
        return {};
    }
    const QList<SerialPortManager::Port> available = _manager->availablePorts();
    QList<Port> ports;
    ports.reserve(available.size());
    for (const auto& port : available) {
        ports.append({.systemLocation = port.systemLocation,
                      .boardName = port.boardName,
                      .description = port.description,
                      .physicalDeviceId = port.physicalDeviceId,
                      .rtkReceiver = port.boardType == QGCSerialPortInfo::BoardTypeRTKGPS,
                      .bootloader = port.bootloader});
    }
    return ports;
}

bool GPSSerialPortManagerAdapter::canReserve(const QString& systemLocation) const
{
    return _manager && _manager->canReservePort(systemLocation);
}

GPSSerialPorts::Reservation GPSSerialPortManagerAdapter::reserve(const QString& systemLocation)
{
    return _manager ? _manager->reservePort(systemLocation) : nullptr;
}
