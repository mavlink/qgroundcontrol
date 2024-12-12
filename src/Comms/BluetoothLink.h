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
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothSocket>
#include <QtBluetooth/QBluetoothUuid>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

#ifdef Q_OS_IOS
    class QBluetoothServiceInfo;
    class QBluetoothServiceDiscoveryAgent;
#endif
class QBluetoothDeviceDiscoveryAgent;

Q_DECLARE_LOGGING_CATEGORY(BluetoothLinkLog)

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
        return ((uuid == other.uuid) && (name == other.name));
#else
        return ((name == other.name) && (address == other.address));
#endif
    }

    BluetoothData &operator=(const BluetoothData &other)
    {
#ifdef Q_OS_IOS
        uuid = other.uuid;
#else
        address = other.address;
#endif
        name = other.name;

        return *this;
    }

#ifdef Q_OS_IOS
    QBluetoothUuid uuid;
#else
    QBluetoothAddress address;
#endif
    QString name;
};

//------------------------------------------------------------------------------

class BluetoothConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QString     deviceName READ deviceName NOTIFY deviceChanged)
    Q_PROPERTY(QString     address    READ address    NOTIFY deviceChanged)
    Q_PROPERTY(QStringList nameList   READ nameList   NOTIFY nameListChanged)
    Q_PROPERTY(bool        scanning   READ scanning   NOTIFY scanningChanged)

public:
    explicit BluetoothConfiguration(const QString &name, QObject *parent = nullptr);
    explicit BluetoothConfiguration(const BluetoothConfiguration *source, QObject *parent = nullptr);
    virtual ~BluetoothConfiguration();

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan();
    Q_INVOKABLE void setDevice(const QString &name);

    BluetoothData device() const { return _device; }
    QString deviceName() const { return _device.name; }
#ifdef Q_OS_IOS
    QString address() const { return QString(); };
#else
    QString address() const { return _device.address.toString(); };
#endif
    QStringList nameList() const { return _nameList; }
    bool scanning() const;

    LinkType type() const override { return LinkConfiguration::TypeBluetooth; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) override;
    QString settingsURL() override { return QStringLiteral("BluetoothSettings.qml"); }
    QString settingsTitle() override;

signals:
    void deviceChanged();
    void nameListChanged();
    void scanningChanged();

private slots:
    void _deviceDiscovered(const QBluetoothDeviceInfo &info);

private:
    void _initDeviceDiscoveryAgent();

    QList<BluetoothData> _deviceList;
    BluetoothData _device{};
    QStringList _nameList;
    QBluetoothDeviceDiscoveryAgent *_deviceDiscoveryAgent = nullptr;
};

//------------------------------------------------------------------------------

class BluetoothLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~BluetoothLink();

    void run() override {};
    bool isConnected() const override;
    void disconnect() override;

private slots:
    void _writeBytes(const QByteArray &bytes) override;
    void _readBytes();
#ifdef Q_OS_IOS
    void _discoveryFinished();
    void _serviceDiscovered(const QBluetoothServiceInfo &info);
#endif

private:
    bool _connect() override;

    const BluetoothConfiguration *_bluetoothConfig = nullptr;
    QBluetoothSocket *_socket = nullptr;
#ifdef Q_OS_IOS
    QBluetoothServiceDiscoveryAgent *_serviceDiscoveryAgent = nullptr;
#endif
};
