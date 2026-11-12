#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

#include "BluetoothConfiguration.h"
#include "BluetoothWorker.h"
#include "GPSBluetoothLink.h"
#include "GPSReceiverBluetooth.h"

/// A Bluetooth platform without a radio, for the suites of the receiver and its settings: scripted workers, a
/// permission the test gives, and a device discovery the test fills.
namespace GPSTest {

/// What the Bluetooth end of a session asked of its worker, which the test reads after the worker is gone.
struct BluetoothWorkerRecord
{
    QBluetoothDeviceInfo device;
    int connects = 0;
    int disconnects = 0;
    QByteArray written;
};

/// A worker of the application's Bluetooth links without a socket: the test tells how its connection goes, as the
/// classic worker reports it, and reads what it was asked.
class ScriptedBluetoothWorker : public BluetoothWorker
{
public:
    /// Created as the application's workers are: from a link configuration that selects @a device.
    static ScriptedBluetoothWorker* create(const QBluetoothDeviceInfo& device, QObject* parent)
    {
        BluetoothConfiguration configuration(device.name());
        configuration.setDeviceInfo(device);
        return new ScriptedBluetoothWorker(&configuration, parent);
    }

    const std::shared_ptr<BluetoothWorkerRecord> record = std::make_shared<BluetoothWorkerRecord>();
    /// Called for each write instead of reporting it sent.
    std::function<void(const QByteArray&)> onWrite;
    /// Called when the worker is asked to connect.
    std::function<void()> onConnect;

    /// The connection the worker was asked for comes up.
    void accept()
    {
        _connected = true;
        emit connected();
    }

    /// The connection cannot be made: the socket's error, then disconnected(), as before the connection is up.
    void refuse(const QString& error)
    {
        emit errorOccurred(error);
        emit disconnected();
    }

    /// The receiver sends @a data.
    void receive(const QByteArray& data) { emit dataReceived(data); }

    /// Reports @a data as taken by the socket.
    void sent(const QByteArray& data) { emit dataSent(data); }

    /// The connection is lost: the socket's error, then its disconnection.
    void lose(const QString& error)
    {
        emit errorOccurred(error);
        _connected = false;
        emit disconnected();
    }

protected:
    void onSetupConnection() override {}

    void onConnectLink() override
    {
        ++record->connects;
        if (onConnect) {
            onConnect();
        }
    }

    void onDisconnectLink() override
    {
        ++record->disconnects;
        _connected = false;
    }

    void onWriteData(const QByteArray& data) override
    {
        record->written += data;
        if (onWrite) {
            onWrite(data);
        } else {
            emit dataSent(data);
        }
    }

    void onServiceDiscoveryTimeout() override {}

    void onResetAfterConsecutiveFailures() override {}

private:
    ScriptedBluetoothWorker(const BluetoothConfiguration* configuration, QObject* parent)
        : BluetoothWorker(configuration, parent)
    {
        record->device = _device;
    }
};

/// The device discovery the receiver's Bluetooth device picker scans with, as the test fills it: the properties,
/// methods and signal of BluetoothConfiguration that the picker uses.
class ScriptedBluetoothDiscovery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList devicesModel READ devicesModel NOTIFY devicesModelChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
    using QObject::QObject;

    QVariantList devicesModel() const { return _devices; }

    bool scanning() const { return _scanning; }

    Q_INVOKABLE void startScan()
    {
        ++scans;
        _devices.clear();
        emit devicesModelChanged();
        _scanning = true;
        emit scanningChanged();
    }

    Q_INVOKABLE void stopScan()
    {
        if (std::exchange(_scanning, false)) {
            emit scanningChanged();
        }
    }

    /// The scan finds the device @a name at @a address.
    void find(const QString& name, const QString& address)
    {
        _devices.append(QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("address"), address}});
        emit devicesModelChanged();
    }

    /// The scan ends with @a error.
    void fail(const QString& error)
    {
        stopScan();
        emit errorOccurred(error);
    }

    /// How many scans the picker started.
    int scans = 0;

signals:
    void devicesModelChanged();
    void scanningChanged();
    void errorOccurred(const QString& errorString);

private:
    QVariantList _devices;
    bool _scanning = false;
};

/// The Bluetooth side of a receiver without a radio.
struct ScriptedBluetooth
{
    ScriptedBluetooth() = default;
    ScriptedBluetooth(const ScriptedBluetooth&) = delete;
    ScriptedBluetooth& operator=(const ScriptedBluetooth&) = delete;

    GPSBluetoothPlatform platform()
    {
        return {.supported = supported,
                .createDiscovery = [this](QObject* parent) -> QObject* {
                    discovery = new ScriptedBluetoothDiscovery(parent);
                    return discovery;
                },
                .createWorker = [this](const QBluetoothDeviceInfo& device, QObject* parent) -> BluetoothWorker* {
                    auto* const created = ScriptedBluetoothWorker::create(device, parent);
                    created->onConnect = [this, created] {
                        if (onConnect) {
                            onConnect(created);
                        }
                    };
                    workers.append(created);
                    records.append(created->record);
                    return created;
                },
                .requestPermission = [this](QObject*, std::function<void(bool)> answer) { answer(permitted); }};
    }

    /// Creates the side as the receiver's child, for GPSReceiver::Dependencies::bluetooth.
    auto side()
    {
        return [this](QObject* parent) { return new GPSReceiverBluetooth(parent, platform()); };
    }

    /// The worker of the latest connection; null before one was asked for and once its session is over.
    ScriptedBluetoothWorker* worker() const { return workers.isEmpty() ? nullptr : workers.last().data(); }

    /// What the latest connection's worker was asked, also after it is gone.
    const BluetoothWorkerRecord& asked() const { return *records.last(); }

    /// Receivers connect over Bluetooth, as in a build with Bluetooth sockets.
    bool supported = true;
    /// The user's answer to the Bluetooth permission.
    bool permitted = true;
    /// Called when a worker is asked to connect, for a connection that answers by itself.
    std::function<void(ScriptedBluetoothWorker*)> onConnect;
    /// The discovery the receiver created; null until the picker asks for it.
    QPointer<ScriptedBluetoothDiscovery> discovery;
    QList<QPointer<ScriptedBluetoothWorker>> workers;
    QList<std::shared_ptr<BluetoothWorkerRecord>> records;
};

}  // namespace GPSTest
