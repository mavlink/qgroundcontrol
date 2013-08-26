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
#include <qserialport.h>
#include <qserialportinfo.h>
#include "SerialLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <MG.h>



SerialLink::SerialLink(QString portname, int baudRate, bool hardwareFlowControl, bool parity,
                       int dataBits, int stopBits) :
    m_bytesRead(0),
    m_port(NULL),
    m_stopp(false),
    m_reqReset(false)
{
    qDebug() << "create SerialLink " << portname << baudRate << hardwareFlowControl
             << parity << dataBits << stopBits;
    // Setup settings
    m_portName = portname.trimmed();

    if (m_portName == "" && getCurrentPorts().size() > 0)
    {
        m_portName = m_ports.first().trimmed();
    }

    qDebug() << "m_portName " << m_portName;

    // Set unique ID and add link to the list of links
    m_id = getNextLinkId();

    m_baud = baudRate;

    if (hardwareFlowControl)
    {
        m_flowControl = QSerialPort::HardwareControl;
    }
    else
    {
        m_flowControl = QSerialPort::NoFlowControl;
    }
    if (parity)
    {
        m_parity = QSerialPort::EvenParity;
    }
    else
    {
        m_parity = QSerialPort::NoParity;
    }

    m_dataBits = dataBits;
    m_stopBits = stopBits;

    loadSettings();
    LinkManager::instance()->add(this);
}
void SerialLink::requestReset()
{
    QMutexLocker locker(&this->m_stoppMutex);
    m_reqReset = true;
}

SerialLink::~SerialLink()
{
    disconnect();
    if(m_port) delete m_port;
    m_port = NULL;
}

QList<QString> SerialLink::getCurrentPorts()
{
    m_ports.clear();

    QList<QSerialPortInfo> portList =  QSerialPortInfo::availablePorts();

    if( portList.count() == 0){
        qDebug() << "No Ports Found" << m_ports;
    }

    foreach (const QSerialPortInfo &info, portList)
    {
//        qDebug() << "PortName    : " << info.portName()
//                 << "Description : " << info.description();
//        qDebug() << "Manufacturer: " << info.manufacturer();

        m_ports.append(info.portName());
    }
    return m_ports;
}

void SerialLink::loadSettings()
{
    // Load defaults from settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.sync();
    if (settings.contains("SERIALLINK_COMM_PORT"))
    {
        m_portName = settings.value("SERIALLINK_COMM_PORT").toString();
        m_baud = settings.value("SERIALLINK_COMM_BAUD").toInt();
        m_parity = settings.value("SERIALLINK_COMM_PARITY").toInt();
        m_stopBits = settings.value("SERIALLINK_COMM_STOPBITS").toInt();
        m_dataBits = settings.value("SERIALLINK_COMM_DATABITS").toInt();
        m_flowControl = settings.value("SERIALLINK_COMM_FLOW_CONTROL").toInt();
    }
}

