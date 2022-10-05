/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QString>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QByteArray>
#include <QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothSocket>
#include <qbluetoothserviceinfo.h>
#include <qbluetoothservicediscoveryagent.h>

#include "QGCConfig.h"
#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QBluetoothDeviceDiscoveryAgent;
class QBluetoothServiceDiscoveryAgent;
class LinkManager;

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
    bool operator==(const BluetoothData& other)
    {
#ifdef __ios__
        return uuid == other.uuid && name == other.name;
#else
        return name == other.name && address == other.address;
#endif
    }
    BluetoothData& operator=(const BluetoothData& other)
    {
        name = other.name;
#ifdef __ios__
        uuid = other.uuid;
#else
        address = other.address;
#endif
        return *this;
    }
    QString name;
#ifdef __ios__
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
    BluetoothConfiguration(BluetoothConfiguration* source);
    ~BluetoothConfiguration();

    Q_PROPERTY(QString      devName     READ devName    WRITE setDevName  NOTIFY devNameChanged)
    Q_PROPERTY(QString      address     READ address                      NOTIFY addressChanged)
    Q_PROPERTY(QStringList  nameList    READ nameList                     NOTIFY nameListChanged)
    Q_PROPERTY(bool         scanning    READ scanning                     NOTIFY scanningChanged)

    Q_INVOKABLE void startScan  (void);
    Q_INVOKABLE void stopScan   (void);

    QString         devName     (void)                  { return _device.name; }
    QString         address     (void);
    QStringList     nameList    (void)                  { return _nameList; }
    bool            scanning    (void)                  { return _deviceDiscover != nullptr; }
    BluetoothData   device      (void)                  { return _device; }
    void            setDevName  (const QString& name);

    /// LinkConfiguration overrides
    LinkType    type            (void) override                                         { return LinkConfiguration::TypeBluetooth; }
    void        copyFrom        (LinkConfiguration* source) override;
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
#ifdef __ios__
    void    serviceDiscovered   (const QBluetoothServiceInfo &info);
    void    discoveryFinished   (void);
#endif

private slots:
    // LinkInterface overrides
    void _writeBytes(const QByteArray bytes) override;

private:

    // LinkInterface overrides
    bool _connect(void) override;

    bool _hardwareConnect   (void);
    void _createSocket      (void);

    QBluetoothSocket*                   _targetSocket    = nullptr;
#ifdef __ios__
    QBluetoothServiceDiscoveryAgent*    _discoveryAgent = nullptr;
#endif
    bool                                _shutDown       = false;
    bool                                _connectState   = false;
};

