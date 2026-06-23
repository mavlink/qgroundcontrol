#pragma once

#include "BluetoothWorker.h"

#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyDescriptor>
#include <QtBluetooth/QLowEnergyService>
#include <QtCore/QQueue>

class BluetoothBleWorker : public BluetoothWorker
{
    Q_OBJECT

public:
    explicit BluetoothBleWorker(const BluetoothConfiguration *config, QObject *parent = nullptr);
    ~BluetoothBleWorker() override;

protected:
    void onSetupConnection() override;
    void onConnectLink() override;
    void onDisconnectLink() override;
    void onWriteData(const QByteArray &data) override;
    void onServiceDiscoveryTimeout() override;
    void onResetAfterConsecutiveFailures() override;

private slots:
    void _onControllerConnected();
    void _onControllerDisconnected();
    void _onControllerErrorOccurred(QLowEnergyController::Error error);
    void _onServiceDiscovered(const QBluetoothUuid &uuid);
    void _onServiceDiscoveryFinished();
    void _onServiceStateChanged(QLowEnergyService::ServiceState state);
    void _onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void _onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void _onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void _onDescriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &value);
    void _onDescriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &value);
    void _onServiceError(QLowEnergyService::ServiceError error);

private:
    void _setupBleController();
    void _recreateBleController();
    void _setupBleService();
    void _discoverServiceDetails();
    void _enableNotifications();
    void _writeBleData(const QByteArray &data);
    void _findCharacteristics();
    void _processNextBleWrite();
    void _clearBleWriteQueue();

    QPointer<QLowEnergyController> _controller;
    QPointer<QLowEnergyService> _service;
    QLowEnergyCharacteristic _readCharacteristic;
    QLowEnergyCharacteristic _writeCharacteristic;
    int _mtu = DEFAULT_ATT_MTU;
    qint16 _rssi = 0;
    QPointer<QTimer> _rssiTimer;
    QQueue<QByteArray> _bleWriteQueue;
    QByteArray _currentBleWrite;
    bool _bleWriteInProgress = false;
    bool _forceRandomAddressType = false;

    static constexpr int MAX_BLE_QUEUE_SIZE = 100;
    static constexpr int RSSI_POLL_INTERVAL_MS = 10000;
    static constexpr int BLE_MIN_PACKET_SIZE = 20;
    static constexpr int BLE_MAX_PACKET_SIZE = 512;
    static constexpr int DEFAULT_ATT_MTU = 23;
};
