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
#include <qserialport.h>
#include <configuration.h>
#include "SerialLinkInterface.h"

// convenience type for passing errors
typedef  QSerialPort::SerialPortError SerialLinkPortError_t;

/**
 * @brief The SerialLink class provides cross-platform access to serial links.
 * It takes care of the link management and provides a common API to higher
 * level communication layers. It is implemented as a wrapper class for a thread
 * that handles the serial communication. All methods have therefore to be thread-
 * safe.
 *
 */
class SerialLink : public SerialLinkInterface
{
    Q_OBJECT
    //Q_INTERFACES(SerialLinkInterface:LinkInterface)


public:
    SerialLink(QString portname = "",
               int baudrate=57600,
               bool flow=false,
               bool parity=false,
               int dataBits=8,
               int stopBits=1);
    ~SerialLink();

    static const int poll_interval = SERIAL_POLL_INTERVAL; ///< Polling interval, defined in configuration.h

    static const int buffer_size = 20; ///< Specify how many data points to capture for data rate calculations.

    static const qint64 stats_timespan = 500; ///< Set the maximum age of samples to use for data calculations (ms).

    /** @brief Get a list of the currently available ports */
    QList<QString> getCurrentPorts();

    void requestReset();

    bool isConnected() const;
    qint64 bytesAvailable();

    /**
     * @brief The port handle
     */
    QString getPortName() const;
    /**
     * @brief The human readable port name
     */
    QString getName() const;
    int getBaudRate() const;
    int getDataBits() const;
    int getStopBits() const;

    // ENUM values
    int getBaudRateType() const;
    int getFlowType() const;
    int getParityType() const;
    int getDataBitsType() const;
    int getStopBitsType() const;

    qint64 getConnectionSpeed() const;
    qint64 getCurrentInDataRate() const;
    qint64 getCurrentOutDataRate() const;

    void loadSettings();
    void writeSettings();

    void run();
    void run2();

    int getId() const;

signals: //[TODO] Refactor to Linkinterface
    void updateLink(LinkInterface*);

public slots:
    bool setPortName(QString portName);
    bool setBaudRate(int rate);
    bool setDataBits(int dataBits);
    bool setStopBits(int stopBits);

    // Set string rate
    bool setBaudRateString(const QString& rate);

    // Set ENUM values
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

    void linkError(SerialLinkPortError_t error);

protected:
    quint64 m_bytesRead;
    QSerialPort* m_port;
    int m_baud;
    int m_dataBits;
    int m_flowControl;
    int m_stopBits;
    int m_parity;
    QString m_portName;
    int m_timeout;
    int m_id;
    // Implement a simple circular buffer for storing when and how much data was received.
    // Used for calculating the incoming data rate. Use with *StatsBuffer() functions.
    int inDataIndex;
    quint64 inDataWriteAmounts[buffer_size]; // In bytes
    qint64 inDataWriteTimes[buffer_size]; // in ms

    // Implement a simple circular buffer for storing when and how much data was transmit.
    // Used for calculating the outgoing data rate. Use with *StatsBuffer() functions.
    int outDataIndex;
    quint64 outDataWriteAmounts[buffer_size]; // In bytes
    qint64 outDataWriteTimes[buffer_size]; // in ms

    quint64 m_connectionStartTime; // Connection start time (ms since epoch)
    mutable QMutex m_statisticsMutex; // Mutex for accessing the statistics member variables (inData*,outData*,bitsSent,bitsReceived)
    QMutex m_dataMutex;       // Mutex for reading data from m_port
    QMutex m_writeMutex;      // Mutex for accessing the m_transmitBuffer.
    QList<QString> m_ports;

private:
    /**
     * @brief WriteDataStatsBuffer Stores transmission times/amounts for statistics
     *
     * This function logs the send times and amounts of datas to the given circular buffers.
     * This data is used for calculating the transmission rate.
     *
     * @param bytesBuffer The buffer to write the bytes value into.
     * @param timeBuffer The buffer to write the time value into
     * @param writeIndex The write index used for this buffer.
     * @param bytes The amount of bytes transmit.
     * @param time The time (in ms) this transmission occurred.
     */
    void WriteDataStatsBuffer(quint64 *bytesBuffer, qint64 *timeBuffer, int *writeIndex, quint64 bytes, qint64 time);
    
    volatile bool m_stopp;
    volatile bool m_reqReset;
    QMutex m_stoppMutex; // Mutex for accessing m_stopp
    QByteArray m_transmitBuffer; // An internal buffer for receiving data from member functions and actually transmitting them via the serial port.

    bool hardwareConnect();

signals:
    void aboutToCloseFlag();

};

#endif // SERIALLINK_H
