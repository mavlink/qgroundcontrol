#pragma once

#include <chrono>

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSBluetoothLink.h"
#include "GPSReceiverWorker.h"

/// The Bluetooth side of the receiver service: whether receivers connect over Bluetooth here, the device discovery
/// their device picker scans with, and the transport of a Bluetooth session. A receiver connects over the serial
/// port profile of classic Bluetooth, by its address; Bluetooth Low Energy has no such profile.
class GPSReceiverBluetooth : public QObject
{
    Q_OBJECT

public:
    /// How long a session waits for its connection: a Bluetooth stack can take this long to give up on a device.
    static constexpr std::chrono::milliseconds OPEN_TIMEOUT{30000};
    /// How long a configuration write waits for the application's thread to hand it to the socket.
    static constexpr std::chrono::milliseconds WRITE_TIMEOUT{5000};

    explicit GPSReceiverBluetooth(QObject* parent = nullptr,
                                  GPSBluetoothPlatform platform = GPSBluetoothPlatform::system());

    /// Receivers connect over Bluetooth here: where Qt has Bluetooth sockets, which it has not on iOS.
    bool supported() const { return _platform.supported; }

    /// The device discovery of the application's Bluetooth links, a BluetoothConfiguration in its classic mode,
    /// whose devicesModel the device picker lists and whose startScan() and stopScan() it calls. Created when first
    /// asked for, and null where receivers do not connect over Bluetooth. Nothing is scanned before the picker asks.
    QObject* discovery();

    /// @a address is a Bluetooth device address as the settings hold it, such as 00:11:22:AA:BB:CC.
    [[nodiscard]] static bool validAddress(const QString& address);

    /// Opens the device at @a address for a session: the transport of a pipe whose other end, a GPSBluetoothLink,
    /// lives on this object's thread and ends with the session. @a name is what the device calls itself. Empty
    /// where receivers do not connect over Bluetooth, and for an address that is not valid.
    [[nodiscard]] GPSReceiverWorker::TransportFactory transportFactory(const QString& address, const QString& name);

private:
    const GPSBluetoothPlatform _platform;
    QObject* _discovery = nullptr;
};
