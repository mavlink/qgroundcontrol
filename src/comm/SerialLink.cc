/*=====================================================================
======================================================================*/
/**
 * @file
 *   @brief Cross-platform support for serial ports
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QMutexLocker>
#include "SerialLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <MG.h>
#include <iostream>
#ifdef _WIN32
#include "windows.h"
#endif


SerialLink::SerialLink(QString portname, BaudRateType baudrate, FlowType flow, ParityType parity, DataBitsType dataBits, StopBitsType stopBits) :
        port(NULL)
{
    // Setup settings
    this->porthandle = portname.trimmed();
#ifdef _WIN32
    // Port names above 20 need the network path format - if the port name is not already in this format
    // catch this special case
    if (this->porthandle.size() > 0 && !this->porthandle.startsWith("\\"))
    {
        // Append \\.\ before the port handle. Additional backslashes are used for escaping.
        this->porthandle = "\\\\.\\" + this->porthandle;
    }
#endif
    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();

    // *nix (Linux, MacOS tested) serial port support
//    port = new QextSerialPort(porthandle, QextSerialPort::Polling);
    //port = new QextSerialPort(porthandle, QextSerialPort::EventDriven);

    this->baudrate = baudrate;
    this->flow = flow;
    this->parity = parity;
    this->dataBits = dataBits;
    this->stopBits = stopBits;
    this->timeout = 1; ///< The timeout controls how long the program flow should wait for new serial bytes. As we're polling, we don't want to wait at all.
//    port->setTimeout(timeout); // Timeout of 0 ms, we don't want to wait for data, we just poll again next time
//    port->setBaudRate(baudrate);
//    port->setFlowControl(flow);
//    port->setParity(parity);
//    port->setDataBits(dataBits);
//    port->setStopBits(stopBits);

    // Set the port name
    if (porthandle == "")
    {
        //        name = tr("serial link ") + QString::number(getId()) + tr(" (unconfigured)");
        name = tr("Serial Link ") + QString::number(getId());
    }
    else
    {
        name = portname.trimmed();
    }

#ifdef _WIN3232
    // Windows 32bit & 64bit serial connection
    winPort = CreateFile(porthandle,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         0,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         0);
    if(winPort==INVALID_HANDLE_VALUE){
        if(GetLastError()==ERROR_FILE_NOT_FOUND){
            //serial port does not exist. Inform user.
        }
        //some other error occurred. Inform user.
    }
#else

#endif

    loadSettings();
}

SerialLink::~SerialLink()
{
    disconnect();
    if(port) delete port;
    port = NULL;
}

void SerialLink::loadSettings()
{
    // Load defaults from settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.sync();
    if (settings.contains("SERIALLINK_COMM_PORT"))
    {
        setPortName(settings.value("SERIALLINK_COMM_PORT").toString());
        setBaudRateType(settings.value("SERIALLINK_COMM_BAUD").toInt());
        setParityType(settings.value("SERIALLINK_COMM_PARITY").toInt());
        setStopBits(settings.value("SERIALLINK_COMM_STOPBITS").toInt());
        setDataBits(settings.value("SERIALLINK_COMM_DATABITS").toInt());
        setFlowType(settings.value("SERIALLINK_COMM_FLOW_CONTROL").toInt());
    }
}

void SerialLink::writeSettings()
{
    // Store settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.setValue("SERIALLINK_COMM_PORT", this->porthandle);
    settings.setValue("SERIALLINK_COMM_BAUD", getBaudRateType());
    settings.setValue("SERIALLINK_COMM_PARITY", getParityType());
    settings.setValue("SERIALLINK_COMM_STOPBITS", getStopBits());
    settings.setValue("SERIALLINK_COMM_DATABITS", getDataBits());
    settings.setValue("SERIALLINK_COMM_FLOW_CONTROL", getFlowType());
    settings.sync();
}


/**
 * @brief Runs the thread
 *
 **/
void SerialLink::run()
{
    // Initialize the connection
    hardwareConnect();

    // Qt way to make clear what a while(1) loop does
    forever
    {
        // Check if new bytes have arrived, if yes, emit the notification signal
        checkForBytes();
        /* Serial data isn't arriving that fast normally, this saves the thread
                 * from consuming too much processing time
                 */
        MG::SLEEP::msleep(SerialLink::poll_interval);
    }
}


void SerialLink::checkForBytes()
{
    /* Check if bytes are available */
    if(port && port->isOpen())
    {
        dataMutex.lock();
        qint64 available = port->bytesAvailable();
        dataMutex.unlock();

        if(available > 0)
        {
            readBytes();
        }
    }
    else
    {
        emit disconnected();
    }

}


