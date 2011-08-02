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
 *   @brief UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef QGCFLIGHTGEARLINK_H
#define QGCFLIGHTGEARLINK_H

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QUdpSocket>
#include <LinkInterface.h>
#include <configuration.h>

class QGCFlightGearLink : public QThread
{
    Q_OBJECT
    //Q_INTERFACES(QGCFlightGearLinkInterface:LinkInterface)

public:
    QGCFlightGearLink(QHostAddress host = QHostAddress::Any, quint16 port = 5401);
    //QGCFlightGearLink(QHostAddress host = "239.255.76.67", quint16 port = 7667);
    ~QGCFlightGearLink();

    bool isConnected();
    qint64 bytesAvailable();
    int getPort() const {
        return port;
    }

    /**
     * @brief The human readable port name
     */
    QString getName();

    void run();

public slots:
    void setAddress(QString address);
    void setPort(int port);
    /** @brief Add a new host to broadcast messages to */
    void addHost(const QString& host);
    /** @brief Remove a host from broadcasting messages to */
    void removeHost(const QString& host);
    //    void readPendingDatagrams();

    void readBytes();
    /**
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    void writeBytes(const char* data, qint64 length);
    bool connect();
    bool disconnect();

protected:
    QString name;
    QHostAddress host;
	QHostAddress currentHost
	quint16 currentPort;
    quint16 port;
    int id;
    QUdpSocket* socket;
    bool connectState;

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

signals:
    // Signals are defined by LinkInterface

};

#endif // QGCFLIGHTGEARLINK_H
