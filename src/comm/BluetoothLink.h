/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/*!
 * @file
 *   @brief Bluetooth connection for unmanned vehicles
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

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
#include "LinkManager.h"

class QBluetoothDeviceDiscoveryAgent;
class QBluetoothServiceDiscoveryAgent;

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

    Q_INVOKABLE void        startScan   ();
    Q_INVOKABLE void        stopScan    ();

    QString     devName                 () { return _device.name; }
    QString     address                 ();
    QStringList nameList                () { return _nameList; }
    bool        scanning                () { return _deviceDiscover != nullptr; }

    BluetoothData    device             () { return _device; }

    void        setDevName              (const QString& name);

    /// From LinkConfiguration
    LinkType    type                    () { return LinkConfiguration::TypeBluetooth; }
    void        copyFrom                (LinkConfiguration* source);
    void        loadSettings            (QSettings& settings, const QString& root);
    void        saveSettings            (QSettings& settings, const QString& root);
    void        updateSettings          ();
    QString     settingsURL             () { return "BluetoothSettings.qml"; }
    QString     settingsTitle           ();

public slots:
    void        deviceDiscovered        (QBluetoothDeviceInfo info);
    void        doneScanning            ();

signals:
    void        newDevice               (QBluetoothDeviceInfo info);
    void        devNameChanged          ();
    void        addressChanged          ();
    void        nameListChanged         ();
    void        scanningChanged         ();

private:
    QBluetoothDeviceDiscoveryAgent*     _deviceDiscover;
    BluetoothData                       _device;
    QStringList                         _nameList;
    QList<BluetoothData>                _deviceList;
};

class BluetoothLink : public LinkInterface
{
    Q_OBJECT

    friend class BluetoothConfiguration;
    friend class LinkManager;

public:
    void    requestReset            () { }
    bool    isConnected             () const;
    QString getName                 () const;

    // Extensive statistics for scientific purposes
    qint64  getConnectionSpeed      () const;
    qint64  getCurrentInDataRate    () const;
    qint64  getCurrentOutDataRate   () const;

    void run();

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool    connect                 (void);
    bool    disconnect              (void);

    LinkConfiguration* getLinkConfiguration() { return _config; }

public slots:
    void    readBytes               ();
    void    deviceConnected         ();
    void    deviceDisconnected      ();
    void    deviceError             (QBluetoothSocket::SocketError error);
#ifdef __ios__
    void    serviceDiscovered       (const QBluetoothServiceInfo &info);
    void    discoveryFinished       ();
#endif

protected:

    BluetoothConfiguration*     _config;
    bool                        _connectState;

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    BluetoothLink(SharedLinkConfigurationPointer& config);
    ~BluetoothLink();

    // From LinkInterface
    bool _connect               (void);
    void _disconnect            (void);

    bool _hardwareConnect       ();
    void _restartConnection     ();

private slots:
    void _writeBytes            (const QByteArray bytes);

private:
    void _createSocket          ();

private:

    QBluetoothSocket*           _targetSocket;
#ifdef __ios__
    QBluetoothServiceDiscoveryAgent* _discoveryAgent;
#endif

    bool                        _shutDown;

};

