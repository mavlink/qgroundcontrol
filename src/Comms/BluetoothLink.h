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
#ifdef Q_OS_IOS
#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#endif
#include <QtBluetooth/QBluetoothServiceInfo>
#include <QtBluetooth/QBluetoothSocket>
#include <QtBluetooth/QBluetoothUuid>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QThread;

Q_DECLARE_LOGGING_CATEGORY(BluetoothLinkLog)

/*===========================================================================*/

struct BluetoothData
{
    BluetoothData() = default;
#ifdef Q_OS_IOS
    BluetoothData(const QString &name_, QBluetoothUuid uuid_)
        : uuid(uuid_)
#else
    BluetoothData(const QString &name_, QBluetoothAddress address_)
        : address(address_)
#endif
        , name(name_)
    {}

    BluetoothData(const BluetoothData &other)
    {
        *this = other;
    }

    bool operator==(const BluetoothData &other) const
    {
#ifdef Q_OS_IOS
        return ((name == other.name) && (uuid == other.uuid));
#else
        return ((name == other.name) && (address == other.address));
#endif
    }

    BluetoothData &operator=(const BluetoothData &other)
    {
        name = other.name;
#ifdef Q_OS_IOS
        uuid = other.uuid;
#else
        address = other.address;
#endif
        return *this;
    }

    QString name;
#ifdef Q_OS_IOS
    QBluetoothUuid uuid;
#else
    QBluetoothAddress address;
#endif
};

/*===========================================================================*/

class BluetoothConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QString     deviceName READ deviceName NOTIFY deviceChanged)
    Q_PROPERTY(QString     address    READ address    NOTIFY deviceChanged)
    Q_PROPERTY(QStringList nameList   READ nameList   NOTIFY nameListChanged)
    Q_PROPERTY(bool        scanning   READ scanning   NOTIFY scanningChanged)

public:
    explicit BluetoothConfiguration(const QString &name, QObject *parent = nullptr);
    explicit BluetoothConfiguration(const BluetoothConfiguration *copy, QObject *parent = nullptr);
    virtual ~BluetoothConfiguration();

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan() const;
    Q_INVOKABLE void setDevice(const QString &name);

    LinkType type() const override { return LinkConfiguration::TypeBluetooth; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("BluetoothSettings.qml"); }
    QString settingsTitle() const override;

    BluetoothData device() const { return _device; }
    QString deviceName() const { return _device.name; }
#ifdef Q_OS_IOS
    QString address() const { return QString(); };
#else
    QString address() const { return _device.address.toString(); };
#endif
    QStringList nameList() const { return _nameList; }
    bool scanning() const;

signals:
    void deviceChanged();
    void nameListChanged();
    void scanningChanged();
    void errorOccurred(const QString &errorString);

private slots:
    void _deviceDiscovered(const QBluetoothDeviceInfo &info);
    void _onSocketErrorOccurred(QBluetoothDeviceDiscoveryAgent::Error error);

private:
    void _initDeviceDiscoveryAgent();

    QList<BluetoothData> _deviceList;
    BluetoothData _device{};
    QStringList _nameList;
    QBluetoothDeviceDiscoveryAgent *_deviceDiscoveryAgent = nullptr;
};

/*===========================================================================*/

class BluetoothWorker : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothWorker(const BluetoothConfiguration *config, QObject *parent = nullptr);
    ~BluetoothWorker();

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);

public slots:
    void setupSocket();
    void connectLink();
    void disconnectLink();
    void writeData(const QByteArray &data);

private slots:
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketReadyRead();
    void _onSocketBytesWritten(qint64 bytes);
    void _onSocketErrorOccurred(QBluetoothSocket::SocketError socketError);
#ifdef Q_OS_IOS
    void _onServiceErrorOccurred(QBluetoothServiceDiscoveryAgent::Error error);
    void _serviceDiscovered(const QBluetoothServiceInfo &info);
    void _discoveryFinished();
#endif

private:
    const BluetoothConfiguration *_config = nullptr;
    QBluetoothSocket *_socket = nullptr;
#ifdef Q_OS_IOS
    QBluetoothServiceDiscoveryAgent *_serviceDiscoveryAgent = nullptr;
#endif
};

/*===========================================================================*/

class BluetoothLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~BluetoothLink();

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
