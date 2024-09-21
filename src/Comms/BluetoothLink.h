/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothSocket>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

#ifdef Q_OS_IOS
    class QBluetoothServiceInfo;
    class QBluetoothServiceDiscoveryAgent;
#endif
class QBluetoothDeviceDiscoveryAgent;

class BluetoothData
{
public:
    BluetoothData()
    {
    }
    BluetoothData(const BluetoothData& other)
    {
        *this = other;
    }
    bool operator==(const BluetoothData& other) const
    {
#ifdef Q_OS_IOS
        return uuid == other.uuid && name == other.name;
#else
        return name == other.name && address == other.address;
#endif
    }
    BluetoothData& operator=(const BluetoothData& other)
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
    QString address;
#endif
};

class BluetoothConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:

    BluetoothConfiguration(const QString& name);
    BluetoothConfiguration(const BluetoothConfiguration* source);
    ~BluetoothConfiguration();

    Q_PROPERTY(QString      devName     READ devName    WRITE setDevName  NOTIFY devNameChanged)
    Q_PROPERTY(QString      address     READ address                      NOTIFY addressChanged)
    Q_PROPERTY(QStringList  nameList    READ nameList                     NOTIFY nameListChanged)
    Q_PROPERTY(bool         scanning    READ scanning                     NOTIFY scanningChanged)

    Q_INVOKABLE void startScan  (void);
    Q_INVOKABLE void stopScan   (void);

    QString         devName     (void) const                 { return _device.name; }
    QString         address     (void) const;
    QStringList     nameList    (void) const                  { return _nameList; }
    bool            scanning    (void) const                 { return _deviceDiscover != nullptr; }
    BluetoothData   device      (void) const                  { return _device; }
    void            setDevName  (const QString& name);

    /// LinkConfiguration overrides
    LinkType    type            (void) const override                                         { return LinkConfiguration::TypeBluetooth; }
    void        copyFrom        (const LinkConfiguration* source) override;
    void        loadSettings    (QSettings& settings, const QString& root) override;
    void        saveSettings    (QSettings& settings, const QString& root) override;
    QString     settingsURL     (void) override                                         { return "BluetoothSettings.qml"; }
    QString     settingsTitle   (void) override;

public slots:
    void deviceDiscovered   (QBluetoothDeviceInfo info);
    void doneScanning       (void);

signals:
    void newDevice      (QBluetoothDeviceInfo info);
    void devNameChanged (void);
    void addressChanged (void);
    void nameListChanged(void);
    void scanningChanged(void);

private:
    QBluetoothDeviceDiscoveryAgent* _deviceDiscover = nullptr;
    BluetoothData                   _device;
    QStringList                     _nameList;
    QList<BluetoothData>            _deviceList;
};

class BluetoothLink : public LinkInterface
{
    Q_OBJECT

public:
    BluetoothLink(SharedLinkConfigurationPtr& config);
    virtual ~BluetoothLink();

    // Overrides from QThread
    void run(void) override;

    // LinkConfiguration overrides
    bool isConnected(void) const override;
    void disconnect (void) override;

public slots:
    void    readBytes           (void);
    void    deviceConnected     (void);
    void    deviceDisconnected  (void);
    void    deviceError         (QBluetoothSocket::SocketError error);
#ifdef Q_OS_IOS
    void    serviceDiscovered   (const QBluetoothServiceInfo &info);
    void    discoveryFinished   (void);
#endif

private slots:
    // LinkInterface overrides
    void _writeBytes(const QByteArray &bytes) override;

private:

    // LinkInterface overrides
    bool _connect(void) override;

    bool _hardwareConnect   (void);
    void _createSocket      (void);

    BluetoothConfiguration*             _bluetoothConfig;
    QBluetoothSocket*                   _targetSocket    = nullptr;
#ifdef Q_OS_IOS
    QBluetoothServiceDiscoveryAgent*    _discoveryAgent = nullptr;
#endif
    bool                                _shutDown       = false;
    bool                                _connectState   = false;
};