void SerialLink::writeBytes(const char* data, qint64 size)
{
    if(port && port->isOpen())
    {
        int b = port->write(data, size);
        qDebug() << "Serial link " << this->getName() << "transmitted" << b << "bytes:";

        // Increase write counter
        bitsSentTotal += size * 8;

        //        int i;
        //        for (i=0; i<size; i++)
        //        {
        //            unsigned char v=data[i];

        //            //fprintf(stderr,"%02x ", v);
        //        }
    }
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void SerialLink::readBytes()
{
    dataMutex.lock();
    if(port && port->isOpen())
    {
        const qint64 maxLength = 2048;
        char data[maxLength];
        qint64 numBytes = port->bytesAvailable();

        if(numBytes > 0)
        {
            /* Read as much data in buffer as possible without overflow */
            if(maxLength < numBytes) numBytes = maxLength;

            port->read(data, numBytes);
            QByteArray b(data, numBytes);
            emit bytesReceived(this, b);

            //qDebug() << "SerialLink::readBytes()" << std::hex << data;
            //            int i;
            //            for (i=0; i<numBytes; i++){
            //                unsigned int v=data[i];
            //
            //                fprintf(stderr,"%02x ", v);
            //            }
            //            fprintf(stderr,"\n");
            bitsReceivedTotal += numBytes * 8;
        }
    }
    dataMutex.unlock();
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 SerialLink::bytesAvailable()
{
    if (port)
    {
        return port->bytesAvailable();
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool SerialLink::disconnect()
{
    if (port)
    {
        //#if !defined _WIN32 || !defined _WIN64
        /* Block the thread until it returns from run() */
        //#endif
//        dataMutex.lock();
        port->flush();
        port->close();
        delete port;
        port = NULL;
//        dataMutex.unlock();

        if(this->isRunning()) this->terminate(); //stop running the thread, restart it upon connect

        bool closed = true;
        //port->isOpen();

        emit disconnected();
        emit connected(false);
        return ! closed;
    }
    else
    {
        // No port, so we're disconnected
        return true;
    }

}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool SerialLink::connect()
{
    if (!isConnected())
    {
        qDebug() << "CONNECTING LINK: " << __FILE__ << __LINE__ << "with settings" << porthandle << baudrate << dataBits << parity << stopBits;
        if (!this->isRunning())
        {
            this->start(LowPriority);
        }
    }
}

/**
 * @brief This function is called indirectly by the connect() call.
 *
 * The connect() function starts the thread and indirectly calls this method.
 *
 * @return True if the connection could be established, false otherwise
 * @see connect() For the right function to establish the connection.
 **/
bool SerialLink::hardwareConnect()
{
    if(port)
    {
        port->close();
        delete port;
    }
    port = new QextSerialPort(porthandle, QextSerialPort::Polling);
    QObject::connect(port, SIGNAL(aboutToClose()), this, SIGNAL(disconnected()));

    port->open(QIODevice::ReadWrite);
    port->setBaudRate(this->baudrate);
    port->setParity(this->parity);
    port->setStopBits(this->stopBits);
    port->setDataBits(this->dataBits);
    port->setTimeout(timeout); // Timeout of 0 ms, we don't want to wait for data, we just poll again next time

    connectionStartTime = MG::TIME::getGroundTimeNow();

    bool connectionUp = isConnected();
    if(connectionUp)
    {
        emit connected();
        emit connected(true);
    }

    writeSettings();

    return connectionUp;
}


/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool SerialLink::isConnected()
{
    if (port)
    {
        return port->isOpen();
    }
    else
    {
        return false;
    }
}

int SerialLink::getId()
{
    return id;
}

QString SerialLink::getName()
{
    return name;
}

void SerialLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}


qint64 SerialLink::getNominalDataRate()
{
    qint64 dataRate = 0;
    switch (baudrate)
    {
    case BAUD50:
        dataRate = 50;
        break;
    case BAUD75:
        dataRate = 75;
        break;
    case BAUD110:
        dataRate = 110;
        break;
    case BAUD134:
        dataRate = 134;
        break;
    case BAUD150:
        dataRate = 150;
        break;
    case BAUD200:
        dataRate = 200;
        break;
    case BAUD300:
        dataRate = 300;
        break;
    case BAUD600:
        dataRate = 600;
        break;
    case BAUD1200:
        dataRate = 1200;
        break;
    case BAUD1800:
        dataRate = 1800;
        break;
    case BAUD2400:
        dataRate = 2400;
        break;
    case BAUD4800:
        dataRate = 4800;
        break;
    case BAUD9600:
        dataRate = 9600;
        break;
    case BAUD14400:
        dataRate = 14400;
        break;
    case BAUD19200:
        dataRate = 19200;
        break;
    case BAUD38400:
        dataRate = 38400;
        break;
    case BAUD56000:
        dataRate = 56000;
        break;
    case BAUD57600:
        dataRate = 57600;
        break;
    case BAUD76800:
        dataRate = 76800;
        break;
    case BAUD115200:
        dataRate = 115200;
        break;
    case BAUD128000:
        dataRate = 128000;
        break;
    case BAUD256000:
        dataRate = 256000;
        // Windows-specific high-end baudrates
    case BAUD230400:
        dataRate = 230400;
    case BAUD460800:
        dataRate = 460800;
    case BAUD921600:
        dataRate = 921600;
        break;
    }
    return dataRate;
}

qint64 SerialLink::getTotalUpstream()
{
    statisticsMutex.lock();
    return bitsSentTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
}

qint64 SerialLink::getCurrentUpstream()
{
    return 0; // TODO
}

qint64 SerialLink::getMaxUpstream()
{
    return 0; // TODO
}

qint64 SerialLink::getBitsSent()
{
    return bitsSentTotal;
}

qint64 SerialLink::getBitsReceived()
{
    return bitsReceivedTotal;
}

qint64 SerialLink::getTotalDownstream()
{
    statisticsMutex.lock();
    return bitsReceivedTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
}

qint64 SerialLink::getCurrentDownstream()
{
    return 0; // TODO
}

qint64 SerialLink::getMaxDownstream()
{
    return 0; // TODO
}

bool SerialLink::isFullDuplex()
{
    /* Serial connections are always half duplex */
    return false;
}

int SerialLink::getLinkQuality()
{
    /* This feature is not supported with this interface */
    return -1;
}

QString SerialLink::getPortName()
{
    return porthandle;
}

int SerialLink::getBaudRate()
{
    return getNominalDataRate();
}

int SerialLink::getBaudRateType()
{
    return baudrate;
}

int SerialLink::getFlowType()
{
    return flow;
}

int SerialLink::getParityType()
{
    return parity;
}

int SerialLink::getDataBitsType()
{
    return dataBits;
}

int SerialLink::getStopBitsType()
{
    return stopBits;
}

int SerialLink::getDataBits()
{
    int ret;
    switch (dataBits)
    {
    case DATA_5:
        ret = 5;
        break;
    case DATA_6:
        ret = 6;
        break;
    case DATA_7:
        ret = 7;
        break;
    case DATA_8:
        ret = 8;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

int SerialLink::getStopBits()
{
    int ret;
    switch (stopBits)
    {
    case STOP_1:
        ret = 1;
        break;
    case STOP_2:
        ret = 2;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

bool SerialLink::setPortName(QString portName)
{
    if(portName.trimmed().length() > 0)
    {
        bool reconnect = false;
        if (isConnected()) reconnect = true;
        disconnect();

        this->porthandle = portName.trimmed();
        setName(tr("serial port ") + portName.trimmed());
#ifdef _WIN32
        // Port names above 20 need the network path format - if the port name is not already in this format
        // catch this special case
        if (!this->porthandle.startsWith("\\"))
        {
            // Append \\.\ before the port handle. Additional backslashes are used for escaping.
            this->porthandle = "\\\\.\\" + this->porthandle;
        }
#endif

        if(reconnect) connect();
        return true;
    }
    else
    {
        return false;
    }
}


bool SerialLink::setBaudRateType(int rateIndex)
{
    bool reconnect = false;
    bool accepted = true; // This is changed if none of the data rates matches
    if(isConnected()) reconnect = true;
    disconnect();

    switch (rateIndex) {
    case 0:
        baudrate = BAUD50;
        break;
    case 1:
        baudrate = BAUD75;
        break;
    case 2:
        baudrate = BAUD110;
        break;
    case 3:
        baudrate = BAUD134;
        break;
    case 4:
        baudrate = BAUD150;
        break;
    case 5:
        baudrate = BAUD200;
        break;
    case 6:
        baudrate = BAUD300;
        break;
    case 7:
        baudrate = BAUD600;
        break;
    case 8:
        baudrate = BAUD1200;
        break;
    case 9:
        baudrate = BAUD1800;
        break;
    case 10:
        baudrate = BAUD2400;
        break;
    case 11:
        baudrate = BAUD4800;
        break;
    case 12:
        baudrate = BAUD9600;
        break;
    case 13:
        baudrate = BAUD14400;
        break;
    case 14:
        baudrate = BAUD19200;
        break;
    case 15:
        baudrate = BAUD38400;
        break;
    case 16:
        baudrate = BAUD56000;
        break;
    case 17:
        baudrate = BAUD57600;
        break;
    case 18:
        baudrate = BAUD76800;
        break;
    case 19:
        baudrate = BAUD115200;
        break;
    case 20:
        baudrate = BAUD128000;
        break;
    case 21:
        baudrate = BAUD230400;
        break;
    case 22:
        baudrate = BAUD256000;
        break;
    case 23:
        baudrate = BAUD460800;
        break;
    case 24:
        baudrate = BAUD921600;
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;
}



bool SerialLink::setBaudRate(int rate)
{
    bool reconnect = false;
    bool accepted = true; // This is changed if none of the data rates matches
    if(isConnected())
    {
        reconnect = true;
    }
    disconnect();

    switch (rate)
    {
    case 50:
        baudrate = BAUD50;
        break;
    case 75:
        baudrate = BAUD75;
        break;
    case 110:
        baudrate = BAUD110;
        break;
    case 134:
        baudrate = BAUD134;
        break;
    case 150:
        baudrate = BAUD150;
        break;
    case 200:
        baudrate = BAUD200;
        break;
    case 300:
        baudrate = BAUD300;
        break;
    case 600:
        baudrate = BAUD600;
        break;
    case 1200:
        baudrate = BAUD1200;
        break;
    case 1800:
        baudrate = BAUD1800;
        break;
    case 2400:
        baudrate = BAUD2400;
        break;
    case 4800:
        baudrate = BAUD4800;
        break;
    case 9600:
        baudrate = BAUD9600;
        break;
    case 14400:
        baudrate = BAUD14400;
        break;
    case 19200:
        baudrate = BAUD19200;
        break;
    case 38400:
        baudrate = BAUD38400;
        break;
    case 56000:
        baudrate = BAUD56000;
        break;
    case 57600:
        baudrate = BAUD57600;
        break;
    case 76800:
        baudrate = BAUD76800;
        break;
    case 115200:
        baudrate = BAUD115200;
        break;
    case 128000:
        baudrate = BAUD128000;
        break;
    case 230400:
        baudrate = BAUD230400;
        break;
    case 256000:
        baudrate = BAUD256000;
        break;
    case 460800:
        baudrate = BAUD460800;
        break;
    case 921600:
        baudrate = BAUD921600;
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;

}

bool SerialLink::setFlowType(int flow)
{
    bool reconnect = false;
    bool accepted = true;
    if(isConnected()) reconnect = true;
    disconnect();

    switch (flow)
    {
    case FLOW_OFF:
        this->flow = FLOW_OFF;
        break;
    case FLOW_HARDWARE:
        this->flow = FLOW_HARDWARE;
        break;
    case FLOW_XONXOFF:
        this->flow = FLOW_XONXOFF;
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;
}

bool SerialLink::setParityType(int parity)
{
    bool reconnect = false;
    bool accepted = true;
    if (isConnected()) reconnect = true;
    disconnect();

    switch (parity)
    {
    case (int)PAR_NONE:
        this->parity = PAR_NONE;
        break;
    case (int)PAR_ODD:
        this->parity = PAR_ODD;
        break;
    case (int)PAR_EVEN:
        this->parity = PAR_EVEN;
        break;
    case (int)PAR_MARK:
        this->parity = PAR_MARK;
        break;
    case (int)PAR_SPACE:
        this->parity = PAR_SPACE;
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if (reconnect) connect();
    return accepted;
}


bool SerialLink::setDataBits(int dataBits)
{
    bool reconnect = false;
    if (isConnected()) reconnect = true;
    bool accepted = true;
    disconnect();

    switch (dataBits)
    {
    case 5:
        this->dataBits = DATA_5;
        break;
    case 6:
        this->dataBits = DATA_6;
        break;
    case 7:
        this->dataBits = DATA_7;
        break;
    case 8:
        this->dataBits = DATA_8;
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();

    return accepted;
}

bool SerialLink::setStopBits(int stopBits)
{
    bool reconnect = false;
    bool accepted = true;
    if(isConnected()) reconnect = true;
    disconnect();

    switch (stopBits)
    {
    case 1:
        this->stopBits = STOP_1;
        break;
    case 2:
        this->stopBits = STOP_2;
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;
}

bool SerialLink::setDataBitsType(int dataBits)
{
    bool reconnect = false;
    bool accepted = false;

    if (isConnected()) reconnect = true;
    disconnect();

    if (dataBits >= (int)DATA_5 && dataBits <= (int)DATA_8)
    {
        this->dataBits = (DataBitsType) dataBits;

        if(reconnect) connect();
        accepted = true;
    }

    return accepted;
}

bool SerialLink::setStopBitsType(int stopBits)
{
    bool reconnect = false;
    bool accepted = false;
    if(isConnected()) reconnect = true;
    disconnect();

    if (stopBits >= (int)STOP_1 && dataBits <= (int)STOP_2)
    {
        StopBitsType newBits = (StopBitsType) stopBits;

        port->setStopBits(newBits);
        accepted = true;
    }

    if(reconnect) connect();
    return accepted;
}
