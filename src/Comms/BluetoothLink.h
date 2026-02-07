#pragma once

#include "LinkConfiguration.h"
#include "LinkInterface.h"

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothSocket>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothServiceInfo>
#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include <atomic>

class QThread;


/*===========================================================================*/

class BluetoothConfiguration : public LinkConfiguration
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(BluetoothMode mode            READ mode            WRITE setMode           NOTIFY modeChanged)
    Q_PROPERTY(QString       deviceName      READ deviceName                              NOTIFY deviceChanged)
    Q_PROPERTY(QString       address         READ address                                 NOTIFY deviceChanged)
    Q_PROPERTY(QStringList   nameList        READ nameList                                NOTIFY nameListChanged)
    Q_PROPERTY(QVariantList  devicesModel    READ devicesModel                            NOTIFY devicesModelChanged)
    Q_PROPERTY(bool          scanning        READ scanning                                NOTIFY scanningChanged)
    Q_PROPERTY(QString       serviceUuid     READ serviceUuid     WRITE setServiceUuid    NOTIFY serviceUuidChanged)
    Q_PROPERTY(QString       readUuid        READ readUuid        WRITE setReadUuid       NOTIFY readUuidChanged)
    Q_PROPERTY(QString       writeUuid       READ writeUuid       WRITE setWriteUuid      NOTIFY writeUuidChanged)
    Q_PROPERTY(qint16        connectedRssi   READ connectedRssi                           NOTIFY connectedRssiChanged)
    Q_PROPERTY(qint16        selectedRssi    READ selectedRssi                            NOTIFY selectedRssiChanged)
    Q_PROPERTY(bool          adapterAvailable READ isAdapterAvailable                     NOTIFY adapterStateChanged)
    Q_PROPERTY(bool          adapterPoweredOn READ isAdapterPoweredOn                     NOTIFY adapterStateChanged)
    Q_PROPERTY(QString       adapterName     READ getAdapterName                          NOTIFY adapterStateChanged)
    Q_PROPERTY(QString       adapterAddress  READ getAdapterAddress                       NOTIFY adapterStateChanged)
    Q_PROPERTY(QString       hostMode        READ getHostMode                             NOTIFY adapterStateChanged)

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
    Q_INVOKABLE void setDeviceByAddress(const QString &address);

    // Pairing support (Classic Bluetooth only)
    Q_INVOKABLE void requestPairing(const QString &address);
    Q_INVOKABLE void removePairing(const QString &address);
    Q_INVOKABLE QString getPairingStatus(const QString &address) const;

    // Adapter information
    Q_INVOKABLE bool isAdapterAvailable() const;
    Q_INVOKABLE QString getAdapterAddress() const;
    Q_INVOKABLE QString getAdapterName() const;
    Q_INVOKABLE bool isAdapterPoweredOn() const;
    Q_INVOKABLE QVariantList getAllPairedDevices() const;
    Q_INVOKABLE QVariantList getConnectedDevices() const;
    Q_INVOKABLE void powerOnAdapter();
    Q_INVOKABLE void powerOffAdapter();
    Q_INVOKABLE void setAdapterDiscoverable(bool discoverable);
    Q_INVOKABLE QVariantList getAllAvailableAdapters() const;
    Q_INVOKABLE void selectAdapter(const QString &address);
    Q_INVOKABLE QString getHostMode() const;

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
    const QStringList& nameList() const { return _nameList; }
    QVariantList devicesModel() const;
    bool scanning() const;
    qint16 connectedRssi() const { return _connectedRssi; }
    void setConnectedRssi(qint16 rssi);
    qint16 selectedRssi() const;

    // BLE specific settings
    QString serviceUuid() const { return _serviceUuid.toString(); }
    void setServiceUuid(const QString &uuid);
    QString readUuid() const { return _readCharacteristicUuid.toString(); }
    void setReadUuid(const QString &uuid);
    QString writeUuid() const { return _writeCharacteristicUuid.toString(); }
    void setWriteUuid(const QString &uuid);

    const QBluetoothUuid& readCharacteristicUuid() const { return _readCharacteristicUuid; }
    const QBluetoothUuid& writeCharacteristicUuid() const { return _writeCharacteristicUuid; }

    static bool isBluetoothAvailable();

    // Known BLE UART-like service UUIDs
    static inline const QBluetoothUuid NORDIC_UART_SERVICE{QStringLiteral("6e400001-b5a3-f393-e0a9-e50e24dcca9e")};
    static inline const QBluetoothUuid NORDIC_UART_RX_CHAR{QStringLiteral("6e400003-b5a3-f393-e0a9-e50e24dcca9e")};
    static inline const QBluetoothUuid NORDIC_UART_TX_CHAR{QStringLiteral("6e400002-b5a3-f393-e0a9-e50e24dcca9e")};
    static inline const QBluetoothUuid TI_SENSORTAG_SERVICE{QStringLiteral("0000ffe0-0000-1000-8000-00805f9b34fb")};
    static inline const QBluetoothUuid TI_SENSORTAG_CHAR{QStringLiteral("0000ffe1-0000-1000-8000-00805f9b34fb")};

