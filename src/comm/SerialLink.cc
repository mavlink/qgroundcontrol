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
#include <QSerialPort>
#include <QSerialPortInfo>

#include "SerialLink.h"
#include "QGC.h"
#include "MG.h"

Q_LOGGING_CATEGORY(SerialLinkLog, "SerialLinkLog")

SerialLink::SerialLink(SerialConfiguration* config)
{
    _bytesRead = 0;
    _port     = Q_NULLPTR;
    _stopp    = false;
    _reqReset = false;
    Q_ASSERT(config != NULL);
    _config = config;
    _config->setLink(this);
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);
    // Set unique ID and add link to the list of links
    _id = getNextLinkId();

    qCDebug(SerialLinkLog) << "Create SerialLink " << config->portName() << config->baud() << config->flowControl()
             << config->parity() << config->dataBits() << config->stopBits();
    qCDebug(SerialLinkLog) << "portName: " << config->portName();
}

void SerialLink::requestReset()
{
    QMutexLocker locker(&this->_stoppMutex);
    _reqReset = true;
}

SerialLink::~SerialLink()
{
    // Disconnect link from configuration
    _config->setLink(NULL);
    _disconnect();
    if(_port) delete _port;
    _port = NULL;
    // Tell the thread to exit
    quit();
    // Wait for it to exit
    wait();
}

bool SerialLink::_isBootloader()
{
    QList<QSerialPortInfo> portList =  QSerialPortInfo::availablePorts();
    if( portList.count() == 0){
        return false;
    }
    foreach (const QSerialPortInfo &info, portList)
    {
        qCDebug(SerialLinkLog) << "PortName    : " << info.portName() << "Description : " << info.description();
        qCDebug(SerialLinkLog) << "Manufacturer: " << info.manufacturer();
        if (info.portName().trimmed() == _config->portName() &&
                (info.description().toLower().contains("bootloader") ||
                 info.description().toLower().contains("px4 bl") ||
                 info.description().toLower().contains("px4 fmu v1.6"))) {
            qCDebug(SerialLinkLog) << "BOOTLOADER FOUND";
            return true;
       }
    }
    // Not found
    return false;
}

/**
 * @brief Runs the thread
 *
 **/
void SerialLink::run()
{
    // Initialize the connection
    if (!_hardwareConnect(_type)) {
        // Need to error out here.
        QString err("Could not create port.");
        if (_port) {
            err = _port->errorString();
        }
        _emitLinkError("Error connecting: " + err);
        return;
    }

    qint64  msecs = QDateTime::currentMSecsSinceEpoch();
    qint64  initialmsecs = QDateTime::currentMSecsSinceEpoch();
    quint64 bytes = 0;
    qint64  timeout = 5000;
    int linkErrorCount = 0;

    // Qt way to make clear what a while(1) loop does
    forever {
        {
            QMutexLocker locker(&this->_stoppMutex);
            if (_stopp) {
                _stopp = false;
                break; // exit the thread
            }
        }

        // Write all our buffered data out the serial port.
        if (_transmitBuffer.count() > 0) {
            _writeMutex.lock();
            int numWritten = _port->write(_transmitBuffer);
            bool txSuccess = _port->flush();
            txSuccess |= _port->waitForBytesWritten(10);
            if (!txSuccess || (numWritten != _transmitBuffer.count())) {
                linkErrorCount++;
                qCDebug(SerialLinkLog) << "TX Error! written:" << txSuccess << "wrote" << numWritten << ", asked for " << _transmitBuffer.count() << "bytes";
            }
            else {

                // Since we were successful, reset out error counter.
                linkErrorCount = 0;
            }

            // Now that we transmit all of the data in the transmit buffer, flush it.
            _transmitBuffer = _transmitBuffer.remove(0, numWritten);
            _writeMutex.unlock();

            // Log this written data for this timestep. If this value ends up being 0 due to
            // write() failing, that's what we want as well.
            QMutexLocker dataRateLocker(&dataRateMutex);
            logDataRateToBuffer(outDataWriteAmounts, outDataWriteTimes, &outDataIndex, numWritten, QDateTime::currentMSecsSinceEpoch());
        }

        //wait n msecs for data to be ready
        //[TODO][BB] lower to SerialLink::poll_interval?
        _dataMutex.lock();
        bool success = _port->waitForReadyRead(20);

        if (success) {
            QByteArray readData = _port->readAll();
            while (_port->waitForReadyRead(10))
                readData += _port->readAll();
            _dataMutex.unlock();
            if (readData.length() > 0) {
                emit bytesReceived(this, readData);

                // Log this data reception for this timestep
                QMutexLocker dataRateLocker(&dataRateMutex);
                logDataRateToBuffer(inDataWriteAmounts, inDataWriteTimes, &inDataIndex, readData.length(), QDateTime::currentMSecsSinceEpoch());

                // Track the total amount of data read.
                _bytesRead += readData.length();
                linkErrorCount = 0;
            }
        }
        else {
            _dataMutex.unlock();
            linkErrorCount++;
        }

        if (bytes != _bytesRead) { // i.e things are good and data is being read.
            bytes = _bytesRead;
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
            }
        }
        QGC::SLEEP::msleep(SerialLink::poll_interval);
    } // end of forever

    if (_port) {
        qCDebug(SerialLinkLog) << "Closing Port #" << __LINE__ << _port->portName();
        _port->close();
        delete _port;
        _port = NULL;
    }
}

