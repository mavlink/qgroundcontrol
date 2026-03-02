#pragma once

#include "BluetoothConfiguration.h"
#include "LinkInterface.h"

#include <QtCore/QPointer>

#include <atomic>

class BluetoothWorker;
class QThread;

class BluetoothLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    ~BluetoothLink() override;

    bool isConnected() const override;
    void disconnect() override;

private slots:
    void _writeBytes(const QByteArray &bytes) override;
    void _onConnected();
    void _onDisconnected();
    void _onErrorOccurred(const QString &errorString);
    void _onDataReceived(const QByteArray &data);
    void _onDataSent(const QByteArray &data);
    void _onRssiUpdated(qint16 rssi);

private:
    bool _connect() override;
    void _checkPermission();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);

    BluetoothConfiguration *_bluetoothConfig = nullptr;
    QPointer<BluetoothWorker> _worker;
    QPointer<QThread> _workerThread;
    std::atomic<bool> _connectedCache{false};
    std::atomic<bool> _disconnectedEmitted{false};
};
