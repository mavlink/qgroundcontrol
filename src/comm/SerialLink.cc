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
#else
#include <QSerialPort>
#endif

#include "SerialLink.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "QGCSerialPortInfo.h"

QGC_LOGGING_CATEGORY(SerialLinkLog, "SerialLinkLog")

static QStringList kSupportedBaudRates;

SerialLink::SerialLink(SharedLinkConfigurationPointer& config, bool isPX4Flow)
    : LinkInterface(config, isPX4Flow)
    , _port(nullptr)
    , _bytesRead(0)
    , _stopp(false)
    , _reqReset(false)
    , _serialConfig(qobject_cast<SerialConfiguration*>(config.data()))
{
    if (!_serialConfig) {
        qWarning() << "Internal error";
        return;
    }

    qCDebug(SerialLinkLog) << "Create SerialLink " << _serialConfig->portName() << _serialConfig->baud() << _serialConfig->flowControl()
                           << _serialConfig->parity() << _serialConfig->dataBits() << _serialConfig->stopBits();
    qCDebug(SerialLinkLog) << "portName: " << _serialConfig->portName();
}

void SerialLink::requestReset()
{
    QMutexLocker locker(&this->_stoppMutex);
    _reqReset = true;
}

SerialLink::~SerialLink()
{
    _disconnect();
}