void SerialLink::writeBytes(const char* data, qint64 size)
{
    if(_port && _port->isOpen()) {
        QByteArray byteArray(data, size);
        _writeMutex.lock();
        _transmitBuffer.append(byteArray);
        _writeMutex.unlock();
    } else {
        // Error occured
        _emitLinkError(tr("Could not send data - link %1 is disconnected!").arg(getName()));
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
    if(_port && _port->isOpen()) {
        const qint64 maxLength = 2048;
        char data[maxLength];
        _dataMutex.lock();
        qint64 numBytes = _port->bytesAvailable();

        if(numBytes > 0) {
            /* Read as much data in buffer as possible without overflow */
            if(maxLength < numBytes) numBytes = maxLength;

            _port->read(data, numBytes);
            QByteArray b(data, numBytes);
            emit bytesReceived(this, b);
        }
        _dataMutex.unlock();
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool SerialLink::_disconnect(void)
{
    if (isRunning())
    {
        {
            QMutexLocker locker(&_stoppMutex);
            _stopp = true;
        }
        wait(); // This will terminate the thread and close the serial port
        return true;
    }
    _transmitBuffer.clear(); //clear the output buffer to avoid sending garbage at next connect
    qCDebug(SerialLinkLog) << "Already disconnected";
    return true;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool SerialLink::_connect(void)
{
    qCDebug(SerialLinkLog) << "CONNECT CALLED";
    if (isRunning())
        _disconnect();
    {
        QMutexLocker locker(&this->_stoppMutex);
        _stopp = false;
    }
    start(HighPriority);
    return true;
}

/**
 * @brief This function is called indirectly by the _connect() call.
 *
 * The _connect() function starts the thread and indirectly calls this method.
 *
 * @return True if the connection could be established, false otherwise
 * @see _connect() For the right function to establish the connection.
 **/
bool SerialLink::_hardwareConnect(QString &type)
{
    if (_port) {
        qCDebug(SerialLinkLog) << "SerialLink:" << QString::number((long)this, 16) << "closing port";
        _port->close();
        QGC::SLEEP::usleep(50000);
        delete _port;
        _port = NULL;
    }

    qCDebug(SerialLinkLog) << "SerialLink: hardwareConnect to " << _config->portName();

    // If we are in the Pixhawk bootloader code wait for it to timeout
    if (_isBootloader()) {
        qCDebug(SerialLinkLog) << "Not connecting to a bootloader, waiting for 2nd chance";
        const unsigned retry_limit = 12;
        unsigned retries;
        for (retries = 0; retries < retry_limit; retries++) {
            if (!_isBootloader()) {
                QGC::SLEEP::msleep(500);
                break;
            }
            QGC::SLEEP::msleep(500);
        }
        // Check limit
        if (retries == retry_limit) {
            // bail out
            qWarning() << "Timeout waiting for something other than booloader";
            return false;
        }
    }

    _port = new QSerialPort(_config->portName());
    if (!_port) {
        emit communicationUpdate(getName(),"Error opening port: " + _config->portName());
        return false; // couldn't create serial port.
    }
    _port->moveToThread(this);

    // We need to catch this signal and then emit disconnected. You can't connect
    // signal to signal otherwise disonnected will have the wrong QObject::Sender
    QObject::connect(_port, SIGNAL(aboutToClose()), this, SLOT(_rerouteDisconnected()));
    QObject::connect(_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(linkError(QSerialPort::SerialPortError)));

    //  port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);

    // TODO This needs a bit of TLC still...

    // After the bootloader times out, it still can take a second or so for the Pixhawk USB driver to come up and make
    // the port available for open. So we retry a few times to wait for it.
    for (int openRetries = 0; openRetries < 4; openRetries++) {
        if (!_port->open(QIODevice::ReadWrite)) {
            qCDebug(SerialLinkLog) << "Port open failed, retrying";
            QGC::SLEEP::msleep(500);
        } else {
            break;
        }
    }
    if (!_port->isOpen() ) {
        emit communicationUpdate(getName(),"Error opening port: " + _port->errorString());
        _port->close();
        return false; // couldn't open serial port
    }

    qCDebug(SerialLinkLog) << "Configuring port";
    _port->setBaudRate     (_config->baud());
    _port->setDataBits     (static_cast<QSerialPort::DataBits>     (_config->dataBits()));
    _port->setFlowControl  (static_cast<QSerialPort::FlowControl>  (_config->flowControl()));
    _port->setStopBits     (static_cast<QSerialPort::StopBits>     (_config->stopBits()));
    _port->setParity       (static_cast<QSerialPort::Parity>       (_config->parity()));

    emit communicationUpdate(getName(), "Opened port!");
    emit connected();

    qCDebug(SerialLinkLog) << "CONNECTING LINK: " << __FILE__ << __LINE__ << "type:" << type << "with settings" << _config->portName()
             << _config->baud() << _config->dataBits() << _config->parity() << _config->stopBits();

    return true; // successful connection
}

void SerialLink::linkError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        // You can use the following qDebug output as needed during development. Make sure to comment it back out
        // when you are done. The reason for this is that this signal is very noisy. For example if you try to
        // connect to a PixHawk before it is ready to accept the connection it will output a continuous stream
        // of errors until the Pixhawk responds.
        //qCDebug(SerialLinkLog) << "SerialLink::linkError" << error;
    }
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool SerialLink::isConnected() const
{
    bool isConnected = false;

    if (_port) {
        isConnected = _port->isOpen();
    }
    
    return isConnected;
}

int SerialLink::getId() const
{
    return _id;
}

QString SerialLink::getName() const
{
    return _config->portName();
}

/**
  * This function maps baud rate constants to numerical equivalents.
  * It relies on the mapping given in qportsettings.h from the QSerialPort library.
  */
qint64 SerialLink::getConnectionSpeed() const
{
    int baudRate;
    if (_port) {
        baudRate = _port->baudRate();
    } else {
        baudRate = _config->baud();
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
        default:
            dataRate = -1;
            break;
    }
    return dataRate;
}

void SerialLink::_resetConfiguration()
{
    bool somethingChanged = false;
    if (_port) {
        somethingChanged = _port->setBaudRate     (_config->baud());
        somethingChanged |= _port->setDataBits    (static_cast<QSerialPort::DataBits>    (_config->dataBits()));
        somethingChanged |= _port->setFlowControl (static_cast<QSerialPort::FlowControl> (_config->flowControl()));
        somethingChanged |= _port->setStopBits    (static_cast<QSerialPort::StopBits>    (_config->stopBits()));
        somethingChanged |= _port->setParity      (static_cast<QSerialPort::Parity>      (_config->parity()));
    }
    if(somethingChanged) {
        qCDebug(SerialLinkLog) << "Reconfiguring port";
        emit updateLink(this);
    }
}

void SerialLink::_rerouteDisconnected(void)
{
    emit disconnected();
}

void SerialLink::_emitLinkError(const QString& errorMsg)
{
    QString msg("Error on link %1. %2");
    emit communicationError(tr("Link Error"), msg.arg(getName()).arg(errorMsg));
}

LinkConfiguration* SerialLink::getLinkConfiguration()
{
    return _config;
}

//--------------------------------------------------------------------------
//-- SerialConfiguration

SerialConfiguration::SerialConfiguration(const QString& name) : LinkConfiguration(name)
{
    _baud       = 57600;
    _flowControl= QSerialPort::NoFlowControl;
    _parity     = QSerialPort::NoParity;
    _dataBits   = 8;
    _stopBits   = 1;
}

SerialConfiguration::SerialConfiguration(SerialConfiguration* copy) : LinkConfiguration(copy)
{
    _baud       = copy->baud();
    _flowControl= copy->flowControl();
    _parity     = copy->parity();
    _dataBits   = copy->dataBits();
    _stopBits   = copy->stopBits();
    _portName   = copy->portName();
}

void SerialConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    SerialConfiguration* ssource = dynamic_cast<SerialConfiguration*>(source);
    Q_ASSERT(ssource != NULL);
    _baud       = ssource->baud();
    _flowControl= ssource->flowControl();
    _parity     = ssource->parity();
    _dataBits   = ssource->dataBits();
    _stopBits   = ssource->stopBits();
    _portName   = ssource->portName();
}

void SerialConfiguration::updateSettings()
{
    if(_link) {
        SerialLink* serialLink = dynamic_cast<SerialLink*>(_link);
        if(serialLink) {
            serialLink->_resetConfiguration();
        }
    }
}

void SerialConfiguration::setBaud(int baud)
{
    _baud = baud;
}

void SerialConfiguration::setDataBits(int databits)
{
    _dataBits = databits;
}

void SerialConfiguration::setFlowControl(int flowControl)
{
    _flowControl = flowControl;
}

void SerialConfiguration::setStopBits(int stopBits)
{
    _stopBits = stopBits;
}

void SerialConfiguration::setParity(int parity)
{
    _parity = parity;
}

void SerialConfiguration::setPortName(const QString& portName)
{
    // No effect on a running connection
    QString pname = portName.trimmed();
    if (!pname.isEmpty() && pname != _portName) {
        _portName = pname;
    }
}

void SerialConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("baud",        _baud);
    settings.setValue("dataBits",    _dataBits);
    settings.setValue("flowControl", _flowControl);
    settings.setValue("stopBits",    _stopBits);
    settings.setValue("parity",      _parity);
    settings.setValue("portName",    _portName);
    settings.endGroup();
}

void SerialConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    if(settings.contains("baud"))        _baud         = settings.value("baud").toInt();
    if(settings.contains("dataBits"))    _dataBits     = settings.value("dataBits").toInt();
    if(settings.contains("flowControl")) _flowControl  = settings.value("flowControl").toInt();
    if(settings.contains("stopBits"))    _stopBits     = settings.value("stopBits").toInt();
    if(settings.contains("parity"))      _parity       = settings.value("parity").toInt();
    if(settings.contains("portName"))    _portName     = settings.value("portName").toString();
    settings.endGroup();
}

QList<QString> SerialConfiguration::getCurrentPorts()
{
    QList<QString> ports;
    QList<QSerialPortInfo> portList =  QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &info, portList)
    {
        ports.append(info.portName());
    }
    return ports;
}
