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
#include "QGCConfig.h"
#include "SerialLinkInterface.h"

// We use QSerialPort::SerialPortError in a signal so we must declare it as a meta type
#include <QSerialPort>
#include <QMetaType>
Q_DECLARE_METATYPE(QSerialPort::SerialPortError)

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

    static const int poll_interval = SERIAL_POLL_INTERVAL; ///< Polling interval, defined in QGCConfig.h

    /** @brief Get a list of the currently available ports */
    QList<QString> getCurrentPorts();

    /** @brief Check if the current port is a bootloader */
    bool isBootloader();

    void requestReset();

    bool isConnected() const;

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

    void checkIfCDC();

    void run();
    void run2();

    int getId() const;
    
    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

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

    void linkError(QSerialPort::SerialPortError error);

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
    QMutex m_dataMutex;       // Mutex for reading data from m_port
    QMutex m_writeMutex;      // Mutex for accessing the m_transmitBuffer.
    QString type;
    bool m_is_cdc;
    
private slots:
    void _rerouteDisconnected(void);

private:
    // From LinkInterface
    virtual bool _connect(void);
    virtual bool _disconnect(void);
    
    void _emitLinkError(const QString& errorMsg);

    volatile bool m_stopp;
    volatile bool m_reqReset;
    QMutex m_stoppMutex; // Mutex for accessing m_stopp
    QByteArray m_transmitBuffer; // An internal buffer for receiving data from member functions and actually transmitting them via the serial port.

    bool hardwareConnect(QString &type);

signals:
    void aboutToCloseFlag();

};

#endif // SERIALLINK_H
