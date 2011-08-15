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

using namespace TNX;

//#define USE_QEXTSERIAL // this allows us to revert to old serial library during transition

SerialLink::SerialLink(QString portname, int baudRate, bool hardwareFlowControl, bool parity,
                       int dataBits, int stopBits) :
    port(NULL)
{
    // Setup settings
    this->porthandle = portname.trimmed();
#ifdef _WIN32
    // Port names above 20 need the network path format - if the port name is not already in this format
    // catch this special case
    if (this->porthandle.size() > 0 && !this->porthandle.startsWith("\\")) {
        // Append \\.\ before the port handle. Additional backslashes are used for escaping.
        this->porthandle = "\\\\.\\" + this->porthandle;
    }
#endif
    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();

    setBaudRate(baudRate);
    if (hardwareFlowControl)
    {
        portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
    }
    else
    {
        portSettings.setFlowControl(QPortSettings::FLOW_OFF);
    }
    if (parity)
    {
        portSettings.setParity(QPortSettings::PAR_EVEN);
    }
    else
    {
        portSettings.setParity(QPortSettings::PAR_NONE);
    }
    setDataBits(dataBits);
    setStopBits(stopBits);

    // Set the port name
    if (porthandle == "")
    {
        name = tr("Serial Link ") + QString::number(getId());
    }
    else
    {
        name = portname.trimmed();
    }
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
    if (settings.contains("SERIALLINK_COMM_PORT")) {
        if (porthandle == "") setPortName(settings.value("SERIALLINK_COMM_PORT").toString());
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
    forever {
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
    if(port && port->isOpen() && port->isWritable()) {
        dataMutex.lock();
        qint64 available = port->bytesAvailable();
        dataMutex.unlock();

        if(available > 0) {
            readBytes();
        }
    } else {
        emit disconnected();
    }

}

void SerialLink::writeBytes(const char* data, qint64 size)
{
    if(port && port->isOpen()) {
        int b = port->write(data, size);

        if (b > 0) {

//            qDebug() << "Serial link " << this->getName() << "transmitted" << b << "bytes:";

            // Increase write counter
            bitsSentTotal += size * 8;

//            int i;
//            for (i=0; i<size; i++)
//            {
//                unsigned char v =data[i];
//                qDebug("%02x ", v);
//            }
//            qDebug("\n");
        } else {
            disconnect();
            // Error occured
            emit communicationError(this->getName(), tr("Could not send data - link %1 is disconnected!").arg(this->getName()));
        }
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
    if(port && port->isOpen()) {
        const qint64 maxLength = 2048;
        char data[maxLength];
        qint64 numBytes = port->bytesAvailable();
        //qDebug() << "numBytes: " << numBytes;

        if(numBytes > 0) {
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
    if (port) {
        return port->bytesAvailable();
    } else {
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
    if (port) {
        //#if !defined _WIN32 || !defined _WIN64
        /* Block the thread until it returns from run() */
        //#endif
//        dataMutex.lock();
        port->flushInBuffer();
        port->flushOutBuffer();
        port->close();
        delete port;
        port = NULL;
//        dataMutex.unlock();

        if(this->isRunning()) this->terminate(); //stop running the thread, restart it upon connect

        bool closed = true;
        //port->isOpen();

        emit disconnected();
        emit connected(false);
        return closed;
    } else {
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
    if (this->isRunning()) this->disconnect();
    this->start(LowPriority);
    return true;
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
    if(port) {
        port->close();
        delete port;
    }
    port = new QSerialPort(porthandle, portSettings);
    QObject::connect(port,SIGNAL(aboutToClose()),this,SIGNAL(disconnected()));
    port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);
    connectionStartTime = MG::TIME::getGroundTimeNow();

    port->open();

    bool connectionUp = isConnected();
    if(connectionUp) {
        emit connected();
        emit connected(true);
    }

    //qDebug() << "CONNECTING LINK: " << __FILE__ << __LINE__ << "with settings" << port->portName() << getBaudRate() << getDataBits() << getParityType() << getStopBits();


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
    if (port) {
        return port->isOpen();
    } else {
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
    switch (portSettings.baudRate()) {
#ifndef Q_OS_WIN
    case QPortSettings::BAUDR_50:
        dataRate = 50;
        break;
    case QPortSettings::BAUDR_75:
        dataRate = 75;
        break;
    case QPortSettings::BAUDR_110:
        dataRate = 110;
        break;
    case QPortSettings::BAUDR_134:
        dataRate = 134;
        break;
    case QPortSettings::BAUDR_150:
        dataRate = 150;
        break;
    case QPortSettings::BAUDR_200:
        dataRate = 200;
        break;
#endif
    case QPortSettings::BAUDR_300:
        dataRate = 300;
        break;
    case QPortSettings::BAUDR_600:
        dataRate = 600;
        break;
    case QPortSettings::BAUDR_1200:
        dataRate = 1200;
        break;
#ifndef Q_OS_WIN
    case QPortSettings::BAUDR_1800:
        dataRate = 1800;
        break;
#endif
    case QPortSettings::BAUDR_2400:
        dataRate = 2400;
        break;
    case QPortSettings::BAUDR_4800:
        dataRate = 4800;
        break;
    case QPortSettings::BAUDR_9600:
        dataRate = 9600;
        break;
#ifdef Q_OS_WIN
    case QPortSettings::BAUDR_14400:
        dataRate = 14400;
        break;
#endif
    case QPortSettings::BAUDR_19200:
        dataRate = 19200;
        break;
    case QPortSettings::BAUDR_38400:
        dataRate = 38400;
        break;
#ifdef Q_OS_WIN
    case QPortSettings::BAUDR_56000:
        dataRate = 56000;
        break;
#endif
    case QPortSettings::BAUDR_57600:
        dataRate = 57600;
        break;
#ifdef Q_OS_WIN_XXXX // FIXME
    case QPortSettings::BAUDR_76800:
        dataRate = 76800;
        break;
#endif
    case QPortSettings::BAUDR_115200:
        dataRate = 115200;
        break;
#ifdef Q_OS_WIN
        // Windows-specific high-end baudrates
    case QPortSettings::BAUDR_128000:
        dataRate = 128000;
        break;
    case QPortSettings::BAUDR_256000:
        dataRate = 256000;
    case QPortSettings::BAUDR_230400:
        dataRate = 230400;
    case QPortSettings::BAUDR_460800:
        dataRate = 460800;
#endif
        // All-OS high-speed
    case QPortSettings::BAUDR_921600:
        dataRate = 921600;
        break;
    case QPortSettings::BAUDR_UNKNOWN:
        default:
        // Do nothing
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
    return portSettings.baudRate();
}

int SerialLink::getFlowType()
{
    return portSettings.flowControl();
}

int SerialLink::getParityType()
{
    return portSettings.parity();
}

int SerialLink::getDataBitsType()
{
    return portSettings.dataBits();
}

int SerialLink::getStopBitsType()
{
    return portSettings.stopBits();
}

int SerialLink::getDataBits()
{
    int ret = -1;
    switch (portSettings.dataBits()) {
    case QPortSettings::DB_5:
        ret = 5;
        break;
    case QPortSettings::DB_6:
        ret = 6;
        break;
    case QPortSettings::DB_7:
        ret = 7;
        break;
    case QPortSettings::DB_8:
        ret = 8;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

int SerialLink::getStopBits()
{
    int ret = -1;
        switch (portSettings.stopBits()) {
        case QPortSettings::STOP_1:
            ret = 1;
            break;
        case QPortSettings::STOP_2:
            ret = 2;
            break;
        default:
            ret = -1;
            break;
        }
    return ret;
}

bool SerialLink::setPortName(QString portName)
{
    if(portName.trimmed().length() > 0) {
        bool reconnect = false;
        if (isConnected()) reconnect = true;
        disconnect();

        this->porthandle = portName.trimmed();
        setName(tr("serial port ") + portName.trimmed());
#ifdef _WIN32
        // Port names above 20 need the network path format - if the port name is not already in this format
        // catch this special case
        if (!this->porthandle.startsWith("\\")) {
            // Append \\.\ before the port handle. Additional backslashes are used for escaping.
            this->porthandle = "\\\\.\\" + this->porthandle;
        }
#endif

        if(reconnect) connect();
        return true;
    } else {
        return false;
    }
}


bool SerialLink::setBaudRateType(int rateIndex)
{
    bool reconnect = false;
    bool accepted = true; // This is changed if none of the data rates matches
    if(isConnected()) reconnect = true;
    disconnect();

#ifdef Q_OS_WIN
	const int minBaud = (int)QPortSettings::BAUDR_14400;
#else
	const int minBaud = (int)QPortSettings::BAUDR_50;
#endif

    if (rateIndex >= minBaud && rateIndex <= (int)QPortSettings::BAUDR_921600)
    {
        portSettings.setBaudRate((QPortSettings::BaudRate)rateIndex);
    }

    if(reconnect) connect();
    return accepted;
}

bool SerialLink::setBaudRateString(const QString& rate)
{
    bool ok;
    int intrate = rate.toInt(&ok);
    if (!ok) return false;
    return setBaudRate(intrate);
}

bool SerialLink::setBaudRate(int rate)
{
    //qDebug() << "BAUD RATE:" << rate;

    bool reconnect = false;
    bool accepted = true; // This is changed if none of the data rates matches
    if(isConnected()) {
        reconnect = true;
    }
    disconnect();

    switch (rate) {

#ifndef Q_OS_WIN
    case 50:
        portSettings.setBaudRate(QPortSettings::BAUDR_50);
        break;
    case 75:
        portSettings.setBaudRate(QPortSettings::BAUDR_75);
        break;
    case 110:
        portSettings.setBaudRate(QPortSettings::BAUDR_110);
        break;
    case 134:
        portSettings.setBaudRate(QPortSettings::BAUDR_134);
        break;
    case 150:
        portSettings.setBaudRate(QPortSettings::BAUDR_150);
        break;
    case 200:
        portSettings.setBaudRate(QPortSettings::BAUDR_200);
        break;
#endif
    case 300:
        portSettings.setBaudRate(QPortSettings::BAUDR_300);
        break;
    case 600:
        portSettings.setBaudRate(QPortSettings::BAUDR_600);
        break;
    case 1200:
        portSettings.setBaudRate(QPortSettings::BAUDR_1200);
        break;
#ifndef Q_OS_WIN
    case 1800:
        portSettings.setBaudRate(QPortSettings::BAUDR_1800);
        break;
#endif
    case 2400:
        portSettings.setBaudRate(QPortSettings::BAUDR_2400);
        break;
    case 4800:
        portSettings.setBaudRate(QPortSettings::BAUDR_4800);
        break;
    case 9600:
        portSettings.setBaudRate(QPortSettings::BAUDR_9600);
        break;
#ifdef Q_OS_WIN
    case 14400:
        portSettings.setBaudRate(QPortSettings::BAUDR_14400);
        break;
#endif
    case 19200:
        portSettings.setBaudRate(QPortSettings::BAUDR_19200);
        break;
    case 38400:
        portSettings.setBaudRate(QPortSettings::BAUDR_38400);
        break;
#ifdef Q_OS_WIN
    case 56000:
        portSettings.setBaudRate(QPortSettings::BAUDR_56000);
        break;
#endif
    case 57600:
        portSettings.setBaudRate(QPortSettings::BAUDR_57600);
        break;
#ifdef Q_OS_WIN_XXXX // FIXME CHECK THIS
    case 76800:
        portSettings.setBaudRate(QPortSettings::BAUDR_76800);
        break;
#endif
    case 115200:
        portSettings.setBaudRate(QPortSettings::BAUDR_115200);
        break;
#ifdef Q_OS_WIN
    case 128000:
        portSettings.setBaudRate(QPortSettings::BAUDR_128000);
        break;
    case 230400:
        portSettings.setBaudRate(QPortSettings::BAUDR_230400);
        break;
    case 256000:
        portSettings.setBaudRate(QPortSettings::BAUDR_256000);
        break;
    case 460800:
        portSettings.setBaudRate(QPortSettings::BAUDR_460800);
        break;
#endif
    case 921600:
        portSettings.setBaudRate(QPortSettings::BAUDR_921600);
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

    switch (flow) {
    case (int)QPortSettings::FLOW_OFF:
        portSettings.setFlowControl(QPortSettings::FLOW_OFF);
        break;
    case (int)QPortSettings::FLOW_HARDWARE:
        portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
        break;
    case (int)QPortSettings::FLOW_XONXOFF:
        portSettings.setFlowControl(QPortSettings::FLOW_XONXOFF);
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

    switch (parity) {
    case (int)QPortSettings::PAR_NONE:
        portSettings.setParity(QPortSettings::PAR_NONE);
        break;
    case (int)QPortSettings::PAR_ODD:
        portSettings.setParity(QPortSettings::PAR_ODD);
        break;
    case (int)QPortSettings::PAR_EVEN:
        portSettings.setParity(QPortSettings::PAR_EVEN);
        break;
    case (int)QPortSettings::PAR_SPACE:
        portSettings.setParity(QPortSettings::PAR_SPACE);
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
    //qDebug() << "Setting" << dataBits << "data bits";
    bool reconnect = false;
    if (isConnected()) reconnect = true;
    bool accepted = true;
    disconnect();

    switch (dataBits) {
    case 5:
        portSettings.setDataBits(QPortSettings::DB_5);
        break;
    case 6:
        portSettings.setDataBits(QPortSettings::DB_6);
        break;
    case 7:
        portSettings.setDataBits(QPortSettings::DB_7);
        break;
    case 8:
        portSettings.setDataBits(QPortSettings::DB_8);
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

    switch (stopBits) {
    case 1:
        portSettings.setStopBits(QPortSettings::STOP_1);
        break;
    case 2:
        portSettings.setStopBits(QPortSettings::STOP_2);
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

    if (dataBits >= (int)QPortSettings::DB_5 && dataBits <= (int)QPortSettings::DB_8) {
        portSettings.setDataBits((QPortSettings::DataBits) dataBits);

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

    if (stopBits >= (int)QPortSettings::STOP_1 && stopBits <= (int)QPortSettings::STOP_2) {
        portSettings.setStopBits((QPortSettings::StopBits) stopBits);

        if(reconnect) connect();
        accepted = true;
    }

    if(reconnect) connect();
    return accepted;
}
