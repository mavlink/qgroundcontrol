/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/*!
 * @file
 *   @brief Bluetooth connection for unmanned vehicles
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#ifndef BTLINK_H
#define BTLINK_H

#include <QString>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QByteArray>
#include <QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothSocket>

#include "QGCConfig.h"
#include "LinkManager.h"

class QBluetoothDeviceDiscoveryAgent;

class BluetoothData
{
public:
    BluetoothData()
    {
        bits = 0;
    }
    BluetoothData(const BluetoothData& other)
    {
        *this = other;
    }
    bool operator==(const BluetoothData& other)
    {
        return bits == other.bits && name == other.name && address == other.address;
    }
    BluetoothData& operator=(const BluetoothData& other)
    {
        bits = other.bits;
        name = other.name;
        address = other.address;
        return *this;
    }
    quint32 bits;
    QString name;
    QString address;
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
    QString     address                 () { return _device.address; }
    QStringList nameList                () { return _nameList; }
    bool        scanning                () { return _deviceDiscover != NULL; }

    BluetoothData    device             () { return _device; }

    void        setDevName              (const QString& name);

    /// From LinkConfiguration
    LinkType    type                    () { return LinkConfiguration::TypeBluetooth; }
    void        copyFrom                (LinkConfiguration* source);
    void        loadSettings            (QSettings& settings, const QString& root);
    void        saveSettings            (QSettings& settings, const QString& root);
    void        updateSettings          ();
    QString     settingsURL             () { return "BluetoothSettings.qml"; }

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
    void    writeBytes              (const char* data, qint64 length);
    void    deviceConnected         ();
    void    deviceDisconnected      ();
    void    deviceError             (QBluetoothSocket::SocketError error);

protected:

    BluetoothConfiguration*     _config;
    bool                        _connectState;

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    BluetoothLink(BluetoothConfiguration* config);
    ~BluetoothLink();

    // From LinkInterface
    bool _connect               (void);
    void _disconnect            (void);

    bool _hardwareConnect       ();
    void _restartConnection     ();
    void _sendBytes             (const char* data, qint64 size);

private:

    QBluetoothSocket*                   _targetSocket;
    QBluetoothDeviceInfo*               _targetDevice;
    bool                                _running;
};

#endif // BTLINK_H
