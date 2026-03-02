#pragma once

#include "LinkConfiguration.h"

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothHostInfo>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtCore/QFutureWatcher>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(BluetoothLinkLog)
Q_DECLARE_LOGGING_CATEGORY(BluetoothLinkVerboseLog)

/*===========================================================================*/

class BluetoothConfiguration : public LinkConfiguration
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(BluetoothMode mode            READ mode            WRITE setMode           NOTIFY modeChanged)
    Q_PROPERTY(QString       deviceName      READ deviceName                              NOTIFY deviceChanged)
    Q_PROPERTY(QString       address         READ address                                 NOTIFY deviceChanged)
    Q_PROPERTY(QVariantList  devicesModel    READ devicesModel                            NOTIFY devicesModelChanged)
    Q_PROPERTY(bool          scanning        READ scanning                                NOTIFY scanningChanged)
    Q_PROPERTY(QString       serviceUuid     READ serviceUuid     WRITE setServiceUuid    NOTIFY serviceUuidChanged)
    Q_PROPERTY(QString       readUuid        READ readUuid        WRITE setReadUuid       NOTIFY readUuidChanged)
    Q_PROPERTY(QString       writeUuid       READ writeUuid       WRITE setWriteUuid      NOTIFY writeUuidChanged)
    Q_PROPERTY(qint16        connectedRssi   READ connectedRssi                           NOTIFY connectedRssiChanged)
    Q_PROPERTY(qint16        selectedRssi    READ selectedRssi                            NOTIFY selectedRssiChanged)
    Q_PROPERTY(bool          adapterAvailable READ isAdapterAvailable                     NOTIFY adapterStateChanged)
    Q_PROPERTY(bool          adapterPoweredOn READ isAdapterPoweredOn                     NOTIFY adapterStateChanged)
    Q_PROPERTY(bool          adapterDiscoverable READ isDiscoverable                      NOTIFY adapterStateChanged)
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
    [[nodiscard]] Q_INVOKABLE bool isPaired(const QString &address) const;
    [[nodiscard]] Q_INVOKABLE bool isDiscoverable() const;

    // Adapter information
    [[nodiscard]] Q_INVOKABLE bool isAdapterAvailable() const;
    Q_INVOKABLE QString getAdapterAddress() const;
    Q_INVOKABLE QString getAdapterName() const;
    [[nodiscard]] Q_INVOKABLE bool isAdapterPoweredOn() const;
    Q_INVOKABLE QVariantList getAllPairedDevices() const;
    Q_INVOKABLE QVariantList getConnectedDevices() const;
    Q_INVOKABLE void powerOnAdapter();
    Q_INVOKABLE void powerOffAdapter();
    Q_INVOKABLE void setAdapterDiscoverable(bool discoverable);
    Q_INVOKABLE QVariantList getAllAvailableAdapters();
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

    [[nodiscard]] static bool isBluetoothAvailable();

    // Known BLE UART-like service UUIDs
    static inline const QBluetoothUuid NORDIC_UART_SERVICE{QStringLiteral("6e400001-b5a3-f393-e0a9-e50e24dcca9e")};
    static inline const QBluetoothUuid NORDIC_UART_RX_CHAR{QStringLiteral("6e400003-b5a3-f393-e0a9-e50e24dcca9e")};
    static inline const QBluetoothUuid NORDIC_UART_TX_CHAR{QStringLiteral("6e400002-b5a3-f393-e0a9-e50e24dcca9e")};
    static inline const QBluetoothUuid TI_SENSORTAG_SERVICE{QStringLiteral("0000ffe0-0000-1000-8000-00805f9b34fb")};
    static inline const QBluetoothUuid TI_SENSORTAG_CHAR{QStringLiteral("0000ffe1-0000-1000-8000-00805f9b34fb")};

signals:
    void modeChanged();
    void deviceChanged();
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
    void _refreshAvailableAdaptersAsync();
    void _updateAvailableAdapters(const QList<QBluetoothHostInfo> &hosts);
    bool _ensureLocalDevice();
    void _requestHostMode(QBluetoothLocalDevice::HostMode mode);
    void _updateDeviceList();
    void _applySelectedDevice(const QBluetoothDeviceInfo &info);
    bool _createLocalDevice(const QBluetoothAddress &address);
    void _connectLocalDeviceSignals();
    bool _isDeviceCompatibleWithMode(const QBluetoothDeviceInfo &info) const;

    BluetoothMode _mode = BluetoothMode::ModeClassic;
    QBluetoothDeviceInfo _device;
    QList<QBluetoothDeviceInfo> _deviceList;
    QVariantList _devicesModelCache;
    QPointer<QBluetoothDeviceDiscoveryAgent> _deviceDiscoveryAgent;
    QPointer<QBluetoothLocalDevice> _localDevice;
    QVariantList _availableAdapters;
    QPointer<QFutureWatcher<QList<QBluetoothHostInfo>>> _adapterWatcher;
    bool _adapterEnumerationInProgress = false;
    bool _hostModeRequestPending = false;
    bool _deferredPowerOnFixPending = false;
    QBluetoothLocalDevice::HostMode _requestedHostMode = QBluetoothLocalDevice::HostPoweredOff;
    bool _hasQueuedHostModeRequest = false;
    QBluetoothLocalDevice::HostMode _queuedHostMode = QBluetoothLocalDevice::HostPoweredOff;
    qint16 _connectedRssi = 0;

    // BLE specific - Default to Nordic UART Service
    QBluetoothUuid _serviceUuid{NORDIC_UART_SERVICE};
    QBluetoothUuid _readCharacteristicUuid{NORDIC_UART_RX_CHAR};
    QBluetoothUuid _writeCharacteristicUuid{NORDIC_UART_TX_CHAR};
};
