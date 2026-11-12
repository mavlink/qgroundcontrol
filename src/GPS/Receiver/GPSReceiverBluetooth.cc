#include "GPSReceiverBluetooth.h"

#include <memory>
#include <utility>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>

#include "GPSPipe.h"
#include "GPSPipeTransport.h"

GPSReceiverBluetooth::GPSReceiverBluetooth(QObject* parent, GPSBluetoothPlatform platform)
    : QObject(parent)
    , _platform(std::move(platform))
{}

QObject* GPSReceiverBluetooth::discovery()
{
    if (!_discovery && supported() && _platform.createDiscovery) {
        _discovery = _platform.createDiscovery(this);
    }
    return _discovery;
}

bool GPSReceiverBluetooth::validAddress(const QString& address)
{
    return !QBluetoothAddress(address.trimmed()).isNull();
}

GPSReceiverWorker::TransportFactory GPSReceiverBluetooth::transportFactory(const QString& address, const QString& name)
{
    const QBluetoothAddress device(address.trimmed());
    if (!supported() || device.isNull()) {
        return {};
    }
    auto pipe = std::make_shared<GPSPipe>();
    // The link deletes itself once its session is over; as a child it also ends with the receiver.
    (void) new GPSBluetoothLink(pipe, QBluetoothDeviceInfo(device, name, 0), _platform, this);
    return GPSPipeTransport::factory(std::move(pipe), {.open = OPEN_TIMEOUT, .write = WRITE_TIMEOUT});
}
