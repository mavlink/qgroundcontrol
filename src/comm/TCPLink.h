/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
#include <QTcpSocket>
#include <LinkInterface.h>
#include <configuration.h>

//#define TCPLINK_READWRITE_DEBUG   // Use to debug data reads/writes

class TCPLink : public LinkInterface
{
    Q_OBJECT
    
public:
    TCPLink(QHostAddress hostAddress = QHostAddress::LocalHost, quint16 socketPort = 5760);
    ~TCPLink();
    
    void setHostAddress(QHostAddress hostAddress);
    
    QHostAddress getHostAddress(void) const { return _hostAddress; }
    quint16 getPort(void) const { return _port; }
    QTcpSocket* getSocket(void) { return _socket; }
    
    // LinkInterface methods
    virtual int     getId(void) const;
    virtual QString getName(void) const;
    virtual bool    isConnected(void) const;
    virtual bool    connect(void);
    virtual bool    disconnect(void);
    virtual qint64  bytesAvailable(void);
    virtual void    requestReset(void) {};

    virtual qint64 getNominalDataRate(void) const;
    
public slots:
    void setHostAddress(const QString& hostAddress);
    void setPort(int port);
    
    // From LinkInterface
    virtual void writeBytes(const char* data, qint64 length);

protected slots:
    void _socketError(QAbstractSocket::SocketError socketError);

    // From LinkInterface
    virtual void readBytes(void);

protected:
    // From LinkInterface->QThread
    virtual void run(void);

private:
    void _resetName(void);
	bool _hardwareConnect(void);
#ifdef TCPLINK_READWRITE_DEBUG
    void _writeDebugBytes(const char *data, qint16 size);
#endif

    QString         _name;
    QHostAddress    _hostAddress;
    quint16         _port;
    int             _linkId;
    QTcpSocket*     _socket;
    bool            _socketIsConnected;
    
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
