#include "SerialLink.h"

#include <QtCore/QSettings>
#include <QtCore/QThread>

#include "QGCLoggingCategory.h"
#include "QGCSerialPort.h"
#include "QGCSerialPortInfo.h"
#include "SerialPlatform.h"

QGC_LOGGING_CATEGORY(SerialLinkLog, "Comms.SerialLink")

namespace {
constexpr int DISCONNECT_TIMEOUT_MS = 3000;
}  // namespace

SerialConfiguration::SerialConfiguration(const QString& name, QObject* parent) : LinkConfiguration(name, parent)
{
    qCDebug(SerialLinkLog) << this;

    (void) connect(SerialPlatform::SerialDevicesNotifier::instance(),
                   &SerialPlatform::SerialDevicesNotifier::devicesChanged,
                   this, [this]() {
        _portDisplayNameValid = false;
        emit portDisplayNameChanged();
    }, Qt::QueuedConnection);
}

SerialConfiguration::SerialConfiguration(const SerialConfiguration* source, QObject* parent)
    : LinkConfiguration(source, parent)
{
    qCDebug(SerialLinkLog) << this;

    (void) connect(SerialPlatform::SerialDevicesNotifier::instance(),
                   &SerialPlatform::SerialDevicesNotifier::devicesChanged,
                   this, [this]() {
        _portDisplayNameValid = false;
        emit portDisplayNameChanged();
    }, Qt::QueuedConnection);

    SerialConfiguration::copyFrom(source);
}

SerialConfiguration::~SerialConfiguration()
{
    qCDebug(SerialLinkLog) << this;
}

void SerialConfiguration::setPortName(const QString& name)
{
    const QString portName = name.trimmed();
    if (portName.isEmpty()) {
        return;
    }

    if (portName != _portName) {
        _portName = portName;
        _portDisplayNameValid = false;
        emit portNameChanged();
        emit portDisplayNameChanged();  // portDisplayName() is derived from _portName
    }
}

void SerialConfiguration::copyFrom(const LinkConfiguration* source)
{
    LinkConfiguration::copyFrom(source);

    const SerialConfiguration* serialSource = qobject_cast<const SerialConfiguration*>(source);
    if (!serialSource) {
        qCWarning(SerialLinkLog) << "copyFrom called with non-SerialConfiguration source";
        return;
    }

    setBaud(serialSource->baud());
    setDataBits(serialSource->dataBits());
    setFlowControl(serialSource->flowControl());
    setStopBits(serialSource->stopBits());
    setParity(serialSource->parity());
    setPortName(serialSource->portName());
    setUsbDirect(serialSource->usbDirect());
    setdtrForceLow(serialSource->dtrForceLow());
}

void SerialConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);

    setBaud(settings.value("baud", _baud).toInt());
    setDataBits(settings.value("dataBits", dataBits()).toInt());
    setFlowControl(settings.value("flowControl", flowControl()).toInt());
    setStopBits(settings.value("stopBits", stopBits()).toInt());
    if (settings.contains("parityV2")) {
        setParity(settings.value("parityV2").toInt());
    } else {
        // Migrate legacy QSerialPort-numbered "parity" (No=0,Even=2,Odd=3,Space=4,Mark=5) to QGCParity.
        static constexpr int kLegacyParityToQGC[] = {0, 0, 2, 1, 4, 3};
        setParity(kLegacyParityToQGC[qBound(0, settings.value("parity", 0).toInt(), 5)]);
    }
    setPortName(settings.value("portName", _portName).toString());
    setdtrForceLow(settings.value("dtrForceLow", _dtrForceLow).toBool());
    setUsbDirect(settings.value("usbDirect", _usbDirect).toBool());

    settings.endGroup();
}

void SerialConfiguration::saveSettings(QSettings& settings, const QString& root) const
{
    settings.beginGroup(root);

    settings.setValue("baud", _baud);
    settings.setValue("dataBits", dataBits());
    settings.setValue("flowControl", flowControl());
    settings.setValue("stopBits", stopBits());
    settings.setValue("parityV2", parity());
    settings.setValue("portName", _portName);
    settings.setValue("dtrForceLow", _dtrForceLow);
    settings.setValue("usbDirect", _usbDirect);

    settings.endGroup();
}

SerialPortConfig SerialConfiguration::portConfig() const
{
    SerialPortConfig cfg;
    cfg.baud = _baud;
    cfg.dataBits = _dataBits;
    cfg.stopBits = _stopBits;
    cfg.parity = _parity;
    cfg.flowControl = _flowControl;
    return cfg;
}

QString SerialConfiguration::portDisplayName() const
{
    if (!_portDisplayNameValid) {
        _portDisplayName = QGCSerialPortInfo::displayNameForLocation(_portName);
        _portDisplayNameValid = true;
    }
    return _portDisplayName;
}

SerialWorker::SerialWorker(const SharedLinkConfigurationPtr& config, QObject* parent)
    : QObject(parent), _configHolder(config), _serialConfig(qobject_cast<const SerialConfiguration*>(config.get()))
{
    qCDebug(SerialLinkLog) << this;
}

SerialWorker::~SerialWorker()
{
    disconnectFromPort();

    qCDebug(SerialLinkLog) << this;
}

bool SerialWorker::isConnected() const
{
    // _connected is atomic; _port is set once in setupPort() and never changes after.
    return _connected.load(std::memory_order_acquire);
}

void SerialWorker::setupPort()
{
    if (!_port) {
        _port = SerialPlatform::makeSerialPort(_serialConfig->portName(), this);

        (void) connect(_port, &QIODevice::aboutToClose, this, &SerialWorker::_onPortDisconnected);
        (void) connect(_port, &QIODevice::readyRead, this, &SerialWorker::_onPortReadyRead);
        (void) connect(_port, &QIODevice::bytesWritten, this, &SerialWorker::_onPortBytesWritten);
        (void) connect(_port, &QGCSerialPort::errorOccurred, this, &SerialWorker::_onPortErrorOccurred);
    }
}

void SerialWorker::connectToPort()
{
    if (!_port) {
        emit errorOccurred(tr("Serial port not created"));
        _onPortDisconnected();
        return;
    }

    if (isConnected()) {
        qCWarning(SerialLinkLog) << "Already connected to" << _port->portName();
        return;
    }

    _port->setPortName(_serialConfig->portName());

    const QGCSerialPortInfo portInfo(_serialConfig->portName());
    if (portInfo.isBootloader()) {
        qCWarning(SerialLinkLog) << "Not connecting to bootloader" << _port->portName();
        emit errorOccurred(tr("Not connecting to a bootloader"));
        _onPortDisconnected();
        return;
    }

    _errorEmitted = false;

    qCDebug(SerialLinkLog) << "Attempting to open port" << _port->portName();
    // 2 MB soft cap: write() returns 0 when full; _flushPendingWrites holds the remainder and resumes on bytesWritten.
    _port->setWriteBufferSize(kSerialWriteBufferCapBytes);
    if (_port->writeBufferSize() != kSerialWriteBufferCapBytes) {
        qCWarning(SerialLinkLog) << "Write buffer cap not honored for" << _port->portName() << "- requested"
                                 << kSerialWriteBufferCapBytes << "got" << _port->writeBufferSize();
    }
    if (!_port->open(QIODevice::ReadWrite)) {
        qCWarning(SerialLinkLog) << "Opening port" << _port->portName() << "failed:" << _port->errorString();

        // If auto-connect is enabled, we don't want to emit an error for PermissionError from devices already in use
        if (!_errorEmitted &&
            (!_serialConfig->isAutoConnect() || _port->error() != QGCSerialPortError::PermissionDenied)) {
            emit errorOccurred(tr("Could not open port: %1").arg(_port->errorString()));
            _errorEmitted = true;
        }

        _onPortDisconnected();

        return;
    }

    _onPortConnected();
}

void SerialWorker::disconnectFromPort()
{
    if (!_port) {
        return;
    }

    if (!isConnected()) {
        qCDebug(SerialLinkLog) << "Already disconnected from port:" << _port->portName();
        return;
    }

    qCDebug(SerialLinkLog) << "Attempting to close port:" << _port->portName();

    _port->close();
}

void SerialWorker::writeData(const QByteArray& data)
{
    if (data.isEmpty()) {
        _emitErrorOnce(tr("Data to Send is Empty"));
        return;
    }

    if (!isConnected()) {
        _emitErrorOnce(tr("Port is not Connected"));
        return;
    }

    if (!_port->isWritable()) {
        _emitErrorOnce(tr("Port is not Writable"));
        return;
    }

    // Backlog is bounded by the same cap as the port write buffer; exceeding it means the device has wedged.
    if ((_pendingWrite.size() + data.size()) > kSerialWriteBufferCapBytes) {
        _emitErrorOnce(tr("Could Not Send Data - Write Buffer Overflow"));
        return;
    }

    _pendingWrite.append(data);
    _flushPendingWrites();
}

void SerialWorker::_flushPendingWrites()
{
    while (!_pendingWrite.isEmpty()) {
        const qint64 bytesWritten = _port->write(_pendingWrite.constData(), _pendingWrite.size());
        if (bytesWritten == -1) {
            _emitErrorOnce(tr("Could Not Send Data - Write Failed: %1").arg(_port->errorString()));
            _pendingWrite.clear();
            return;
        }
        if (bytesWritten == 0) {
            // Port write buffer full; QSerialPort emits bytesWritten as it drains → _onPortBytesWritten resumes.
            return;
        }
        const QByteArray sent = _pendingWrite.left(bytesWritten);
        _pendingWrite.remove(0, bytesWritten);
        emit dataSent(sent);
    }
}

void SerialWorker::_onPortBytesWritten()
{
    _flushPendingWrites();
}

void SerialWorker::_emitErrorOnce(const QString& errorString)
{
    // One emission per connect/disconnect cycle prevents burst UI-modal floods.
    if (_errorEmitted) {
        return;
    }
    _errorEmitted = true;
    emit errorOccurred(errorString);
}

void SerialWorker::_onPortConnected()
{
    qCDebug(SerialLinkLog) << "Port connected:" << _port->portName();

    // Wire params before DTR: dtrForceLow callers must not see a partially-configured device.
    if (!_port->reconfigure(_serialConfig->portConfig())) {
        _emitErrorOnce(tr("Failed to configure port: %1").arg(_port->errorString()));
        _port->close();  // _connected never published here, so _onPortDisconnected's exchange emits nothing (balanced)
        return;
    }
    // DTR failure is non-fatal; QSerialPort treats unsupported DTR benignly on host too.
    if (!_port->setDataTerminalReady(!_serialConfig->dtrForceLow())) {
        qCWarning(SerialLinkLog) << "setDataTerminalReady failed on" << _port->portName() << ":"
                                 << _port->errorString();
    }

    _errorEmitted = false;
    // Publish connected state only after full success so isConnected() never leads the connected() signal.
    _connected.store(true, std::memory_order_release);
    emit connected();
}

void SerialWorker::_onPortDisconnected()
{
    qCDebug(SerialLinkLog) << "Port disconnected:" << (_port ? _port->portName() : QString());

    _errorEmitted = false;
    _pendingWrite.clear();  // drop backlog: a closed port can never drain it
    // Only emit if we previously emitted connected(); prevents unbalanced disconnected() on a failed open.
    if (_connected.exchange(false, std::memory_order_acq_rel)) {
        emit disconnected();
    }
}

void SerialWorker::_onPortReadyRead()
{
    const QByteArray data = _port->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void SerialWorker::_onPortErrorOccurred(QGCSerialPortError portError)
{
    switch (portError) {
        case QGCSerialPortError::NoError:
            return;
        case QGCSerialPortError::ResourceUnavailable:
            qCDebug(SerialLinkLog) << "Resource error (likely USB disconnect):" << _port->errorString();
            _port->close();
            return;
        case QGCSerialPortError::PermissionDenied:
            if (_serialConfig->isAutoConnect()) {
                return;
            }
            break;
        default:
            break;
    }

    const QString errorString = _port->errorString();
    qCWarning(SerialLinkLog) << "Port error:" << portError << errorString;

    _emitErrorOnce(errorString);
}

SerialLink::SerialLink(SharedLinkConfigurationPtr& config, QObject* parent)
    : LinkInterface(config, parent),
      _serialConfig(qobject_cast<const SerialConfiguration*>(config.get())),
      _worker(new SerialWorker(config)),
      _workerThread(std::make_unique<WorkerThread>(_worker, QStringLiteral("Serial_%1").arg(_serialConfig->name())))
{
    qCDebug(SerialLinkLog) << this;

    (void) connect(_workerThread->thread(), &QThread::started, _worker, &SerialWorker::setupPort);

    (void) connect(_worker, &SerialWorker::connected, this, &SerialLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::disconnected, this, &SerialLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::dataReceived, this, &SerialLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::dataSent, this, &SerialLink::_onDataSent, Qt::QueuedConnection);
    (void) connect(_worker, &SerialWorker::errorOccurred, this, &SerialLink::_onErrorOccurred, Qt::QueuedConnection);

    _workerThread->start();
}

SerialLink::~SerialLink()
{
    // Close on the worker thread directly if we are it; otherwise queue it (stopAndWait drives the missed-queue case).
    if (isConnected()) {
        if (QThread::currentThread() == _workerThread->thread()) {
            _worker->disconnectFromPort();
        } else {
            (void) QMetaObject::invokeMethod(_worker, "disconnectFromPort", Qt::QueuedConnection);
        }
    }
    // Flush disconnected() for the USB-unplug race: worker set _connected=false but the queued slot hasn't drained.
    if (_emittedConnected.load(std::memory_order_acquire)) {
        _onDisconnected();
    }

    // quit + bounded join; detaches (never terminate()) if the worker is wedged in a JNI/USB call. Deletes the worker.
    (void) _workerThread->stopAndWait(DISCONNECT_TIMEOUT_MS);
    _worker = nullptr;

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
    if (_worker && isConnected()) {
        (void) QMetaObject::invokeMethod(_worker, "disconnectFromPort", Qt::QueuedConnection);
    }
}

void SerialLink::_onConnected()
{
    _emittedConnected.store(true, std::memory_order_release);
    emit connected();
}

void SerialLink::_onDisconnected()
{
    if (_emittedConnected.exchange(false)) {
        emit disconnected();
    }
}

void SerialLink::_onErrorOccurred(const QString& errorString)
{
    qCWarning(SerialLinkLog) << "Communication error:" << errorString;
    emit communicationError(
        tr("Serial Link Error"),
        tr("Link %1: (Port: %2) %3").arg(_serialConfig->name(), _serialConfig->portName(), errorString));
}

void SerialLink::_onDataReceived(const QByteArray& data)
{
    emit bytesReceived(this, data);
}

void SerialLink::_onDataSent(const QByteArray& data)
{
    emit bytesSent(this, data);
}

void SerialLink::_writeBytes(const QByteArray& data)
{
    if (!_worker) {
        return;
    }
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}
