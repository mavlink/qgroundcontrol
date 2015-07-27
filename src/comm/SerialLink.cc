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

#ifdef __android__
#include "qserialport.h"
#include "qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

#include "SerialLink.h"
#include "QGC.h"
#include "MG.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SerialLinkLog, "SerialLinkLog")

SerialLink::SerialLink(SerialConfiguration* config)
{
    _bytesRead = 0;
    _port     = Q_NULLPTR;
    _stopp    = false;
    _reqReset = false;
    Q_ASSERT(config != NULL);
    _config = config;
    _config->setLink(this);

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
}

bool SerialLink::_isBootloader()
{
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
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

void SerialLink::writeBytes(const char* data, qint64 size)
{
    if(_port && _port->isOpen()) {
        _logOutputDataRate(size, QDateTime::currentMSecsSinceEpoch());
        _port->write(data, size);
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

        if (numBytes > 0) {
            /* Read as much data in buffer as possible without overflow */
            if(maxLength < numBytes) numBytes = maxLength;

            _logInputDataRate(numBytes, QDateTime::currentMSecsSinceEpoch());
            
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
    if (_port) {
        _port->close();
        delete _port;
        _port = NULL;
    }
    
#ifdef __android__
    LinkManager::instance()->suspendConfigurationUpdates(false);
#endif
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

    _disconnect();
    
#ifdef __android__
    LinkManager::instance()->suspendConfigurationUpdates(true);
#endif
    
    // Initialize the connection
    if (!_hardwareConnect(_type)) {
        // Need to error out here.
        QString err("Could not create port.");
        if (_port) {
            err = _port->errorString();
        }
        _emitLinkError("Error connecting: " + err);
        return false;
    }
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

    // We need to catch this signal and then emit disconnected. You can't connect
    // signal to signal otherwise disonnected will have the wrong QObject::Sender
    QObject::connect(_port, SIGNAL(aboutToClose()), this, SLOT(_rerouteDisconnected()));
    QObject::connect(_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(linkError(QSerialPort::SerialPortError)));
    QObject::connect(_port, &QIODevice::readyRead, this, &SerialLink::_readBytes);

    //  port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);

    // TODO This needs a bit of TLC still...

    // After the bootloader times out, it still can take a second or so for the Pixhawk USB driver to come up and make
    // the port available for open. So we retry a few times to wait for it.
#ifdef __android__
    _port->open(QIODevice::ReadWrite);
#else
    for (int openRetries = 0; openRetries < 4; openRetries++) {
        if (!_port->open(QIODevice::ReadWrite)) {
            qCDebug(SerialLinkLog) << "Port open failed, retrying";
            QGC::SLEEP::msleep(500);
        } else {
            break;
        }
    }
#endif
    if (!_port->isOpen() ) {
        emit communicationUpdate(getName(),"Error opening port: " + _port->errorString());
        _port->close();
        delete _port;
        _port = NULL;
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

void SerialLink::_readBytes(void)
{
    qint64 byteCount = _port->bytesAvailable();
    if (byteCount)
    {
        QByteArray buffer;
        buffer.resize(byteCount);
        _port->read(buffer.data(), buffer.size());
        emit bytesReceived(this, buffer);
    }
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
    if (_port) {
        _port->setBaudRate      (_config->baud());
        _port->setDataBits      (static_cast<QSerialPort::DataBits>    (_config->dataBits()));
        _port->setFlowControl   (static_cast<QSerialPort::FlowControl> (_config->flowControl()));
        _port->setStopBits      (static_cast<QSerialPort::StopBits>    (_config->stopBits()));
        _port->setParity        (static_cast<QSerialPort::Parity>      (_config->parity()));
    }
}

void SerialLink::_rerouteDisconnected(void)
{
    emit disconnected();
}

void SerialLink::_emitLinkError(const QString& errorMsg)
{
    QString msg("Error on link %1. %2");
    qDebug() << errorMsg;
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