signals:
    void modeChanged();
    void deviceChanged();
    void nameListChanged();
    void devicesModelChanged();
    void scanningChanged();
    void serviceUuidChanged();
    void readUuidChanged();
    void writeUuidChanged();
    void connectedRssiChanged();
    void selectedRssiChanged();
    void errorOccurred(const QString &errorString);
    void adapterStateChanged();
    void pairingStatusChanged();

private slots:
    void _deviceDiscovered(const QBluetoothDeviceInfo &info);
    void _onDiscoveryErrorOccurred(QBluetoothDeviceDiscoveryAgent::Error error);
    void _onDiscoveryFinished();
    void _deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields);

    // QBluetoothLocalDevice slots
    void _onHostModeStateChanged(QBluetoothLocalDevice::HostMode mode);
    void _onDeviceConnected(const QBluetoothAddress &address);
    void _onDeviceDisconnected(const QBluetoothAddress &address);
    void _onPairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing);
    void _onLocalDeviceErrorOccurred(QBluetoothLocalDevice::Error error);

private:
    void _initDeviceDiscoveryAgent();
    void _updateDeviceList();
    void _applySelectedDevice(const QBluetoothDeviceInfo &info);
    bool _createLocalDevice(const QBluetoothAddress &address);
    void _connectLocalDeviceSignals();
    bool _isDeviceCompatibleWithMode(const QBluetoothDeviceInfo &info) const;

    BluetoothMode _mode = BluetoothMode::ModeClassic;
    QBluetoothDeviceInfo _device;
    QList<QBluetoothDeviceInfo> _deviceList;
    QStringList _nameList;
    QPointer<QBluetoothDeviceDiscoveryAgent> _deviceDiscoveryAgent;
    QPointer<QBluetoothLocalDevice> _localDevice;
    qint16 _connectedRssi = 0;

    // BLE specific - Default to Nordic UART Service
    QBluetoothUuid _serviceUuid{NORDIC_UART_SERVICE};
    QBluetoothUuid _readCharacteristicUuid{NORDIC_UART_RX_CHAR};
    QBluetoothUuid _writeCharacteristicUuid{NORDIC_UART_TX_CHAR};
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
    void rssiUpdated(qint16 rssi);

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
    void _onClassicServiceDiscovered(const QBluetoothServiceInfo &serviceInfo);
    void _onClassicServiceDiscoveryFinished();
    void _onClassicServiceDiscoveryCanceled();
    void _onClassicServiceDiscoveryError(QBluetoothServiceDiscoveryAgent::Error error);

    // BLE slots
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

    // Common slots
    void _reconnectTimeout();
    void _serviceDiscoveryTimeout();

private:
    void _setupClassicSocket();
    void _startClassicServiceDiscovery();
    void _setupBleController();
    void _recreateBleController();
    void _setupBleService();
    void _discoverServiceDetails();
    void _enableNotifications();
    void _writeBleData(const QByteArray &data);
    void _writeClassicData(const QByteArray &data);
    void _findCharacteristics();
    void _processNextBleWrite();
    void _clearBleWriteQueue();

    // Cached config values (copied at construction for thread safety)
    const BluetoothConfiguration::BluetoothMode _mode;
    const QBluetoothDeviceInfo _device;
    const QString _serviceUuid;
    const QBluetoothUuid _readCharacteristicUuid;
    const QBluetoothUuid _writeCharacteristicUuid;

    // Classic Bluetooth
    QPointer<QBluetoothSocket> _socket;

    // BLE
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

    // Common
    QPointer<QTimer> _reconnectTimer;
    QPointer<QTimer> _serviceDiscoveryTimer;
    std::atomic<bool> _intentionalDisconnect{false};
    std::atomic<bool> _connected{false};
    std::atomic<int> _reconnectAttempts{0};
    int _consecutiveFailures = 0;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
    static constexpr int MAX_BLE_QUEUE_SIZE = 100;
    static constexpr int RSSI_POLL_INTERVAL_MS = 10000;
    static constexpr int RECONNECT_BASE_INTERVAL_MS = 5000;
    static constexpr int MAX_RECONNECT_INTERVAL_MS = 60000;
    static constexpr int SERVICE_DISCOVERY_TIMEOUT_MS = 30000;
    static constexpr int MAX_CONSECUTIVE_FAILURES = 3;
    QPointer<QBluetoothServiceDiscoveryAgent> _classicDiscovery;
    QBluetoothServiceInfo _classicDiscoveredService;

    // BLE packet size constraints
    static constexpr int BLE_MIN_PACKET_SIZE = 20;
    static constexpr int BLE_MAX_PACKET_SIZE = 512;
    static constexpr int DEFAULT_ATT_MTU = 23;
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
    void _onRssiUpdated(qint16 rssi);

private:
    bool _connect() override;
    void _checkPermission();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);

    BluetoothConfiguration *_bluetoothConfig = nullptr;
    QPointer<BluetoothWorker> _worker;
    QPointer<QThread> _workerThread;
    std::atomic<bool> _disconnectedEmitted{false};
};
