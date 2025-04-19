/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SerialLink.h"
#include "QGCLoggingCategory.h"
#include "QGCSerialPortInfo.h"
#ifdef Q_OS_ANDROID
#include "qserialportinfo.h"
#else
#include <QtSerialPort/QSerialPortInfo>
#endif
#include <QtCore/QSettings>
#include <QtCore/QThread>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(SerialLinkLog, "qgc.comms.seriallink")

namespace {
    constexpr int CONNECT_TIMEOUT_MS = 1000;
    constexpr int DISCONNECT_TIMEOUT_MS = 3000;
    constexpr int READ_TIMEOUT_MS = 100;
}

/*===========================================================================*/

SerialConfiguration::SerialConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

SerialConfiguration::SerialConfiguration(const SerialConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;

    SerialConfiguration::copyFrom(source);
}

SerialConfiguration::~SerialConfiguration()
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

void SerialConfiguration::setPortName(const QString &name)
{
    const QString portName = name.trimmed();
    if (portName.isEmpty()) {
        return;
    }

    if (portName != _portName) {
        _portName = portName;
        emit portNameChanged();
    }

    const QString portDisplayName = cleanPortDisplayName(portName);
    setPortDisplayName(portDisplayName);
}

void SerialConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);
    LinkConfiguration::copyFrom(source);

    const SerialConfiguration* const serialSource = qobject_cast<const SerialConfiguration*>(source);
    Q_ASSERT(serialSource);

    setBaud(serialSource->baud());
    setDataBits(serialSource->dataBits());
    setFlowControl(serialSource->flowControl());
    setStopBits(serialSource->stopBits());
    setParity(serialSource->parity());
    setPortName(serialSource->portName());
    setPortDisplayName(serialSource->portDisplayName());
    setUsbDirect(serialSource->usbDirect());
}

void SerialConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setBaud(settings.value("baud", _baud).toInt());
    setDataBits(static_cast<QSerialPort::DataBits>(settings.value("dataBits", _dataBits).toInt()));
    setFlowControl(static_cast<QSerialPort::FlowControl>(settings.value("flowControl", _flowControl).toInt()));
    setStopBits(static_cast<QSerialPort::StopBits>(settings.value("stopBits", _stopBits).toInt()));
    setParity(static_cast<QSerialPort::Parity>(settings.value("parity", _parity).toInt()));
    setPortName(settings.value("portName", _portName).toString());
    setPortDisplayName(settings.value("portDisplayName", _portDisplayName).toString());

    settings.endGroup();
}

void SerialConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue("baud", _baud);
    settings.setValue("dataBits", _dataBits);
    settings.setValue("flowControl", _flowControl);
    settings.setValue("stopBits", _stopBits);
    settings.setValue("parity", _parity);
    settings.setValue("portName", _portName);
    settings.setValue("portDisplayName", _portDisplayName);

    settings.endGroup();
}

QStringList SerialConfiguration::supportedBaudRates()
{
    QStringList supportBaudRateStrings;

    const QList<qint32> rates = QSerialPortInfo::standardBaudRates();
    for (qint32 rate : rates) {
        supportBaudRateStrings.append(QString::number(rate));
    }

    return supportBaudRateStrings;
}

QString SerialConfiguration::cleanPortDisplayName(const QString &name)
{
    const QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : availablePorts) {
        if (portInfo.systemLocation() == name) {
            return portInfo.portName();
        }
    }

    return QString();
}

/*===========================================================================*/

SerialWorker::SerialWorker(const SerialConfiguration *config, QObject *parent)
    : QObject(parent)
    , _serialConfig(config)
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
}

