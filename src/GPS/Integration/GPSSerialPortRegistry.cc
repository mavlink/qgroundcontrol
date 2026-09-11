#include "GPSSerialPortRegistry.h"

#include "QGCLoggingCategory.h"
QGC_LOGGING_CATEGORY(GPSSerialPortRegistryLog, "GPS.Integration.GPSSerialPortRegistry")

namespace {
struct RegistryLease final : GPSSerialDiscovery::Lease
{
    explicit RegistryLease(SerialPortManager::ReservationPtr value)
        : reservation(std::move(value))
    {}

    SerialPortManager::ReservationPtr reservation;
};

GPSSerialDiscovery::ReservationPtr lease(SerialPortManager::ReservationPtr reservation)
{
    return reservation ? std::make_shared<RegistryLease>(std::move(reservation)) : nullptr;
}
}  // namespace

GPSSerialPortRegistry::GPSSerialPortRegistry(SerialPortManager* ports, QObject* parent)
    : GPSSerialDiscovery(parent)
    , _ports(ports)
{
    qCDebug(GPSSerialPortRegistryLog) << this;
    if (_ports)
        connect(_ports, &SerialPortManager::serialPortsChanged, this, &GPSSerialDiscovery::serialPortsChanged);
}

GPSSerialPortRegistry::~GPSSerialPortRegistry()
{
    qCDebug(GPSSerialPortRegistryLog) << this;
}

QList<GPSSerialDiscovery::Port> GPSSerialPortRegistry::availablePorts()
{
    QList<Port> result;
    if (_ports)
        for (const auto& port : _ports->availablePorts())
            result.append({port.systemLocation, port.portName, port.boardName,
                           port.boardType == QGCSerialPortInfo::BoardTypeRTKGPS, port.bootloader,
                           port.autoConnectAllowed});
    return result;
}

GPSSerialDiscovery::ReservationPtr GPSSerialPortRegistry::reservePort(const QString& device)
{
    return _ports ? lease(_ports->reservePort(device)) : nullptr;
}

GPSSerialDiscovery::ReservationPtr GPSSerialPortRegistry::excludeFromAutoConnect(const QString& device)
{
    return _ports ? lease(_ports->excludeFromAutoConnect(device)) : nullptr;
}

bool GPSSerialPortRegistry::canReservePort(const QString& device) const
{
    return _ports && _ports->canReservePort(device);
}

bool GPSSerialPortRegistry::canAutoConnectPort(const QString& device) const
{
    return _ports && _ports->canAutoConnectPort(device);
}

bool GPSSerialPortRegistry::isAutoConnectExcluded(const QString& device) const
{
    return _ports && _ports->isAutoConnectExcluded(device);
}
