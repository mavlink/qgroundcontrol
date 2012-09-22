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
 * @file QGCXPlaneLink.h
 *   @brief X-Plane simulation link
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef QGCXPLANESIMULATIONLINK_H
#define QGCXPLANESIMULATIONLINK_H

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QUdpSocket>
#include <QTimer>
#include <QProcess>
#include <LinkInterface.h>
#include <configuration.h>
#include "UASInterface.h"
#include "QGCHilLink.h"

class QGCXPlaneLink : public QGCHilLink
{
    Q_OBJECT
    //Q_INTERFACES(QGCXPlaneLinkInterface:LinkInterface)

public:
    QGCXPlaneLink(UASInterface* mav, QString remoteHost=QString("127.0.0.1:49000"), QHostAddress localHost = QHostAddress::Any, quint16 localPort = 49005);
    ~QGCXPlaneLink();

    bool isConnected();
    qint64 bytesAvailable();
    int getPort() const {
        return localPort;
    }

    /**
     * @brief The human readable port name
     */
    QString getName();

    void run();

public slots:
//    void setAddress(QString address);
    void setPort(int port);
    /** @brief Add a new host to broadcast messages to */
    void setRemoteHost(const QString& host);
    /** @brief Send new control states to the simulation */
    void updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode);
//    /** @brief Remove a host from broadcasting messages to */
//    void removeHost(const QString& host);
    //    void readPendingDatagrams();
    void processError(QProcess::ProcessError err);

    void readBytes();
    /**
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    void writeBytes(const char* data, qint64 length);
    bool connectSimulation();
    bool disconnectSimulation();
    /**
     * @brief Select airplane model
     * @param plane the name of the airplane
     */
    void selectPlane(const QString& plane);
    /**
     * @brief Set the airplane position and attitude
     * @param lat
     * @param lon
     * @param alt
     * @param roll
     * @param pitch
     * @param yaw
     */
    void setPositionAttitude(double lat, double lon, double alt, double roll, double pitch, double yaw);

    /**
     * @brief Set a random position
     */
    void setRandomPosition();

    /**
     * @brief Set a random attitude
     */
    void setRandomAttitude();

protected:
    UASInterface* mav;
    QString name;
    QHostAddress localHost;
    quint16 localPort;
    QHostAddress remoteHost;
    quint16 remotePort;
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
    QTimer refreshTimer;
    QProcess* process;
    QProcess* terraSync;

    bool gpsReceived;
    bool attitudeReceived;

    void setName(QString name);

};

#endif // QGCXPLANESIMULATIONLINK_H