SerialWorker::~SerialWorker()
{
    disconnectFromPort();

    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

bool SerialWorker::isConnected() const
{
    return (_port && _port->isOpen());
}

void SerialWorker::setupPort()
{
    Q_ASSERT(!_port);
    _port = new QSerialPort(this);

    Q_ASSERT(!_timer);
    _timer = new QTimer(this);

    (void) connect(_port, &QSerialPort::aboutToClose, this, &SerialWorker::_onPortDisconnected);
    (void) connect(_port, &QSerialPort::readyRead, this, &SerialWorker::_onPortReadyRead);
    (void) connect(_port, &QSerialPort::errorOccurred, this, &SerialWorker::_onPortErrorOccurred);

    /* if (SerialLinkLog().isDebugEnabled()) {
        (void) connect(_port, &QSerialPort::bytesWritten, this, &SerialWorker::_onPortBytesWritten);
    } */

    (void) connect(_timer, &QTimer::timeout, this, &SerialWorker::_checkPortAvailability);
    _timer->start(CONNECT_TIMEOUT_MS);
}

void SerialWorker::connectToPort()
{
    if (isConnected()) {
        qCWarning(SerialLinkLog) << "Already connected to" << _port->portName();
        return;
    }

    _port->setPortName(_serialConfig->portName());

    const QGCSerialPortInfo portInfo(*_port);
    if (portInfo.isBootloader()) {
        qCWarning(SerialLinkLog) << "Not connecting to bootloader" << _port->portName();
        emit errorOccurred(tr("Not connecting to a bootloader"));
        _onPortDisconnected();
        return;
    }

    _errorEmitted = false;

    qCDebug(SerialLinkLog) << "Attempting to open port" << _port->portName();
    if (!_port->open(QIODevice::ReadWrite)) {
        qCWarning(SerialLinkLog) << "Opening port" << _port->portName() << "failed:" << _port->errorString();

        if (!_errorEmitted) {
            emit errorOccurred(tr("Could not open port: %1").arg(_port->errorString()));
            _errorEmitted = true;
        }

        // Disconnecting here on autoconnect will cause continuous error popups
        if (!_serialConfig->isAutoConnect()) {
            _onPortDisconnected();
        }

        return;
    }

    _onPortConnected();
}

void SerialWorker::disconnectFromPort()
{
    if (!isConnected()) {
        qCDebug(SerialLinkLog) << "Already disconnected from port:" << _port->portName();
        return;
    }

    qCDebug(SerialLinkLog) << "Attempting to close port:" << _port->portName();
    _port->close();
}

void SerialWorker::writeData(const QByteArray &data)
{
    if (data.isEmpty()) {
        emit errorOccurred(tr("Data to Send is Empty"));
        return;
    }

    if (!isConnected()) {
        emit errorOccurred(tr("Port is not Connected"));
        return;
    }

    if (!_port->isWritable()) {
        emit errorOccurred(tr("Port is not Writable"));
        return;
    }

    qint64 totalBytesWritten = 0;
    while (totalBytesWritten < data.size()) {
        const qint64 bytesWritten = _port->write(data.constData() + totalBytesWritten, data.size() - totalBytesWritten);
        if (bytesWritten == -1) {
            emit errorOccurred(tr("Could Not Send Data - Write Failed: %1").arg(_port->errorString()));
            return;
        } else if (bytesWritten == 0) {
            emit errorOccurred(tr("Could Not Send Data - Write Returned 0 Bytes"));
            return;
        }
        totalBytesWritten += bytesWritten;
    }

    const QByteArray sent = data.first(totalBytesWritten);
    emit dataSent(sent);
}

void SerialWorker::_onPortConnected()
{
    qCDebug(SerialLinkLog) << "Port connected:" << _port->portName();

    _port->setDataTerminalReady(true);
    _port->setBaudRate(_serialConfig->baud());
    _port->setDataBits(static_cast<QSerialPort::DataBits>(_serialConfig->dataBits()));
    _port->setFlowControl(static_cast<QSerialPort::FlowControl>(_serialConfig->flowControl()));
    _port->setStopBits(static_cast<QSerialPort::StopBits>(_serialConfig->stopBits()));
    _port->setParity(static_cast<QSerialPort::Parity>(_serialConfig->parity()));

    _errorEmitted = false;
    emit connected();
}

void SerialWorker::_onPortDisconnected()
{
    qCDebug(SerialLinkLog) << "Port disconnected:" << _port->portName();
    _errorEmitted = false;
    emit disconnected();
}

void SerialWorker::_onPortReadyRead()
{
    const QByteArray data = _port->readAll();
    if (!data.isEmpty()) {
        // qCDebug(SerialLinkLog) << "_onPortReadyRead:" << data.size();
        emit dataReceived(data);
    }
}

void SerialWorker::_onPortBytesWritten(qint64 bytes) const
{
    qCDebug(SerialLinkLog) << _port->portName() << "Wrote" << bytes << "bytes";
}

void SerialWorker::_onPortErrorOccurred(QSerialPort::SerialPortError portError)
{
    const QString errorString = _port->errorString();
    qCWarning(SerialLinkLog) << "Port error:" << portError << errorString;

    switch (portError) {
    case QSerialPort::NoError:
        qCDebug(SerialLinkLog) << "About to open port" << _port->portName();
        return;
    case QSerialPort::PermissionError:
        if (_serialConfig->isAutoConnect()) {
            return;
        }
        break;
    /*case QSerialPort::ResourceError:
        serialPort->close();
        break;*/
    default:
        break;
    }

    if (!_errorEmitted) {
        emit errorOccurred(errorString);
        _errorEmitted = true;
    }
}

void SerialWorker::_checkPortAvailability()
{
    if (!isConnected()) {
        return;
    }

    bool portExists = false;
    const auto availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : availablePorts) {
        if (info.portName() == _serialConfig->portDisplayName()) {
            portExists = true;
            break;
        }
    }

    if (!portExists) {
        _port->close();
    }
}

/*===========================================================================*/

SerialLink::SerialLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _serialConfig(qobject_cast<const SerialConfiguration*>(config.get()))
    , _worker(new SerialWorker(_serialConfig))
    , _workerThread(new QThread(this))
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;

    _workerThread->setObjectName(QStringLiteral("Serial_%1").arg(_serialConfig->name()));

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &SerialWorker::setupPort);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &SerialWorker::connected, this, &SerialLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::disconnected, this, &SerialLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::dataReceived, this, &SerialLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::dataSent, this, &SerialLink::_onDataSent, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::errorOccurred, this, &SerialLink::_onErrorOccurred, Qt::QueuedConnection);

    _workerThread->start();
}

SerialLink::~SerialLink()
{
    SerialLink::disconnect();

    _workerThread->quit();
    if (!_workerThread->wait(DISCONNECT_TIMEOUT_MS)) {
        qCWarning(SerialLinkLog) << "Failed to wait for Serial Thread to close";
        // _workerThread->terminate();
    }

    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

bool SerialLink::isConnected() const
{
    return _worker->isConnected();
}

bool SerialLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectToPort", Qt::QueuedConnection);
}

void SerialLink::disconnect()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectFromPort", Qt::QueuedConnection);
}

void SerialLink::_onConnected()
{
    emit connected();
}

void SerialLink::_onDisconnected()
{
    emit disconnected();
}

void SerialLink::_onErrorOccurred(const QString &errorString)
{
    qCWarning(SerialLinkLog) << "Communication error:" << errorString;
    emit communicationError(tr("Serial Link Error"), tr("Link %1: (Port: %2) %3").arg(_serialConfig->name(), _serialConfig->portName(), errorString));
}

void SerialLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void SerialLink::_onDataSent(const QByteArray &data)
{
    emit bytesSent(this, data);
}

void SerialLink::_writeBytes(const QByteArray &data)
{
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}
