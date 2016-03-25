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

/// @file
///     @brief TCP link type for SITL support
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef TCPLINK_H
#define TCPLINK_H

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QHostAddress>
#include <LinkInterface.h>
#include "QGCConfig.h"
#include "LinkManager.h"

// Even though QAbstractSocket::SocketError is used in a signal by Qt, Qt doesn't declare it as a meta type.
// This in turn causes debug output to be kicked out about not being able to queue the signal. We declare it
// as a meta type to silence that.
#include <QMetaType>
#include <QTcpSocket>

//#define TCPLINK_READWRITE_DEBUG   // Use to debug data reads/writes

class TCPLinkUnitTest;

#define QGC_TCP_PORT 5760

class TCPConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:

    Q_PROPERTY(quint16  port    READ port   WRITE setPort   NOTIFY portChanged)
    Q_PROPERTY(QString  host    READ host   WRITE setHost   NOTIFY hostChanged)

    /*!
     * @brief Regular constructor
     *
     * @param[in] name Configuration (user friendly) name
     */
    TCPConfiguration(const QString& name);

    /*!
     * @brief Copy contructor
     *
     * When manipulating data, you create a copy of the configuration, edit it
     * and then transfer its content to the original (using copyFrom() below). Use this
     * contructor to create an editable copy.
     *
     * @param[in] source Original configuration
     */
    TCPConfiguration(TCPConfiguration* source);

    /*!
     * @brief The TCP port
     *
     * @return Port number
     */
    quint16 port   () { return _port; }

    /*!
     * @brief Set the TCP port
     *
     * @param[in] port Port number
     */
    void setPort   (quint16 port);

    /*!
     * @brief The host address
     *
     * @return Host address
     */
    const QHostAddress& address   () { return _address; }
    const QString       host      () { return _address.toString(); }

    /*!
     * @brief Set the host address
     *
     * @param[in] address Host address
     */
    void setAddress (const QHostAddress& address);
    void setHost    (const QString host);

    /// From LinkConfiguration
    LinkType    type            () { return LinkConfiguration::TypeTcp; }
    void        copyFrom        (LinkConfiguration* source);
    void        loadSettings    (QSettings& settings, const QString& root);
    void        saveSettings    (QSettings& settings, const QString& root);
    void        updateSettings  ();
    QString     settingsURL     () { return "TcpSettings.qml"; }

signals:
    void portChanged();
    void hostChanged();

private:
    QHostAddress _address;
    quint16 _port;
};

class TCPLink : public LinkInterface
{
    Q_OBJECT

    friend class TCPLinkUnitTest;
    friend class TCPConfiguration;
    friend class LinkManager;

public:
    QTcpSocket* getSocket(void) { return _socket; }

    void signalBytesWritten(void);

    // LinkInterface methods
    virtual QString getName(void) const;
    virtual bool    isConnected(void) const;
    virtual void    requestReset(void) {};

    // Extensive statistics for scientific purposes
    qint64 getConnectionSpeed() const;
    qint64 getCurrentInDataRate() const;
    qint64 getCurrentOutDataRate() const;

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

private slots:
    // From LinkInterface
    void _writeBytes(const QByteArray data);

public slots:
    void waitForBytesWritten(int msecs);
    void waitForReadyRead(int msecs);

protected slots:
    void _socketError(QAbstractSocket::SocketError socketError);

    // From LinkInterface
    virtual void readBytes(void);

protected:
    // From LinkInterface->QThread
    virtual void run(void);

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    TCPLink(TCPConfiguration* config);
    ~TCPLink();

    // From LinkInterface
    virtual bool _connect(void);
    virtual void _disconnect(void);

    bool _hardwareConnect();
    void _restartConnection();

#ifdef TCPLINK_READWRITE_DEBUG
    void _writeDebugBytes(const QByteArray data);
#endif

    TCPConfiguration* _config;
    QTcpSocket*       _socket;
    bool              _socketIsConnected;

    quint64 _bitsSentTotal;
    quint64 _bitsSentCurrent;
    quint64 _bitsSentMax;
    quint64 _bitsReceivedTotal;
    quint64 _bitsReceivedCurrent;
    quint64 _bitsReceivedMax;
    quint64 _connectionStartTime;
    QMutex  _statisticsMutex;
};

#endif // TCPLINK_H