void SerialLink::writeSettings()
{
    // Store settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.setValue("SERIALLINK_COMM_PORT", getPortName());
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
    if (!hardwareConnect()) {
        //Need to error out here.
        emit communicationError(getName(),"Error connecting: " + m_port->errorString());
        disconnect(); // This tidies up and sends the necessary signals
        emit communicationError(tr("Serial Port %1").arg(getPortName()), tr("Cannot read / write data - check physical USB and cable connections."));
        return;
    }

    // Qt way to make clear what a while(1) loop does
    qint64 msecs = QDateTime::currentMSecsSinceEpoch();
    qint64 initialmsecs = QDateTime::currentMSecsSinceEpoch();
    quint64 bytes = 0;
    bool triedreset = false;
    bool triedDTR = false;
    qint64 timeout = 5000;
    int linkErrorCount = 0;

    forever  {
        {
        QMutexLocker locker(&this->m_stoppMutex);
            if(m_stopp) {
                m_stopp = false;
                break; // exit the thread
            }

            if (m_reqReset) {
                m_reqReset = false;
                emit communicationUpdate(getName(),"Reset requested via DTR signal");
                m_port->setDataTerminalReady(true);
                msleep(250);
                m_port->setDataTerminalReady(false);
            }
        }

        if (isConnected() && (linkErrorCount > 100)) {
            qDebug() << "linkErrorCount too high: disconnecting!";
            linkErrorCount = 0;
            emit communicationUpdate(getName(), tr("Disconnecting on too many link errors"));
            disconnect();
        }

        if (m_transmitBuffer.count() > 0) {
            QMutexLocker writeLocker(&m_writeMutex);
            int numWritten = m_port->write(m_transmitBuffer);
            bool txSuccess = m_port->waitForBytesWritten(5);
            if (!txSuccess || (numWritten != m_transmitBuffer.count())) {
                linkErrorCount++;
                qDebug() << "TX Error! wrote" << numWritten << ", asked for " << m_transmitBuffer.count() << "bytes";
            }
            else {
                linkErrorCount = 0;
            }
            m_transmitBuffer =  m_transmitBuffer.remove(0, numWritten);
        }

        //wait n msecs for data to be ready
        //[TODO][BB] lower to SerialLink::poll_interval?
        bool success = m_port->waitForReadyRead(10);

        if (success) {
            QByteArray readData = m_port->readAll();
            while (m_port->waitForReadyRead(10))
                readData += m_port->readAll();
            if (readData.length() > 0) {
                emit bytesReceived(this, readData);
//                qDebug() << "rx of length " << QString::number(readData.length());

                m_bytesRead += readData.length();
                m_bitsReceivedTotal += readData.length() * 8;
                linkErrorCount = 0;
            }
        }
        else {
            linkErrorCount++;
            //qDebug() << "Wait read response timeout" << QTime::currentTime().toString();
        }

        if (bytes != m_bytesRead) { // i.e things are good and data is being read.
            bytes = m_bytesRead;
            msecs = QDateTime::currentMSecsSinceEpoch();
        }
        else {
            if (QDateTime::currentMSecsSinceEpoch() - msecs > timeout) {
                //It's been 10 seconds since the last data came in. Reset and try again
                msecs = QDateTime::currentMSecsSinceEpoch();
                if (msecs - initialmsecs > 25000) {
                    //After initial 25 seconds, timeouts are increased to 30 seconds.
                    //This prevents temporary silences from things like calibration commands
                    //from screwing things up. In all reality, timeouts should be enabled/disabled
                    //for events like that on a case by case basis.
                    //TODO ^^
                    timeout = 30000;
                }
                if (!triedDTR && triedreset) {
                    triedDTR = true;
                    emit communicationUpdate(getName(),"No data to receive on COM port. Attempting to reset via DTR signal");
                    qDebug() << "No data!!! Attempting reset via DTR.";
                    m_port->setDataTerminalReady(true);
                    msleep(250);
                    m_port->setDataTerminalReady(false);
                }
                else if (!triedreset) {
                    qDebug() << "No data!!! Attempting reset via reboot command.";
                    emit communicationUpdate(getName(),"No data to receive on COM port. Assuming possible terminal mode, attempting to reset via \"reboot\" command");
                    m_port->write("reboot\r\n",8);
                    triedreset = true;
                }
                else {
                    emit communicationUpdate(getName(),"No data to receive on COM port....");
                    qDebug() << "No data!!!";
                }
            }
        }
        MG::SLEEP::msleep(SerialLink::poll_interval);
    } // end of forever
    
    if (m_port) { // [TODO][BB] Not sure we need to close the port here
        qDebug() << "Closing Port #"<< __LINE__ << m_port->portName();
        m_port->close();
        delete m_port;
        m_port = NULL;

        emit disconnected();
        emit connected(false);
    }
}

