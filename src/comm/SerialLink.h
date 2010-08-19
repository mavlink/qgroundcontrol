/*=====================================================================
 
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
 
(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>
 
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
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef SERIALLINK_H
#define SERIALLINK_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QString>
#include <qextserialport.h>
#include <configuration.h>
#include "SerialLinkInterface.h"
#ifdef _WIN32
#include "windows.h"
#endif


/**
 * @brief The SerialLink class provides cross-platform access to serial links.
 * It takes care of the link management and provides a common API to higher
 * level communication layers. It is implemented as a wrapper class for a thread
 * that handles the serial communication. All methods have therefore to be thread-
 * safe.
 *
 */
class SerialLink : public SerialLinkInterface {
    Q_OBJECT
    //Q_INTERFACES(SerialLinkInterface:LinkInterface)

public:
    SerialLink(QString portname=NULL, BaudRateType baudrate=BAUD57600, FlowType flow=FLOW_OFF, ParityType parity=PAR_NONE, DataBitsType dataBits=DATA_8, StopBitsType stopBits=STOP_1);
    ~SerialLink();

    static const int poll_interval = SERIAL_POLL_INTERVAL; ///< Polling interval, defined in configuration.h

    bool isConnected();
    qint64 bytesAvailable();

    /**
     * @brief The port handle
     */
    QString getPortName();
    /**
     * @brief The human readable port name
     */
    QString getName();
    int getBaudRate();
    int getBaudRateType();
    int getFlowType();
    int getParityType();
    int getDataBitsType();
    int getStopBitsType();

    /* Extensive statistics for scientific purposes */
    qint64 getNominalDataRate();
    qint64 getTotalUpstream();
    qint64 getCurrentUpstream();
    qint64 getMaxUpstream();
    qint64 getTotalDownstream();
    qint64 getCurrentDownstream();
    qint64 getMaxDownstream();
    qint64 getBitsSent();
    qint64 getBitsReceived();

    void run();

    int getLinkQuality();
    bool isFullDuplex();
    int getId();

public slots:
    bool setPortName(QString portName);
    bool setBaudRate(int rate);
    bool setBaudRateType(int rateIndex);
    bool setFlowType(int flow);
    bool setParityType(int parity);
    bool setDataBitsType(int dataBits);
    bool setStopBitsType(int stopBits);

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

protected slots:
    void checkForBytes();

protected:
    QextSerialPort* port;
#ifdef _WIN32
    HANDLE winPort;
    DCB winPortSettings;
#endif
    QString porthandle;
    QString name;
    BaudRateType baudrate;
    FlowType flow;
    ParityType parity;
    DataBitsType dataBits;
    StopBitsType stopBits;
    int timeout;
    int id;

    quint64 bitsSentTotal;
    quint64 bitsSentShortTerm;
    quint64 bitsSentCurrent;
    quint64 bitsSentMax;
    quint64 bitsReceivedTotal;
    quint64 bitsReceivedShortTerm;
    quint64 bitsReceivedCurrent;
    quint64 bitsReceivedMax;
    quint64 connectionStartTime;
    QMutex statisticsMutex;
    QMutex dataMutex;

    void setName(QString name);
    bool hardwareConnect();

signals:
    // Signals are defined by LinkInterface

};

#endif // SERIALLINK_H
