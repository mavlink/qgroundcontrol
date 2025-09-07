/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothSocket>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QThread;

Q_DECLARE_LOGGING_CATEGORY(BluetoothLinkLog)

/*===========================================================================*/

class BluetoothConfiguration : public LinkConfiguration
{
    Q_OBJECT
    QML_UNCREATABLE("")
    Q_PROPERTY(BluetoothMode mode          READ mode          WRITE setMode         NOTIFY modeChanged)
    Q_PROPERTY(QString       deviceName    READ deviceName                          NOTIFY deviceChanged)
    Q_PROPERTY(QString       address       READ address                             NOTIFY deviceChanged)
    Q_PROPERTY(QStringList   nameList      READ nameList                            NOTIFY nameListChanged)
    Q_PROPERTY(bool          scanning      READ scanning                            NOTIFY scanningChanged)
    Q_PROPERTY(QString       serviceUuid   READ serviceUuid   WRITE setServiceUuid  NOTIFY serviceUuidChanged)

public:
    explicit BluetoothConfiguration(const QString &name, QObject *parent = nullptr);
    explicit BluetoothConfiguration(const BluetoothConfiguration *copy, QObject *parent = nullptr);
    ~BluetoothConfiguration() override;

    enum class BluetoothMode {
        ModeClassic,
        ModeLowEnergy
    };
    Q_ENUM(BluetoothMode)

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan();
    Q_INVOKABLE void setDevice(const QString &name);

    LinkType type() const override { return LinkConfiguration::TypeBluetooth; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("BluetoothSettings.qml"); }
    QString settingsTitle() const override;

    BluetoothMode mode() const { return _mode; }
    void setMode(BluetoothMode mode);

    const QBluetoothDeviceInfo& device() const { return _device; }
    QString deviceName() const { return _device.name(); }
    QString address() const { return _device.address().toString(); }
    QStringList nameList() const { return _nameList; }
    bool scanning() const;

    // BLE specific settings
    QString serviceUuid() const { return _serviceUuid.toString(); }
    void setServiceUuid(const QString &uuid);
    const QBluetoothUuid& readCharacteristicUuid() const { return _readCharacteristicUuid; }
    const QBluetoothUuid& writeCharacteristicUuid() const { return _writeCharacteristicUuid; }
    void setReadCharacteristicUuid(const QBluetoothUuid &uuid) { _readCharacteristicUuid = uuid; }
    void setWriteCharacteristicUuid(const QBluetoothUuid &uuid) { _writeCharacteristicUuid = uuid; }

signals:
    void modeChanged();
    void deviceChanged();
    void nameListChanged();
    void scanningChanged();
    void serviceUuidChanged();
    void errorOccurred(const QString &errorString);

private slots:
    void _deviceDiscovered(const QBluetoothDeviceInfo &info);
    void _onDiscoveryErrorOccurred(QBluetoothDeviceDiscoveryAgent::Error error);
    void _onDiscoveryFinished();

private:
    void _initDeviceDiscoveryAgent();
    void _updateDeviceList();

    BluetoothMode _mode = BluetoothMode::ModeClassic;
    QBluetoothDeviceInfo _device;
    QList<QBluetoothDeviceInfo> _deviceList;
    QStringList _nameList;
    QPointer<QBluetoothDeviceDiscoveryAgent> _deviceDiscoveryAgent;

    // BLE specific
    QBluetoothUuid _serviceUuid;
    QBluetoothUuid _readCharacteristicUuid{QStringLiteral("6e400003-b5a3-f393-e0a9-e50e24dcca9e")};  // Nordic UART RX
    QBluetoothUuid _writeCharacteristicUuid{QStringLiteral("6e400002-b5a3-f393-e0a9-e50e24dcca9e")}; // Nordic UART TX
};

/*===========================================================================*/

class BluetoothWorker : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothWorker(const BluetoothConfiguration *config, QObject *parent = nullptr);
    ~BluetoothWorker() override;

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);

public slots:
    void setupConnection();
    void connectLink();
    void disconnectLink();
    void writeData(const QByteArray &data);

private slots:
    // Classic Bluetooth slots
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketReadyRead();
    void _onSocketBytesWritten(qint64 bytes);
    void _onSocketErrorOccurred(QBluetoothSocket::SocketError socketError);

    // BLE slots
    void _onControllerConnected();
    void _onControllerDisconnected();
    void _onControllerErrorOccurred(QLowEnergyController::Error error);
    void _onServiceDiscovered(const QBluetoothUuid &uuid);
    void _onServiceDiscoveryFinished();
    void _onServiceStateChanged(QLowEnergyService::ServiceState state);
    void _onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void _onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void _onServiceError(QLowEnergyService::ServiceError error);

    // Common slots
    void _reconnectTimeout();

private:
    void _setupClassicSocket();
    void _setupBleController();
    void _setupBleService();
    void _discoverServiceDetails();
    void _enableNotifications();
    void _writeBleData(const QByteArray &data);
    void _writeClassicData(const QByteArray &data);

    const BluetoothConfiguration *_config = nullptr;

    // Classic Bluetooth
    QPointer<QBluetoothSocket> _socket;

    // BLE
    QPointer<QLowEnergyController> _controller;
    QPointer<QLowEnergyService> _service;
    QLowEnergyCharacteristic _readCharacteristic;
    QLowEnergyCharacteristic _writeCharacteristic;
    int _mtu = 23; // default ATT MTU
    qint16 _rssi = 0;

    // Common
    QTimer *_reconnectTimer = nullptr;
    bool _intentionalDisconnect = false;
};

/*===========================================================================*/

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

private:
    bool _connect() override;
    void _checkPermission();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);

    const BluetoothConfiguration *_bluetoothConfig = nullptr;
    BluetoothWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
};