void SerialLink::writeBytes(const char* data, qint64 size)
{
    if(m_port && m_port->isOpen()) {
//        qDebug() << "writeBytes" << m_portName << "attempting to tx " << size << "bytes.";

        QByteArray byteArray(data, size);
        {
            QMutexLocker writeLocker(&m_writeMutex);
            m_transmitBuffer.append(byteArray);
        }

        // Increase write counter
        m_bitsSentTotal += size * 8;

        // Extra debug logging
//            qDebug() << byteArray->toHex();
    } else {
        disconnect();
        // Error occured
        emit communicationError(getName(), tr("Could not send data - link %1 is disconnected!").arg(getName()));
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
    m_dataMutex.lock();
    if(m_port && m_port->isOpen()) {
        const qint64 maxLength = 2048;
        char data[maxLength];
        qint64 numBytes = m_port->bytesAvailable();
        //qDebug() << "numBytes: " << numBytes;

        if(numBytes > 0) {
            /* Read as much data in buffer as possible without overflow */
            if(maxLength < numBytes) numBytes = maxLength;

            m_port->read(data, numBytes);
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
            m_bitsReceivedTotal += numBytes * 8;
        }
    }
    m_dataMutex.unlock();
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 SerialLink::bytesAvailable()
{
    if (m_port) {
        return m_port->bytesAvailable();
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
    qDebug() << "disconnect";
    if (m_port)
        qDebug() << m_port->portName();

    if (isRunning())
    {
        qDebug() << "running so disconnect" << m_port->portName();
        {
            QMutexLocker locker(&m_stoppMutex);
            m_stopp = true;
        }
        wait(); // This will terminate the thread and close the serial port

        emit disconnected(); // [TODO] There are signals from QSerialPort we should use
        emit connected(false);
        return true;
    }

    m_transmitBuffer.clear(); //clear the output buffer to avoid sending garbage at next connect

    qDebug() << "already disconnected";
    return true;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool SerialLink::connect()
{   
    if (isRunning())
        disconnect();
    {
        QMutexLocker locker(&this->m_stoppMutex);
        m_stopp = false;
    }

    start(LowPriority);
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
    if(m_port) {
        qDebug() << "SerialLink:" << QString::number((long)this, 16) << "closing port";
        m_port->close();
        delete m_port;
        m_port = NULL;
    }
    qDebug() << "SerialLink: hardwareConnect to " << m_portName;
    m_port = new QSerialPort(m_portName);

    if (m_port == NULL) {
        emit communicationUpdate(getName(),"Error opening port: " + m_port->errorString());
        return false; // couldn't create serial port.
    }

    QObject::connect(m_port,SIGNAL(aboutToClose()),this,SIGNAL(disconnected()));
    QObject::connect(m_port, SIGNAL(error(SerialLinkPortError_t)),
                     this, SLOT(linkError(SerialLinkPortError_t)));

//    port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);
    m_connectionStartTime = MG::TIME::getGroundTimeNow();

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit communicationUpdate(getName(),"Error opening port: " + m_port->errorString());
        m_port->close();
        return false; // couldn't open serial port
    }

    emit communicationUpdate(getName(),"Opened port!");

    // Need to configure the port
    m_port->setBaudRate(m_baud);
    m_port->setDataBits(static_cast<QSerialPort::DataBits>(m_dataBits));
    m_port->setFlowControl(static_cast<QSerialPort::FlowControl>(m_flowControl));
    m_port->setStopBits(static_cast<QSerialPort::StopBits>(m_stopBits));
    m_port->setParity(static_cast<QSerialPort::Parity>(m_parity));

    emit connected();
    emit connected(true);

    qDebug() << "CONNECTING LINK: " << __FILE__ << __LINE__ << "with settings" << m_port->portName()
             << getBaudRate() << getDataBits() << getParityType() << getStopBits();

    writeSettings();

    return true; // successful connection
}

void SerialLink::linkError(SerialLinkPortError_t error)
{
    qDebug() << error;
}


/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool SerialLink::isConnected() const
{

    if (m_port) {
        bool isConnected = m_port->isOpen();
//        qDebug() << "SerialLink #" << __LINE__ << ":"<<  m_port->portName()
//                 << " isConnected =" << QString::number(isConnected);
        return isConnected;
    } else {
//        qDebug() << "SerialLink #" << __LINE__ << ":" <<  m_portName
//                 << " isConnected = NULL";
        return false;
    }
}

int SerialLink::getId() const
{
    return m_id;
}

QString SerialLink::getName() const
{
    return m_portName;
}

/**
  * This function maps baud rate constants to numerical equivalents.
  * It relies on the mapping given in qportsettings.h from the QSerialPort library.
  */
qint64 SerialLink::getNominalDataRate() const
{
    int baudRate;
    if (m_port) {
        baudRate = m_port->baudRate();
    } else {
        baudRate = m_baud;
    }
    qint64 dataRate;
    switch (baudRate)
    {
        case QSerialPort::Baud1200:
            dataRate = 1200;
            break;
        case QSerialPort::Baud2400:
            dataRate = 2400;
            break;
        case QSerialPort::Baud4800:
            dataRate = 4800;
            break;
        case QSerialPort::Baud9600:
            dataRate = 9600;
            break;
        case QSerialPort::Baud19200:
            dataRate = 19200;
            break;
        case QSerialPort::Baud38400:
            dataRate = 38400;
            break;
        case QSerialPort::Baud57600:
            dataRate = 57600;
            break;
        case QSerialPort::Baud115200:
            dataRate = 115200;
            break;
            // Otherwise do nothing.
        case QSerialPort::UnknownBaud:
        default:
            dataRate = -1;
            break;
    }
    return dataRate;
}

qint64 SerialLink::getTotalUpstream()
{
    m_statisticsMutex.lock();
    return m_bitsSentTotal / ((MG::TIME::getGroundTimeNow() - m_connectionStartTime) / 1000);
    m_statisticsMutex.unlock();
}

qint64 SerialLink::getCurrentUpstream()
{
    return 0; // TODO
}

qint64 SerialLink::getMaxUpstream()
{
    return 0; // TODO
}

qint64 SerialLink::getBitsSent() const
{
    return m_bitsSentTotal;
}

qint64 SerialLink::getBitsReceived() const
{
    return m_bitsReceivedTotal;
}

qint64 SerialLink::getTotalDownstream()
{
    m_statisticsMutex.lock();
    return m_bitsReceivedTotal / ((MG::TIME::getGroundTimeNow() - m_connectionStartTime) / 1000);
    m_statisticsMutex.unlock();
}

qint64 SerialLink::getCurrentDownstream()
{
    return 0; // TODO
}

qint64 SerialLink::getMaxDownstream()
{
    return 0; // TODO
}

bool SerialLink::isFullDuplex() const
{
    /* Serial connections are always half duplex */
    return false;
}

int SerialLink::getLinkQuality() const
{
    /* This feature is not supported with this interface */
    return -1;
}

QString SerialLink::getPortName() const
{
    return m_portName;
}

// We should replace the accessors below with one to get the QSerialPort

int SerialLink::getBaudRate() const
{
    return getNominalDataRate();
}

int SerialLink::getBaudRateType() const
{
    int baudRate;
    if (m_port) {
        baudRate = m_port->baudRate();
    } else {
        baudRate = m_baud;
    }
    return baudRate;
}

int SerialLink::getFlowType() const
{
    int flowControl;
    if (m_port) {
        flowControl = m_port->flowControl();
    } else {
        flowControl = m_flowControl;
    }
    return flowControl;
}

int SerialLink::getParityType() const
{
    int parity;
    if (m_port) {
        parity = m_port->parity();
    } else {
        parity = m_parity;
    }
    return parity;
}

int SerialLink::getDataBitsType() const
{
    int dataBits;
    if (m_port) {
        dataBits = m_port->dataBits();
    } else {
        dataBits = m_dataBits;
    }
    return dataBits;
}

int SerialLink::getStopBitsType() const
{
    int stopBits;
    if (m_port) {
        stopBits = m_port->stopBits();
    } else {
        stopBits = m_stopBits;
    }
    return stopBits;
}

int SerialLink::getDataBits() const
{
    int ret;
    int dataBits;
    if (m_port) {
        dataBits = m_port->dataBits();
    } else {
        dataBits = m_dataBits;
    }

    switch (dataBits) {
    case QSerialPort::Data5:
        ret = 5;
        break;
    case QSerialPort::Data6:
        ret = 6;
        break;
    case QSerialPort::Data7:
        ret = 7;
        break;
    case QSerialPort::Data8:
        ret = 8;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

int SerialLink::getStopBits() const
{
    int stopBits;
    if (m_port) {
        stopBits = m_port->stopBits();
    } else {
        stopBits = m_stopBits;
    }
    int ret = -1;
    switch (stopBits) {
    case QSerialPort::OneStop:
        ret = 1;
        break;
    case QSerialPort::TwoStop:
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
    qDebug() << "current portName " << m_portName;
    qDebug() << "setPortName to " << portName;
    bool accepted = false;
    if ((portName != m_portName)
            && (portName.trimmed().length() > 0)) {
        m_portName = portName.trimmed();
//        m_name = tr("serial port ") + portName.trimmed(); // [TODO] Do we need this?
        if(m_port)
            m_port->setPortName(portName);

        emit nameChanged(m_portName); // [TODO] maybe we can eliminate this
        emit updateLink(this);
        return accepted;
    }
    return false;
}


bool SerialLink::setBaudRateType(int rateIndex)
{
    Q_ASSERT_X(m_port != NULL, "setBaudRateType", "m_port is NULL");
    // These minimum and maximum baud rates were based on those enumerated in qserialport.h
    bool result;
    const int minBaud = (int)QSerialPort::Baud1200;
    const int maxBaud = (int)QSerialPort::Baud115200;

    if (m_port && (rateIndex >= minBaud && rateIndex <= maxBaud))
    {
        result = m_port->setBaudRate(static_cast<QSerialPort::BaudRate>(rateIndex));
        emit updateLink(this);
        return result;
    }

    return false;
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
    bool accepted = false;
    if (rate != m_baud) {
        m_baud = rate;
        accepted = true;
        if (m_port)
            accepted = m_port->setBaudRate(rate);
        emit updateLink(this);
    }
    return accepted;
}

bool SerialLink::setFlowType(int flow)
{
    bool accepted = false;
    if (flow != m_flowControl) {
        m_flowControl = static_cast<QSerialPort::FlowControl>(flow);
        accepted = true;
        if (m_port)
            accepted = m_port->setFlowControl(static_cast<QSerialPort::FlowControl>(flow));
        emit updateLink(this);
    }
    return accepted;
}

bool SerialLink::setParityType(int parity)
{
    bool accepted = false;
    if (parity != m_parity) {
        m_parity = static_cast<QSerialPort::Parity>(parity);
        accepted = true;
        if (m_port) {
            switch (parity) {
                case QSerialPort::NoParity:
                accepted = m_port->setParity(QSerialPort::NoParity);
                break;
                case 1: // Odd Parity setting for backwards compatibilty
                    accepted = m_port->setParity(QSerialPort::OddParity);
                    break;
                case QSerialPort::EvenParity:
                    accepted = m_port->setParity(QSerialPort::EvenParity);
                    break;
                case QSerialPort::OddParity:
                    accepted = m_port->setParity(QSerialPort::OddParity);
                    break;
                default:
                    // If none of the above cases matches, there must be an error
                    accepted = false;
                    break;
                }
            emit updateLink(this);
        }
    }
    return accepted;
}


bool SerialLink::setDataBits(int dataBits)
{
    bool accepted = false;
    if (dataBits != m_dataBits) {
        m_dataBits = static_cast<QSerialPort::DataBits>(dataBits);
        accepted = true;
        if (m_port)
            accepted = m_port->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
        emit updateLink(this);
    }
    return accepted;
}

bool SerialLink::setStopBits(int stopBits)
{
    // Note 3 is OneAndAHalf stopbits.
    bool accepted = false;
    if (stopBits != m_stopBits) {
        m_stopBits = static_cast<QSerialPort::StopBits>(stopBits);
        accepted = true;
        if (m_port)
            accepted = m_port->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
        emit updateLink(this);
    }
    return accepted;
}

bool SerialLink::setDataBitsType(int dataBits)
{
    bool accepted = false;
    if (dataBits != m_dataBits) {
        m_dataBits = static_cast<QSerialPort::DataBits>(dataBits);
        accepted = true;
        if (m_port)
            accepted = m_port->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
        emit updateLink(this);
    }
    return accepted;
}

bool SerialLink::setStopBitsType(int stopBits)
{
    bool accepted = false;
    if (stopBits != m_stopBits) {
        m_stopBits = static_cast<QSerialPort::StopBits>(stopBits);
        accepted = true;
        if (m_port)
            accepted = m_port->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
        emit updateLink(this);
    }
    return accepted;
}
