/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of simulated system link
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
#include <inttypes.h>
#include <mavlink.h>

#include "LinkInterface.h"

class MAVLinkSimulationLink : public LinkInterface
{
    Q_OBJECT
public:
    MAVLinkSimulationLink(QString readFile="", QString writeFile="", int rate=5);
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
    void mainloop();
    bool connectLink(bool connect);


protected:

    // UAS properties
    float roll, pitch, yaw;
    float x, y, z;
    float spX, spY, spZ, spYaw;
    int battery;

    QTimer* timer;
    /** File which contains the input data (simulated robot messages) **/
    QFile* simulationFile;
    QFile* mavlinkLogFile;
    QString simulationHeader;
    /** File where the commands sent by the groundstation are stored **/
    QFile* receiveFile;
    QTextStream stream;
    QTextStream* fileStream;
    QTextStream* outStream;
    /** Buffer which can be read from connected protocols through readBytes(). **/
    QMutex readyBufferMutex;
    bool _isConnected;
    quint64 rate;
    int maxTimeNoise;
    quint64 lastSent;

    int readyBytes;
    QQueue<uint8_t> readyBuffer;

    int id;
    QString name;
    qint64 timeOffset;
    mavlink_sys_status_t status;
    QMap<QString, float> onboardParams;

    void enqueue(uint8_t* stream, uint8_t* index, mavlink_message_t* msg);

    static const uint8_t systemId = 220;
    static const uint8_t componentId = 0;
    static const uint16_t version = 1000;

signals:
    void valueChanged(int uasId, QString curve, double value, quint64 usec);

};

#endif // MAVLINKSIMULATIONLINK_H
