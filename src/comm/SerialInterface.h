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
 *   @author James Goppertr <james.goppert@gmail.edu>
 *
 */

#ifndef SERIALINTERFACE_H
#define SERIALINTERFACE_H

#include <QIODevice>
#include "qextserialport.h"
#include <QtSerialPort/QSerialPort>

/**
 * @brief The SerialInterface abstracts low level serial calls
 */
class SerialInterface : public QObject
{
    Q_OBJECT

signals:
    void aboutToClose();

public:

    enum baudRateType {
        BAUD50,                //POSIX ONLY
        BAUD75,                //POSIX ONLY
        BAUD110,
        BAUD134,               //POSIX ONLY
        BAUD150,               //POSIX ONLY
        BAUD200,               //POSIX ONLY
        BAUD300,
        BAUD600,
        BAUD1200,
        BAUD1800,              //POSIX ONLY
        BAUD2400,
        BAUD4800,
        BAUD9600,
        BAUD14400,             //WINDOWS ONLY
        BAUD19200,
        BAUD38400,
        BAUD56000,             //WINDOWS ONLY
        BAUD57600,
        BAUD76800,             //POSIX ONLY
        BAUD115200,
        BAUD128000,            // WINDOWS ONLY
        BAUD230400,            // WINDOWS ONLY
        BAUD256000,            // WINDOWS ONLY
        BAUD460800,            // WINDOWS ONLY
        BAUD921600             // WINDOWS ONLY
    };

    enum dataBitsType {
        DATA_5,
        DATA_6,
        DATA_7,
        DATA_8
    };

    enum parityType {
        PAR_NONE,
        PAR_ODD,
        PAR_EVEN,
        PAR_MARK,               //WINDOWS ONLY
        PAR_SPACE
    };

    enum stopBitsType {
        STOP_1,
        STOP_1_5,               //WINDOWS ONLY
        STOP_2
    };

    enum flowType {
        FLOW_OFF,
        FLOW_HARDWARE,
        FLOW_XONXOFF
    };

    /**
     * structure to contain port settings
     */
    struct portSettings {
        baudRateType BaudRate;
        dataBitsType DataBits;
        parityType Parity;
        stopBitsType StopBits;
        flowType FlowControl;
        long timeout_Millisec;
    };

    virtual bool isOpen() = 0;
    virtual bool isWriteable() = 0;
    virtual bool bytesAvailable() = 0;
    virtual int write(const char * data, qint64 size) = 0;
    virtual void read(char * data, qint64 numBytes) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual void open(QIODevice::OpenModeFlag flag) = 0;
    virtual void setBaudRate(baudRateType baudrate) = 0;
    virtual void setParity(parityType parity) = 0;
    virtual void setStopBits(stopBitsType stopBits) = 0;
    virtual void setDataBits(dataBitsType dataBits) = 0;
    virtual void setTimeout(qint64 timeout) = 0;

};

class SerialQextserial : public SerialInterface
{
    Q_OBJECT
private:
    QextSerialPort * _port;
signals:
    void aboutToClose();
public:
    SerialQextserial(QString porthandle, QextSerialPort::QueryMode mode) : _port(NULL) {
        _port = new QextSerialPort(porthandle, QextSerialPort::Polling);
        QObject::connect(_port,SIGNAL(aboutToClose()),this,SIGNAL(aboutToClose()));
    }
    virtual bool isOpen() {
        return _port->isOpen();
    }
    virtual bool isWriteable() {
        return _port->isWritable();    // yess, that is mis-spelled, writable
    }
    virtual bool bytesAvailable() {
        return _port->bytesAvailable();
    }
    virtual int write(const char * data, qint64 size) {
        return _port->write(data,size);
    }
    virtual void read(char * data, qint64 numBytes) {
        _port->read(data,numBytes);
    }
    virtual void flush() {
        _port->flush();
    }
    virtual void close() {
        _port->close();
    }
    virtual void open(QIODevice::OpenModeFlag flag) {
        _port->open(flag);
    }
    virtual void setBaudRate(SerialInterface::baudRateType baudrate) {
        _port->setBaudRate((BaudRateType)baudrate);
    }
    virtual void setParity(SerialInterface::parityType parity) {
        _port->setParity((ParityType)parity);
    }
    virtual void setStopBits(SerialInterface::stopBitsType stopBits) {
        _port->setStopBits((StopBitsType)stopBits);
    }
    virtual void setDataBits(SerialInterface::dataBitsType dataBits) {
        _port->setDataBits((DataBitsType)dataBits);
    }
    virtual void setTimeout(qint64 timeout) {
        _port->setTimeout(timeout);
    };
};

using namespace TNX;

class SerialQserial : public SerialInterface
{
    Q_OBJECT
private:
    QSerialPort * _port;
    TNX::QPortSettings settings;
signals:
    void aboutToClose();
public:
    SerialQserial(QString porthandle, QIODevice::OpenModeFlag flag=QIODevice::ReadWrite)
        : _port(new QSerialPort(porthandle,settings)) {
        QObject::connect(_port,SIGNAL(aboutToClose()),this,SIGNAL(aboutToClose()));
    }
    virtual bool isOpen() {
        return _port->isOpen();
    }
    virtual bool isWriteable() {
        _port->isWritable();
    }
    virtual bool bytesAvailable() {
        return _port->bytesAvailable();
    }
    virtual int write(const char * data, qint64 size) {
        return _port->write(data,size);
    }
    virtual void read(char * data, qint64 numBytes) {
        _port->read(data,numBytes);
    }
    virtual void flush() {
        _port->flushInBuffer();
        _port->flushOutBuffer();
    }
    virtual void close() {
        _port->close();
    }
    virtual void open(QIODevice::OpenModeFlag flag) {
        _port->open(flag);
    }
    virtual void setBaudRate(SerialInterface::baudRateType baudrate) {
        //_port->setBaudRate((BaudRateType)baudrate);
    }
    virtual void setParity(SerialInterface::parityType parity) {
        //_port->setParity((ParityType)parity);
    }
    virtual void setStopBits(SerialInterface::stopBitsType stopBits) {
        //_port->setStopBits((StopBitsType)stopBits);
    }
    virtual void setDataBits(SerialInterface::dataBitsType dataBits) {
        //_port->setDataBits((DataBitsType)dataBits);
    }
    virtual void setTimeout(qint64 timeout) {
        //_port->setTimeout(timeout);
    };
};

#endif // SERIALINTERFACE_H

// vim:ts=4:sw=4:tw=78:expandtab:
