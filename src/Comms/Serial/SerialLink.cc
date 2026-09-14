#include "SerialLink.h"

#include <QtCore/QSettings>
#include <QtCore/QSignalBlocker>
#include <QtCore/QThread>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"
#include "SerialPortManager.h"

QGC_LOGGING_CATEGORY(SerialLinkLog, "Comms.Serial.SerialLink")

/*===========================================================================*/

SerialConfiguration::SerialConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(SerialLinkLog) << this;
}

SerialConfiguration::SerialConfiguration(const SerialConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
{
    qCDebug(SerialLinkLog) << this;

    SerialConfiguration::copyFrom(source);
}

SerialConfiguration::~SerialConfiguration()
{
    qCDebug(SerialLinkLog) << this;
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

    // Only update the display name if the port is currently available. Otherwise keep
    // the existing (e.g. persisted) display name rather than clearing it.
    const QString portDisplayName = cleanPortDisplayName(portName);
    if (!portDisplayName.isEmpty()) {
        setPortDisplayName(portDisplayName);
    }
}

void SerialConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const SerialConfiguration* serialSource = qobject_cast<const SerialConfiguration*>(source);

    setBaud(serialSource->baud());
    setDataBits(serialSource->dataBits());
    setFlowControl(serialSource->flowControl());
    setStopBits(serialSource->stopBits());
    setParity(serialSource->parity());
    setPortName(serialSource->portName());
    setPortDisplayName(serialSource->portDisplayName());
    setUsbDirect(serialSource->usbDirect());
    setdtrForceLow(serialSource->dtrForceLow());
}

void SerialConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setBaud(settings.value("baud", _baud).toInt());
    setDataBits(static_cast<QSerialPort::DataBits>(settings.value("dataBits", _dataBits).toInt()));
    setFlowControl(static_cast<QSerialPort::FlowControl>(settings.value("flowControl", _flowControl).toInt()));
    setStopBits(static_cast<QSerialPort::StopBits>(settings.value("stopBits", _stopBits).toInt()));
    setParity(static_cast<QSerialPort::Parity>(settings.value("parity", _parity).toInt()));
    // Load the saved display name first as a fallback; setPortName() recomputes a
    // fresh display name which takes precedence when the device is present.
    setPortDisplayName(settings.value("portDisplayName", _portDisplayName).toString());
    setPortName(settings.value("portName", _portName).toString());
    setdtrForceLow(settings.value("dtrForceLow", _dtrForceLow).toBool());

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
    settings.setValue("dtrForceLow", _dtrForceLow);

    settings.endGroup();
}

QStringList SerialConfiguration::supportedBaudRates()
{
    return SerialPortManager::supportedBaudRates();
}

SerialConnectionSettings SerialConfiguration::connectionSettings() const
{
    const auto ports = SerialPortManager::instance()->availablePorts();
    const bool bootloader = std::any_of(ports.cbegin(), ports.cend(), [this](const auto& port) {
        return (port.systemLocation == _portName || port.portName == _portName) && port.bootloader;
    });
    return {_portName, _baud, _dataBits, _flowControl, _stopBits, _parity, _dtrForceLow, isAutoConnect(), bootloader};
}

QString SerialConfiguration::cleanPortDisplayName(const QString &name)
{
    return SerialPortManager::instance()->displayName(name);
}

/*===========================================================================*/

SerialWorker::SerialWorker(SerialConnectionSettings settings, SerialPortManager::ReservationPtr reservation,
                           QObject* parent)
    : QObject(parent), _settings(std::move(settings)), _reservation(std::move(reservation))
{
    qCDebug(SerialLinkLog) << this;

    (void) qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
}

SerialWorker::~SerialWorker()
{
    disconnectFromPort();

    qCDebug(SerialLinkLog) << this;
}

bool SerialWorker::isConnected() const
{
    // Called cross-thread from SerialLink; must not touch the thread-affine _port
    return _isConnected;
}

void SerialWorker::setupPort()
{
    if (_port) {
        return;
    }
    _port = new QSerialPort(this);

    (void) connect(_port, &QSerialPort::aboutToClose, this, &SerialWorker::_onPortDisconnected);
    (void) connect(_port, &QSerialPort::readyRead, this, &SerialWorker::_onPortReadyRead);
    (void) connect(_port, &QSerialPort::errorOccurred, this, &SerialWorker::_onPortErrorOccurred);

    /* if (SerialLinkLog().isDebugEnabled()) {
        (void) connect(_port, &QSerialPort::bytesWritten, this, &SerialWorker::_onPortBytesWritten);
    } */
}

void SerialWorker::connectToPort()
{
    if (!_port) {
        setupPort();
    }
    if (isConnected()) {
        qCWarning(SerialLinkLog) << "Already connected to" << _port->portName();
        return;
    }

    _port->setPortName(_settings.portName);

    if (_settings.bootloader) {
        qCWarning(SerialLinkLog) << "Not connecting to bootloader" << _port->portName();
        emit errorOccurred(tr("Not connecting to a bootloader"));
        _onPortDisconnected();
        return;
    }

    _errorEmitted = false;

    qCDebug(SerialLinkLog) << "Attempting to open port" << _port->portName();
    if (!_port->open(QIODevice::ReadWrite)) {
        const bool busyAutoConnect = _settings.autoConnect && (_port->error() == QSerialPort::PermissionError ||
                                                               _port->error() == QSerialPort::DeviceNotFoundError);
        if (busyAutoConnect) {
            qCDebug(SerialLinkLog) << "Auto-connect port unavailable:" << _port->portName() << _port->errorString();
        } else {
            qCWarning(SerialLinkLog) << "Opening port" << _port->portName() << "failed:" << _port->errorString();
        }

        // Occupied or disappearing devices are normal races during automatic discovery.
        if (!_errorEmitted && !busyAutoConnect) {
            emit errorOccurred(tr("Could not open port: %1").arg(_port->errorString()));
            _errorEmitted = true;
        }

        _onPortDisconnected();

        return;
    }

    if (!_configurePort()) {
        const QString error = _port->errorString();
        emit errorOccurred(tr("Could not configure port: %1").arg(error));
        _port->close();
        return;
    }
    _onPortConnected();
}

void SerialWorker::disconnectFromPort()
{
    if (!_port || !_port->isOpen()) {
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

    _isConnected = true;
    _errorEmitted = false;
    emit connected();
}

void SerialWorker::_onPortDisconnected()
{
    qCDebug(SerialLinkLog) << "Port disconnected:" << _port->portName();

    _isConnected = false;
    _errorEmitted = false;
    emit disconnected();
}

void SerialWorker::_onPortReadyRead()
{
    const QByteArray data = _port->readAll();
    if (!data.isEmpty()) {
        // qCDebug(SerialLinkLog) << data.size();
        emit dataReceived(data);
    }
}

void SerialWorker::_onPortBytesWritten(qint64 bytes) const
{
    qCDebug(SerialLinkLog) << _port->portName() << "Wrote" << bytes << "bytes";
}

void SerialWorker::_onPortErrorOccurred(QSerialPort::SerialPortError portError)
{
    switch (portError) {
    case QSerialPort::NoError:
        qCDebug(SerialLinkLog) << "About to open port" << _port->portName();
        return;
    case QSerialPort::ResourceError:
        // We get this when a usb cable is unplugged - close port to allow reconnection
        qCDebug(SerialLinkLog) << "Resource error (likely USB disconnect):" << _port->errorString();
        _port->close();
        return;
    case QSerialPort::PermissionError:
    case QSerialPort::DeviceNotFoundError:
        if (_settings.autoConnect) {
            return;
        }
        break;
    default:
        break;
    }

    const QString errorString = _port->errorString();
    qCWarning(SerialLinkLog) << "Port error:" << portError << errorString;

    if (!_errorEmitted) {
        emit errorOccurred(errorString);
        _errorEmitted = true;
    }
}

void SerialWorker::checkPortAvailability(const QStringList& availablePorts)
{
    if (isConnected() && !availablePorts.contains(_settings.portName)) {
        _port->close();
    }
}

bool SerialWorker::_configurePort()
{
    // Report one setup failure; QSerialPort also emits errorOccurred synchronously.
    const QSignalBlocker blocker(_port);
    if (!_port->setBaudRate(_settings.baud) || !_port->setDataBits(_settings.dataBits) ||
        !_port->setFlowControl(_settings.flowControl) || !_port->setStopBits(_settings.stopBits) ||
        !_port->setParity(_settings.parity)) {
        return false;
    }
    if (!_port->setDataTerminalReady(!_settings.dtrForceLow)) {
        // PTYs and some USB adapters have no modem-control lines. Forcing DTR low is explicit and required.
        if (_settings.dtrForceLow || _port->error() != QSerialPort::UnsupportedOperationError) {
            return false;
        }
        qCDebug(SerialLinkLog) << "DTR is unavailable on" << _settings.portName;
        _port->clearError();
    }
    return true;
}

/*===========================================================================*/

SerialLink::SerialLink(SharedLinkConfigurationPtr& config, SerialPortManager::ReservationPtr reservation,
                       QObject* parent)
    : LinkInterface(config, parent),
      _serialConfig(qobject_cast<const SerialConfiguration*>(config.get())),
      _reservation(std::move(reservation)),
      _worker(new SerialWorker(_serialConfig->connectionSettings(), _reservation)),
      _workerThread(new QThread(this))
{
    qCDebug(SerialLinkLog) << this;

    _workerThread->setObjectName(QStringLiteral("Serial_%1").arg(_serialConfig->name()));

    (void) _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &SerialWorker::setupPort);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &SerialWorker::connected, this, &SerialLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::disconnected, this, &SerialLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::dataReceived, this, &SerialLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::dataSent, this, &SerialLink::_onDataSent, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::errorOccurred, this, &SerialLink::_onErrorOccurred, Qt::QueuedConnection);

    (void) connect(SerialPortManager::instance(), &SerialPortManager::portsEnumerated, _worker,
                   &SerialWorker::checkPortAvailability, Qt::QueuedConnection);

    _workerThread->start();
}

SerialLink::~SerialLink()
{
    if (isConnected()) {
        // Queued, not blocking: a wedged worker (e.g. QSerialPort::close() hung in a driver)
        // would block here forever, before _shutdownWorkerThread can bound the wait.
        // If quit() beats the queued call, ~SerialWorker still disconnects on thread finish.
        (void) QMetaObject::invokeMethod(_worker, "disconnectFromPort", Qt::QueuedConnection);
        _onDisconnected();
    }

    _shutdownWorkerThread(_workerThread, SerialLinkLog());

    qCDebug(SerialLinkLog) << this;
}

bool SerialLink::isConnected() const
{
    return _worker && _worker->isConnected();
}

bool SerialLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectToPort", Qt::QueuedConnection);
}

void SerialLink::disconnect()
{
    if (isConnected()) {
        (void) QMetaObject::invokeMethod(_worker, "disconnectFromPort", Qt::QueuedConnection);
    }
}

void SerialLink::_onConnected()
{
    _disconnectedEmitted = false;
    emit connected();
}

void SerialLink::_onDisconnected()
{
    if (!_disconnectedEmitted.exchange(true)) {
        emit disconnected();
    }
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
