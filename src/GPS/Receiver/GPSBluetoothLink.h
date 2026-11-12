#pragma once

#include <functional>
#include <memory>

#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSPipeLink.h"

class BluetoothWorker;
class GPSPipe;

/// What a receiver's Bluetooth connection is built on: the application's Bluetooth links here, and what tests script.
struct GPSBluetoothPlatform
{
    /// The build's: Bluetooth sockets where Qt has them, the device discovery and the worker of the application's
    /// Bluetooth links, and the platform's Bluetooth permission.
    [[nodiscard]] static GPSBluetoothPlatform system();

    /// Receivers connect over the serial port profile here.
    bool supported = false;
    /// Creates the device discovery the receiver's device picker scans with, as a child of @a parent.
    std::function<QObject*(QObject* parent)> createDiscovery;
    /// Creates the worker that connects @a device, as a child of @a parent.
    std::function<BluetoothWorker*(const QBluetoothDeviceInfo& device, QObject* parent)> createWorker;
    /// Asks for the permission to use Bluetooth where the platform has one, and tells @a answer whether it is
    /// granted: at once, or once the user answered, unless @a context is gone by then.
    std::function<void(QObject* context, std::function<void(bool granted)> answer)> requestPermission;
};

/// The Bluetooth end of a receiver session: it owns the connection to the receiver, which a worker of the
/// application's Bluetooth links makes over the serial port profile, and carries its bytes through the session's
/// pipe. It lives on its creator's thread, whose event loop serves the worker.
///
/// The first failure the worker reports ends the link for the session: one before the connection is up refuses the
/// session's open, one after it is the loss of the link. The worker's own reconnection and its search for another
/// service on the device are stopped then, as the receiver's connection policy decides what is tried again.
class GPSBluetoothLink : public GPSPipeLink
{
    Q_OBJECT

public:
    GPSBluetoothLink(std::shared_ptr<GPSPipe> pipe, QBluetoothDeviceInfo device, GPSBluetoothPlatform platform,
                     QObject* parent = nullptr);
    ~GPSBluetoothLink() override;

protected:
    void openLink() override;
    void writeLink(const QByteArray& bytes) override;
    void closeLink() override;

private:
    void _connect();
    /// Ends the link for the session with the reason @a detail gives the user, and stops the worker.
    void _end(const QString& detail);

    const QBluetoothDeviceInfo _device;
    const GPSBluetoothPlatform _platform;
    QPointer<BluetoothWorker> _worker;
    bool _ended = false;
};
