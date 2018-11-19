/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Brief Description
 *
 *   @author James Goppertr <james.goppert@gmail.edu>
 *
 */

#pragma once

#include <QIODevice>
#include <QtSerialPort/QSerialPort>
#include <iostream>

/**
 * @brief The SerialInterface abstracts low level serial calls
 */
class SerialInterface : public QObject
{
    Q_OBJECT

signals:
    void aboutToClose();

public:

    enum baudRateType
    {
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

    enum dataBitsType
    {
        DATA_5,
        DATA_6,
        DATA_7,
        DATA_8
    };

    enum parityType
    {
        PAR_NONE,
        PAR_ODD,
        PAR_EVEN,
        PAR_MARK,               //WINDOWS ONLY
        PAR_SPACE
    };

    enum stopBitsType
    {
        STOP_1,
        STOP_1_5,               //WINDOWS ONLY
        STOP_2
    };

    enum flowType
    {
        FLOW_OFF,
        FLOW_HARDWARE,
        FLOW_XONXOFF
    };

    /**
     * structure to contain port settings
     */
    struct portSettings
    {
        baudRateType BaudRate;
        dataBitsType DataBits;
        parityType Parity;
        stopBitsType StopBits;
        flowType FlowControl;
        long timeout_Millisec;
    };

    virtual bool isOpen() = 0;
    virtual bool isWritable() = 0;
    virtual qint64 bytesAvailable() = 0;
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
    virtual void setFlow(flowType flow) = 0;
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
        : _port(NULL) {
        QObject::connect(_port,SIGNAL(aboutToClose()),this,SIGNAL(aboutToClose()));
        settings.setBaudRate(QPortSettings::BAUDR_57600);
        settings.setStopBits(QPortSettings::STOP_1);
        settings.setDataBits(QPortSettings::DB_8);
        settings.setFlowControl(QPortSettings::FLOW_OFF);
        settings.setParity(QPortSettings::PAR_NONE);
        _port = new QSerialPort(porthandle,settings);
        _port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);
    }
    ~SerialQserial() {
        delete _port;
        _port = NULL;
    }
    virtual bool isOpen() {
        return _port->isOpen();
    }
    virtual bool isWritable() {
        _port->isWritable();
    }
    virtual qint64 bytesAvailable() {
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
        //flush();
    }
    virtual void setBaudRate(SerialInterface::baudRateType baudrate) {
        // TODO get the baudrate enum to map to one another
        settings.setBaudRate(QPortSettings::BAUDR_57600);
    }
    virtual void setParity(SerialInterface::parityType parity) {
        settings.setParity(QPortSettings::PAR_NONE);
    }
    virtual void setStopBits(SerialInterface::stopBitsType stopBits) {
        // TODO map
        settings.setStopBits(QPortSettings::STOP_1);
    }
    virtual void setDataBits(SerialInterface::dataBitsType dataBits) {
        // TODO map
        settings.setDataBits(QPortSettings::DB_8);
    }
    virtual void setTimeout(qint64 timeout) {
        // TODO implement
        //_port->setTimeout(timeout);
    }
    virtual void setFlow(SerialInterface::flowType flow) {
        // TODO map
        settings.setFlowControl(QPortSettings::FLOW_OFF);
    }
};


// vim:ts=4:sw=4:tw=78:expandtab:
