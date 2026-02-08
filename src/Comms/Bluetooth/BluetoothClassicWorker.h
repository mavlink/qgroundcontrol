#pragma once

#include "BluetoothWorker.h"

#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#include <QtBluetooth/QBluetoothServiceInfo>
#include <QtBluetooth/QBluetoothSocket>

class BluetoothClassicWorker : public BluetoothWorker
{
    Q_OBJECT

public:
    explicit BluetoothClassicWorker(const BluetoothConfiguration *config, QObject *parent = nullptr);
    ~BluetoothClassicWorker() override;

protected:
    void onSetupConnection() override;
    void onConnectLink() override;
    void onDisconnectLink() override;
    void onWriteData(const QByteArray &data) override;
    void onServiceDiscoveryTimeout() override;
    void onResetAfterConsecutiveFailures() override;

private slots:
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketReadyRead();
    void _onSocketBytesWritten(qint64 bytes);
    void _onSocketErrorOccurred(QBluetoothSocket::SocketError socketError);
    void _onClassicServiceDiscovered(const QBluetoothServiceInfo &serviceInfo);
    void _onClassicServiceDiscoveryFinished();
    void _onClassicServiceDiscoveryCanceled();
    void _onClassicServiceDiscoveryError(QBluetoothServiceDiscoveryAgent::Error error);

private:
    void _setupClassicSocket();
    void _startClassicServiceDiscovery();
    void _writeClassicData(const QByteArray &data);

    QPointer<QBluetoothSocket> _socket;
    QPointer<QBluetoothServiceDiscoveryAgent> _classicDiscovery;
    QBluetoothServiceInfo _classicDiscoveredService;

    static inline const QBluetoothUuid SPP_UUID{QBluetoothUuid::ServiceClassUuid::SerialPort};
};
