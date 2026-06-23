#pragma once

#include "BluetoothConfiguration.h"

#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothUuid>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include <atomic>

class BluetoothWorker : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothWorker(const BluetoothConfiguration *config, QObject *parent = nullptr);
    ~BluetoothWorker() override;

    bool isConnected() const;

    static BluetoothWorker *create(const BluetoothConfiguration *config, QObject *parent = nullptr);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void rssiUpdated(qint16 rssi);

public slots:
    void setupConnection();
    void connectLink();
    void disconnectLink();
    void writeData(const QByteArray &data);

protected slots:
    void _reconnectTimeout();
    void _serviceDiscoveryTimeout();

protected:
    virtual void onSetupConnection() = 0;
    virtual void onConnectLink() = 0;
    virtual void onDisconnectLink() = 0;
    virtual void onWriteData(const QByteArray &data) = 0;
    virtual void onServiceDiscoveryTimeout() = 0;
    virtual void onResetAfterConsecutiveFailures() = 0;

    const QBluetoothDeviceInfo _device;
    const QBluetoothUuid _serviceUuid;
    const QBluetoothUuid _readCharacteristicUuid;
    const QBluetoothUuid _writeCharacteristicUuid;

    QPointer<QTimer> _reconnectTimer;
    QPointer<QTimer> _serviceDiscoveryTimer;

    std::atomic<bool> _intentionalDisconnect{false};
    std::atomic<bool> _connected{false};
    std::atomic<int> _reconnectAttempts{0};
    int _consecutiveFailures = 0;

    static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
    static constexpr int RECONNECT_BASE_INTERVAL_MS = 5000;
    static constexpr int MAX_RECONNECT_INTERVAL_MS = 60000;
    static constexpr int SERVICE_DISCOVERY_TIMEOUT_MS = 30000;
    static constexpr int MAX_CONSECUTIVE_FAILURES = 3;
};
