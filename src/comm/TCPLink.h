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

/**
 * @file
 *   @brief TCP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

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
    
    void requestReset() { }
    
    bool isConnected() const;
    qint64 bytesAvailable();
    int getPort() const {
        return port;
    }
    QHostAddress getHostAddress() const {
        return host;
    }
    
    QString getName() const;
    int getBaudRate() const;
    int getBaudRateType() const;
    int getFlowType() const;
    int getParityType() const;
    int getDataBitsType() const;
    int getStopBitsType() const;
    
    /* Extensive statistics for scientific purposes */
    qint64 getNominalDataRate() const;
    qint64 getTotalUpstream();
    qint64 getCurrentUpstream();
    qint64 getMaxUpstream();
    qint64 getTotalDownstream();
    qint64 getCurrentDownstream();
    qint64 getMaxDownstream();
    qint64 getBitsSent() const;
    qint64 getBitsReceived() const;
    
    void run();
    
    int getLinkQuality() const;
    bool isFullDuplex() const;
    int getId() const;
    
public slots:
    void setAddress(QHostAddress host);
    void setPort(int port);    
    void readBytes();
    void writeBytes(const char* data, qint64 length);
    bool connect();
    bool disconnect();
    void socketError(QAbstractSocket::SocketError socketError);
    void setAddress(const QString &text);

    
protected:
    QString name;
    QHostAddress host;
    quint16 port;
    int id;
    QTcpSocket* socket;
    bool socketIsConnected;
    
    quint64 bitsSentTotal;
    quint64 bitsSentCurrent;
    quint64 bitsSentMax;
    quint64 bitsReceivedTotal;
    quint64 bitsReceivedCurrent;
    quint64 bitsReceivedMax;
    quint64 connectionStartTime;
    QMutex statisticsMutex;
    QMutex dataMutex;
    
    void setName(QString name);
    
private:
	bool hardwareConnect(void);
#ifdef TCPLINK_READWRITE_DEBUG
    void writeDebugBytes(const char *data, qint16 size);
#endif
    
signals:
    //Signals are defined by LinkInterface
    
};

#endif // TCPLINK_H
