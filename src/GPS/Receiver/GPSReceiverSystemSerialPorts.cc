#include "GPSReceiverSystemSerialPorts.h"

#include <algorithm>
#include <utility>

#include <QtCore/QSet>

#include "GPSReceiver.h"
#include "GPSReceiverConfig.h"
#include "SerialGPSTransport.h"

GPSReceiverSystemSerialPorts::GPSReceiverSystemSerialPorts(SerialPortManager* manager, QObject* parent)
    : GPSReceiverSerialPorts(parent)
    , _manager(manager)
    , _openDevice([](const QString& device, GPSCancelToken cancelToken) {
        return std::make_unique<SerialGPSTransport>(device, std::move(cancelToken));
    })
{
    if (_manager) {
        connect(_manager, &SerialPortManager::portsEnumerated, this, [this](const QStringList& devices) {
            emit scanned(devices);
            // Just after a scan the manager returns that inventory without scanning again.
            _scannedPorts = _manager->availablePorts();
            refreshEntries();
        });
    }
}

QList<int> GPSReceiverSystemSerialPorts::baudRates() const
{
    QList<int> rates;
    for (const QString& text : SerialPortManager::supportedBaudRates()) {
        const uint32_t rate = text.toUInt();
        if (gpsValidExplicitBaudRate(rate) && !rates.contains(static_cast<int>(rate))) {
            rates.append(static_cast<int>(rate));
        }
    }
    return rates;
}

void GPSReceiverSystemSerialPorts::scanIfDue()
{
    if (_manager) {
        (void) _manager->availablePorts();
    }
}

bool GPSReceiverSystemSerialPorts::available(const QString& device) const
{
    if (device.isEmpty() || !_manager) {
        return false;
    }
    const QList<SerialPortManager::Port> ports = _manager->availablePorts();
    return std::any_of(ports.cbegin(), ports.cend(),
                       [&device](const auto& port) { return port.systemLocation == device && !port.bootloader; });
}

QStringList GPSReceiverSystemSerialPorts::_listedRtkReceivers() const
{
    QStringList receivers;
    if (!_manager) {
        return receivers;
    }
    const QList<SerialPortManager::Port> ports = _manager->availablePorts();
    // Any interface of an excluded device would configure it.
    QSet<QString> excludedDevices;
    for (const auto& port : ports) {
        if (!port.physicalDeviceId.isEmpty() && discoveryExcluded(port.systemLocation)) {
            excludedDevices.insert(port.physicalDeviceId);
        }
    }
    QSet<QString> seenDevices;
    for (const auto& port : ports) {
        if (port.boardType != QGCSerialPortInfo::BoardTypeRTKGPS || port.bootloader ||
            discoveryExcluded(port.systemLocation) || excludedDevices.contains(port.physicalDeviceId)) {
            continue;
        }
        // A labelled NMEA interface remains a receiver candidate on composite GPS devices.
        const bool primary = SerialPortManager::isPrimaryInterface(port, seenDevices);
        if (primary || port.description.contains(QStringLiteral("NMEA"))) {
            receivers.append(port.systemLocation);
        }
    }
    return receivers;
}

QString GPSReceiverSystemSerialPorts::settledRtkReceiver(quint64 nowUs)
{
    QStringList receivers = _listedRtkReceivers();
    receivers.removeIf([this](const QString& device) { return !_manager->canReservePort(device); });
    _rtkReceivers.removeIf([&receivers](const QString& device) { return !receivers.contains(device); });
    for (const QString& device : std::as_const(receivers)) {
        if (_rtkReceivers.settled(device, nowUs)) {
            return device;
        }
    }
    return {};
}

bool GPSReceiverSystemSerialPorts::rtkReceiverKeptClosed() const
{
    // A session of this receiver keeps the other devices closed too, and is not another connection.
    if (_heldHere()) {
        return false;
    }
    const QStringList receivers = _listedRtkReceivers();
    return std::any_of(receivers.cbegin(), receivers.cend(), [this](const QString& device) {
        return !_manager->isPortReserved(device) && !_manager->canReservePort(device);
    });
}

bool GPSReceiverSystemSerialPorts::_heldHere() const
{
    return std::any_of(_claims.cbegin(), _claims.cend(), [](const auto& claim) { return !claim.expired(); });
}

GPSSerialClaim GPSReceiverSystemSerialPorts::claim(const QString& device)
{
    SerialPortManager::ReservationPtr reservation = _manager ? _manager->reservePort(device) : nullptr;
    if (reservation) {
        _claims.insert(device, reservation);
    }
    return reservation;
}

bool GPSReceiverSystemSerialPorts::held(const QString& device) const
{
    return _manager && _manager->isPortReserved(device);
}

GPSReceiverWorker::TransportFactory GPSReceiverSystemSerialPorts::transportFactory(const QString& device,
                                                                                   GPSSerialClaim claim) const
{
    // The session's worker keeps the factory, and with it the claim, until it has stopped.
    return [device, claim = std::move(claim), open = _openDevice](GPSCancelToken cancelToken) {
        return open(device, std::move(cancelToken));
    };
}

void GPSReceiverSystemSerialPorts::refreshEntries()
{
    if (!_manager) {
        return;
    }
    // The marks tell what other connections hold. A session of this receiver keeps the other devices closed too
    // where only one can be open at a time, and marks them no more than its own.
    const bool heldHere = _heldHere();
    QList<GPSSerialPortEntry> entries;
    entries.reserve(_scannedPorts.size());
    for (const auto& port : std::as_const(_scannedPorts)) {
        // The USB product string, else the name of a known board.
        const QString name = port.description.trimmed().isEmpty() ? port.boardName : port.description.trimmed();
        //: %1 is a serial device's name, such as u-blox GNSS receiver; %2 its path, such as /dev/ttyACM0 or COM3
        QString label =
            name.isEmpty() ? port.systemLocation : GPSReceiver::tr("%1 – %2").arg(name, port.systemLocation);
        // A mark comes first: the picker cuts a label that is wider than it is at the end.
        if (_manager->isPortReserved(port.systemLocation)) {
            if (_claims.value(port.systemLocation).expired()) {
                //: %1 is a serial device that another QGroundControl connection, such as a vehicle link, uses
                label = GPSReceiver::tr("(in use) %1").arg(label);
            }
        } else if (!heldHere && !_manager->canReservePort(port.systemLocation)) {
            //: %1 is a serial device that cannot be opened while another QGroundControl connection uses another
            //: serial device, where only one can be open at a time, as with Android's USB serial driver
            label = GPSReceiver::tr("(another is in use) %1").arg(label);
        }
        entries.append({.value = port.systemLocation, .label = label});
    }
    setEntries(std::move(entries));
}
