/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Definition of MAVLinkSimulationLink
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef MAVLINKSIMULATIONLINK_H
#define MAVLINKSIMULATIONLINK_H

#include <QFile>
#include <QTimer>
#include <QTextStream>
#include <QQueue>
#include <QMutex>
#include <QMap>
#include <qmath.h>
#include <inttypes.h>
#include "QGCMAVLink.h"

#include "LinkInterface.h"

class MAVLinkSimulationLink : public LinkInterface
{
    Q_OBJECT
public:
    MAVLinkSimulationLink(QString readFile="", QString writeFile="", int rate=5, QObject* parent = 0);
    ~MAVLinkSimulationLink();
    bool isConnected();
    qint64 bytesAvailable();

    void run();

    bool connect();
    bool disconnect();

    /* Extensive statistics for scientific purposes */
    qint64 getNominalDataRate();
    qint64 getTotalUpstream();
    qint64 getShortTermUpstream();
    qint64 getCurrentUpstream();
    qint64 getMaxUpstream();
    qint64 getTotalDownstream();
    qint64 getShortTermDownstream();
    qint64 getCurrentDownstream();
    qint64 getMaxDownstream();
    qint64 getBitsSent();
    qint64 getBitsReceived();

    QString getName();
    int getId();
    int getBaudRate();
    int getBaudRateType();
    int getFlowType();
    int getParityType();
    int getDataBitsType();
    int getStopBitsType();

    int getLinkQuality();
    bool isFullDuplex();

public slots:
    void writeBytes(const char* data, qint64 size);
    void readBytes();
    /** @brief Mainloop simulating the mainloop of the MAV */
    virtual void mainloop();
    bool connectLink(bool connect);
    void connectLink();
    void sendMAVLinkMessage(const mavlink_message_t* msg);


protected:

    // UAS properties
    float roll, pitch, yaw;
    double x, y, z;
    double spX, spY, spZ, spYaw;
    int battery;

    QTimer* timer;
    /** File which contains the input data (simulated robot messages) **/
    QFile* simulationFile;
    QFile* mavlinkLogFile;
    QString simulationHeader;
    /** File where the commands sent by the groundstation are stored **/
    QFile* receiveFile;
    QTextStream textStream;
    QTextStream* fileStream;
    QTextStream* outStream;
    /** Buffer which can be read from connected protocols through readBytes(). **/
    QMutex readyBufferMutex;
    bool _isConnected;
    quint64 rate;
    int maxTimeNoise;
    quint64 lastSent;
    static const int streamlength = 4096;
    unsigned int streampointer;
    //const int testoffset = 0;
    uint8_t stream[streamlength];

    int readyBytes;
    QQueue<uint8_t> readyBuffer;

    int id;
    QString name;
    qint64 timeOffset;
    mavlink_sys_status_t status;
    mavlink_heartbeat_t system;
    QMap<QString, float> onboardParams;

    void enqueue(uint8_t* stream, uint8_t* index, mavlink_message_t* msg);

    static const uint8_t systemId = 220;
    static const uint8_t componentId = 200;
    static const uint16_t version = 1000;

signals:
    void valueChanged(int uasId, QString curve, double value, quint64 usec);
    void messageReceived(const mavlink_message_t& message);

};

#endif // MAVLINKSIMULATIONLINK_H