bool SerialLink::_isBootloader()
{
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if( portList.count() == 0){
        return false;
    }
    for (const QSerialPortInfo &info: portList)
    {
        qCDebug(SerialLinkLog) << "PortName    : " << info.portName() << "Description : " << info.description();
        qCDebug(SerialLinkLog) << "Manufacturer: " << info.manufacturer();
        if (info.portName().trimmed() == _serialConfig->portName() &&
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

void SerialLink::_writeBytes(const QByteArray data)
{
    if(_port && _port->isOpen()) {
        emit bytesSent(this, data);
        _logOutputDataRate(data.size(), QDateTime::currentMSecsSinceEpoch());
        _port->write(data);
    } else {
        // Error occurred
        qWarning() << "Serial port not writeable";
        _emitLinkError(tr("Could not send data - link %1 is disconnected!").arg(getName()));
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
void SerialLink::_disconnect(void)
{
    if (_port) {
        _port->close();
        _port->deleteLater();
        _port = nullptr;
    }

#ifdef __android__
    qgcApp()->toolbox()->linkManager()->suspendConfigurationUpdates(false);
#endif
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
    qgcApp()->toolbox()->linkManager()->suspendConfigurationUpdates(true);
#endif

    QSerialPort::SerialPortError    error;
    QString                         errorString;

    // Initialize the connection
    if (!_hardwareConnect(error, errorString)) {
        if (qgcApp()->toolbox()->linkManager()->isAutoconnectLink(this)) {
            // Be careful with spitting out open error related to trying to open a busy port using autoconnect
            if (error == QSerialPort::PermissionError) {
                // Device already open, ignore and fail connect
                return false;
            }
        }

        _emitLinkError(tr("Error connecting: Could not create port. %1").arg(errorString));
        return false;
    }
    return true;
}

/// Performs the actual hardware port connection.
///     @param[out] error if failed
///     @param[out] error string if failed
/// @return success/fail
bool SerialLink::_hardwareConnect(QSerialPort::SerialPortError& error, QString& errorString)
{
    if (_port) {
        qCDebug(SerialLinkLog) << "SerialLink:" << QString::number((qulonglong)this, 16) << "closing port";
        _port->close();

        // Wait 50 ms while continuing to run the event queue
        for (unsigned i = 0; i < 10; i++) {
            QGC::SLEEP::usleep(5000);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        delete _port;
        _port = nullptr;
    }

    qCDebug(SerialLinkLog) << "SerialLink: hardwareConnect to " << _serialConfig->portName();

    // If we are in the Pixhawk bootloader code wait for it to timeout
    if (_isBootloader()) {
        qCDebug(SerialLinkLog) << "Not connecting to a bootloader, waiting for 2nd chance";
        const unsigned retry_limit = 12;
        unsigned retries;

        for (retries = 0; retries < retry_limit; retries++) {
            if (!_isBootloader()) {
                // Wait 500 ms while continuing to run the event loop
                for (unsigned i = 0; i < 100; i++) {
                    QGC::SLEEP::msleep(5);
                    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
                }
                break;
            }

            // Wait 500 ms while continuing to run the event loop
            for (unsigned i = 0; i < 100; i++) {
                QGC::SLEEP::msleep(5);
                qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }
        // Check limit
        if (retries == retry_limit) {
            // bail out
            qWarning() << "Timeout waiting for something other than booloader";
            return false;
        }
    }

    _port = new QSerialPort(_serialConfig->portName(), this);

    QObject::connect(_port, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
                     this, &SerialLink::linkError);
    QObject::connect(_port, &QIODevice::readyRead, this, &SerialLink::_readBytes);

    //  port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);

    // TODO This needs a bit of TLC still...

    // After the bootloader times out, it still can take a second or so for the Pixhawk USB driver to come up and make
    // the port available for open. So we retry a few times to wait for it.
#ifdef __android__
    _port->open(QIODevice::ReadWrite);
#else

    // Try to open the port three times
    for (int openRetries = 0; openRetries < 3; openRetries++) {
        if (!_port->open(QIODevice::ReadWrite)) {
            qCDebug(SerialLinkLog) << "Port open failed, retrying";
            // Wait 250 ms while continuing to run the event loop
            for (unsigned i = 0; i < 50; i++) {
                QGC::SLEEP::msleep(5);
                qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        } else {
            break;
        }
    }
#endif
    if (!_port->isOpen() ) {
        qDebug() << "open failed" << _port->errorString() << _port->error() << getName() << qgcApp()->toolbox()->linkManager()->isAutoconnectLink(this);
        error = _port->error();
        errorString = _port->errorString();
        emit communicationUpdate(getName(), tr("Error opening port: %1").arg(_port->errorString()));
        _port->close();
        delete _port;
        _port = nullptr;
        return false; // couldn't open serial port
    }

    _port->setDataTerminalReady(true);

    qCDebug(SerialLinkLog) << "Configuring port";
    _port->setBaudRate     (_serialConfig->baud());
    _port->setDataBits     (static_cast<QSerialPort::DataBits>     (_serialConfig->dataBits()));
    _port->setFlowControl  (static_cast<QSerialPort::FlowControl>  (_serialConfig->flowControl()));
    _port->setStopBits     (static_cast<QSerialPort::StopBits>     (_serialConfig->stopBits()));
    _port->setParity       (static_cast<QSerialPort::Parity>       (_serialConfig->parity()));

    emit communicationUpdate(getName(), "Opened port!");
    emit connected();

    qCDebug(SerialLinkLog) << "Connection SeriaLink: " << "with settings" << _serialConfig->portName()
                           << _serialConfig->baud() << _serialConfig->dataBits() << _serialConfig->parity() << _serialConfig->stopBits();

    return true; // successful connection
}

void SerialLink::_readBytes(void)
{
    if (_port && _port->isOpen()) {
        qint64 byteCount = _port->bytesAvailable();
        if (byteCount) {
            QByteArray buffer;
            buffer.resize(byteCount);
            _port->read(buffer.data(), buffer.size());
            emit bytesReceived(this, buffer);
        }
    } else {
        // Error occurred
        qWarning() << "Serial port not readable";
        _emitLinkError(tr("Could not read data - link %1 is disconnected!").arg(getName()));
    }
}

void SerialLink::linkError(QSerialPort::SerialPortError error)
{
    switch (error) {
    case QSerialPort::NoError:
        break;
    case QSerialPort::ResourceError:
        emit connectionRemoved(this);
        break;
    default:
        // You can use the following qDebug output as needed during development. Make sure to comment it back out
        // when you are done. The reason for this is that this signal is very noisy. For example if you try to
        // connect to a PixHawk before it is ready to accept the connection it will output a continuous stream
        // of errors until the Pixhawk responds.
        //qCDebug(SerialLinkLog) << "SerialLink::linkError" << error;
        break;
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
    return _serialConfig->name();
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
        baudRate = _serialConfig->baud();
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
        _port->setBaudRate      (_serialConfig->baud());
        _port->setDataBits      (static_cast<QSerialPort::DataBits>    (_serialConfig->dataBits()));
        _port->setFlowControl   (static_cast<QSerialPort::FlowControl> (_serialConfig->flowControl()));
        _port->setStopBits      (static_cast<QSerialPort::StopBits>    (_serialConfig->stopBits()));
        _port->setParity        (static_cast<QSerialPort::Parity>      (_serialConfig->parity()));
    }
}

void SerialLink::_emitLinkError(const QString& errorMsg)
{
    QString msg("Error on link %1. %2");
    qDebug() << errorMsg;
    emit communicationError(tr("Link Error"), msg.arg(getName()).arg(errorMsg));
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
    _usbDirect  = false;
}

SerialConfiguration::SerialConfiguration(SerialConfiguration* copy) : LinkConfiguration(copy)
{
    _baud               = copy->baud();
    _flowControl        = copy->flowControl();
    _parity             = copy->parity();
    _dataBits           = copy->dataBits();
    _stopBits           = copy->stopBits();
    _portName           = copy->portName();
    _portDisplayName    = copy->portDisplayName();
    _usbDirect          = copy->_usbDirect;
}

void SerialConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    auto* ssource = qobject_cast<SerialConfiguration*>(source);
    if (ssource) {
        _baud               = ssource->baud();
        _flowControl        = ssource->flowControl();
        _parity             = ssource->parity();
        _dataBits           = ssource->dataBits();
        _stopBits           = ssource->stopBits();
        _portName           = ssource->portName();
        _portDisplayName    = ssource->portDisplayName();
        _usbDirect          = ssource->_usbDirect;
    } else {
        qWarning() << "Internal error";
    }
}

void SerialConfiguration::updateSettings()
{
    if(_link) {
        auto* serialLink = qobject_cast<SerialLink*>(_link);
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
        _portDisplayName = cleanPortDisplayname(pname);
    }
}

QString SerialConfiguration::cleanPortDisplayname(const QString name)
{
    QString pname = name.trimmed();
#ifdef Q_OS_WIN
    pname.replace("\\\\.\\", "");
#else
    pname.replace("/dev/cu.", "");
    pname.replace("/dev/", "");
#endif
    return pname;
}

void SerialConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("baud",           _baud);
    settings.setValue("dataBits",       _dataBits);
    settings.setValue("flowControl",    _flowControl);
    settings.setValue("stopBits",       _stopBits);
    settings.setValue("parity",         _parity);
    settings.setValue("portName",       _portName);
    settings.setValue("portDisplayName",_portDisplayName);
    settings.endGroup();
}

void SerialConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    if(settings.contains("baud"))           _baud           = settings.value("baud").toInt();
    if(settings.contains("dataBits"))       _dataBits       = settings.value("dataBits").toInt();
    if(settings.contains("flowControl"))    _flowControl    = settings.value("flowControl").toInt();
    if(settings.contains("stopBits"))       _stopBits       = settings.value("stopBits").toInt();
    if(settings.contains("parity"))         _parity         = settings.value("parity").toInt();
    if(settings.contains("portName"))       _portName       = settings.value("portName").toString();
    if(settings.contains("portDisplayName"))_portDisplayName= settings.value("portDisplayName").toString();
    settings.endGroup();
}

QStringList SerialConfiguration::supportedBaudRates()
{
    if(!kSupportedBaudRates.size())
        _initBaudRates();
    return kSupportedBaudRates;
}

void SerialConfiguration::_initBaudRates()
{
    kSupportedBaudRates.clear();
    kSupportedBaudRates = QStringList({
#if USE_ANCIENT_RATES
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
        "50",
        "75",
#endif
        "110",
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
        "150",
        "200" ,
        "134"  ,
#endif
        "300",
        "600",
        "1200",
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
        "1800",
#endif
#endif
        "2400",
        "4800",
        "9600",
#if defined(Q_OS_WIN)
        "14400",
#endif
        "19200",
        "38400",
#if defined(Q_OS_WIN)
        "56000",
#endif
        "57600",
        "115200",
#if defined(Q_OS_WIN)
        "128000",
#endif
        "230400",
#if defined(Q_OS_WIN)
        "256000",
#endif
        "460800",
        "500000",
#if defined(Q_OS_LINUX)
        "576000",
#endif
        "921600",
    });
}

void SerialConfiguration::setUsbDirect(bool usbDirect)
{
    if (_usbDirect != usbDirect) {
        _usbDirect = usbDirect;
        emit usbDirectChanged(_usbDirect);
    }
}
