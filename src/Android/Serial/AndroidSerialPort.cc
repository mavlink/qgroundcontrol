#include "AndroidSerialPort.h"

#include <cstring>
#include <utility>

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QScopedValueRollback>

#include "AndroidSerialBackend.h"
#include "AndroidSerialPortInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "Android.AndroidSerialPort")
// What crosses each port, for debugging a link in the field, where the app's own log is the one at hand. It is
// named beside the port's category, not below it: --logging switches a category on with all those below it.
QGC_LOGGING_CATEGORY(AndroidSerialPortTraceLog, "Android.AndroidSerialPort:trace")

namespace {

// Best-effort send of buffered writes when closing; a stalled link must not delay close.
constexpr int CLOSE_WRITE_TIMEOUT_MS = 100;
// A transfer may be thousands of bytes, and its start tells which message it is.
constexpr qint64 MAX_TRACED_BYTES = 32;

/// A transfer as the trace shows it: its direction, its length and its first bytes in hex.
QString tracedTransfer(const char* direction, const char* bytes, qint64 length)
{
    const QByteArray shown = QByteArray::fromRawData(bytes, static_cast<qsizetype>(qMin(length, MAX_TRACED_BYTES)));
    return QStringLiteral("%1 %2: %3%4")
        .arg(QLatin1StringView(direction), QString::number(length), QString::fromLatin1(shown.toHex(' ').toUpper()),
             (length > MAX_TRACED_BYTES) ? QStringLiteral(" ...") : QString());
}

/// What an error that comes without a text says of itself. Null for an error with nothing to say, which
/// QIODevice::errorString() calls an unknown error.
QString errorText(QSerialPort::SerialPortError error)
{
    switch (error) {
        case QSerialPort::NoError:
            return QSerialPort::tr("No error");
        case QSerialPort::OpenError:
            return QSerialPort::tr("Device is already open");
        case QSerialPort::NotOpenError:
            return QSerialPort::tr("Device is not open");
        case QSerialPort::TimeoutError:
            return QSerialPort::tr("Operation timed out");
        case QSerialPort::ReadError:
            return QSerialPort::tr("Error reading from device");
        case QSerialPort::WriteError:
            return QSerialPort::tr("Error writing to device");
        case QSerialPort::ResourceError:
            return QSerialPort::tr("Device disappeared from the system");
        default:
            return QString();
    }
}

}  // namespace

QSerialPort::QSerialPort(QObject* parent)
    : QIODevice(parent)
{}

QSerialPort::QSerialPort(const QString& name, QObject* parent)
    : QIODevice(parent)
{
    setPortName(name);
}

QSerialPort::QSerialPort(const QSerialPortInfo& info, QObject* parent)
    : QIODevice(parent)
{
    setPort(info);
}

QSerialPort::~QSerialPort()
{
    if (isOpen()) {
        close();
    }
}

void QSerialPort::setPortName(const QString& name)
{
    _systemLocation = QSerialPortInfo::portNameToSystemLocation(name);
}

QString QSerialPort::portName() const
{
    return QSerialPortInfo::portNameFromSystemLocation(_systemLocation);
}

void QSerialPort::setPort(const QSerialPortInfo& info)
{
    _systemLocation = info.systemLocation();
}

bool QSerialPort::open(OpenMode mode)
{
    if (isOpen()) {
        _setError(OpenError);
        return false;
    }

    if ((mode & (Append | Truncate | Text | Unbuffered)) || (mode == NotOpen)) {
        _setError(UnsupportedOperationError, tr("Unsupported open mode"));
        return false;
    }

    clearError();
    if (!_openDevice(mode)) {
        _closeDevice();
        _discardBuffers();
        return false;
    }

    // The port holds its input itself, so QIODevice is not to keep a copy of what the owner reads.
    return QIODevice::open(mode | Unbuffered);
}

bool QSerialPort::_openDevice(OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << _systemLocation;

    AndroidSerialBackend* const backend = _backendFor(_systemLocation);
    if (!backend->open(mode)) {
        return false;
    }
    _backend = backend;

    if (!backend->setParameters(_baudRate, _dataBits, _stopBits, _parity, tr("Failed to set parameters"))) {
        return false;
    }

    if (!backend->setFlowControl(_flowControl)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port flow control for" << _systemLocation;
        return false;
    }

    if ((mode & ReadOnly) && !_readOn()) {
        qCWarning(AndroidSerialPortLog) << "Failed to start async read for" << _systemLocation;
        return false;
    }

    if (!_clearDevice(backend, AllDirections)) {
        qCWarning(AndroidSerialPortLog) << "Failed to clear buffers for" << _systemLocation;
        return false;
    }

    return true;
}

void QSerialPort::close()
{
    if (!isOpen()) {
        _setError(NotOpenError);
        return;
    }

    // Buffered writes were accepted by write(); try to send them unless the port can no longer write.
    if ((_error != WriteError) && (_error != ResourceError)) {
        _flushBeforeClose();
    }
    _closeDevice();
    _breakEnabled = false;
    // Its aboutToClose() is the owner's last chance to read the input that it was told of.
    QIODevice::close();
    _discardBuffers();
}

void QSerialPort::_closeDevice()
{
    if (AndroidSerialBackend* const backend = std::exchange(_backend, nullptr)) {
        qCDebug(AndroidSerialPortLog) << "Closing" << _systemLocation;
        backend->close();
    }
    _dataTerminalReady.reset();
    _requestToSend.reset();
}

void QSerialPort::_discardBuffers()
{
    _output.clear();
    _bytesInFlight = 0;
    _discardedInFlight = 0;

    // The backend is closed, so no more input or report can arrive. Dropping what was not delivered keeps a
    // queued admission, and a later re-open, from delivering stale bytes.
    QMutexLocker locker(&_arrivalMutex);
    _input.clear();
    _admitted = 0;
    _admissionPending.store(false);
    _readingHeldBack = false;
    _completedWriteBytes = 0;
    _writeProgress = false;
    _waitInterrupted = false;
}

AndroidSerialBackend* QSerialPort::_backendFor(const QString& location)
{
    const bool usb = QSerialPortInfo::isUsbLocation(location);
    std::unique_ptr<AndroidSerialBackend>& backend = usb ? _usbBackend : _posixBackend;
    if (!backend) {
        const AndroidSerialBackendHost host(*this);
        backend = usb ? createUsbSerialBackend(host) : createPosixSerialBackend(host);
    }

    return backend.get();
}

AndroidSerialBackend* QSerialPort::_openBackend()
{
    if (!_backend) {
        _setError(NotOpenError);
    }

    return _backend;
}

void QSerialPort::_setError(SerialPortError error, const QString& text)
{
    setErrorString(text.isNull() ? errorText(error) : text);
    _error = error;
    emit errorOccurred(error);
}

void QSerialPort::clearError()
{
    _setError(NoError);
}

bool QSerialPort::setBaudRate(qint32 baudRate, Directions directions)
{
    if (directions != AllDirections) {
        qCWarning(AndroidSerialPortLog) << "Custom baud rate direction is unsupported:" << directions;
        _setError(UnsupportedOperationError, tr("Custom baud rate direction is unsupported"));
        return false;
    }

    if (_backend) {
        // A closed port keeps any rate it is given; its backend refuses a bad one when the port opens.
        if (baudRate <= 0) {
            qCWarning(AndroidSerialPortLog) << "Invalid baud rate value:" << baudRate;
            _setError(UnsupportedOperationError, tr("Invalid baud rate value"));
            return false;
        }
        if (!_backend->setParameters(baudRate, _dataBits, _stopBits, _parity, tr("Failed to set baud rate"))) {
            return false;
        }
    }

    if (_baudRate != baudRate) {
        _baudRate = baudRate;
        emit baudRateChanged(baudRate, AllDirections);
    }

    return true;
}

qint32 QSerialPort::baudRate(Directions) const
{
    return _baudRate;
}

bool QSerialPort::setDataBits(DataBits dataBits)
{
    if (_backend && !_backend->setParameters(_baudRate, dataBits, _stopBits, _parity, tr("Failed to set data bits"))) {
        return false;
    }

    if (_dataBits != dataBits) {
        _dataBits = dataBits;
        emit dataBitsChanged(dataBits);
    }

    return true;
}

bool QSerialPort::setParity(Parity parity)
{
    if (_backend && !_backend->setParameters(_baudRate, _dataBits, _stopBits, parity, tr("Failed to set parity"))) {
        return false;
    }

    if (_parity != parity) {
        _parity = parity;
        emit parityChanged(parity);
    }

    return true;
}

bool QSerialPort::setStopBits(StopBits stopBits)
{
    if (_backend && !_backend->setParameters(_baudRate, _dataBits, stopBits, _parity, tr("Failed to set stop bits"))) {
        return false;
    }

    if (_stopBits != stopBits) {
        _stopBits = stopBits;
        emit stopBitsChanged(stopBits);
    }

    return true;
}

bool QSerialPort::setFlowControl(FlowControl flowControl)
{
    if (_backend && !_backend->setFlowControl(flowControl)) {
        return false;
    }

    if (_flowControl != flowControl) {
        _flowControl = flowControl;
        emit flowControlChanged(flowControl);
    }

    return true;
}

bool QSerialPort::setDataTerminalReady(bool set)
{
    AndroidSerialBackend* const backend = _openBackend();
    if (!backend || !backend->setDataTerminalReady(set)) {
        return false;
    }

    if (_dataTerminalReady != set) {
        _dataTerminalReady = set;
        emit dataTerminalReadyChanged(set);
    }

    return true;
}

bool QSerialPort::isDataTerminalReady()
{
    return pinoutSignals() & DataTerminalReadySignal;
}

bool QSerialPort::setRequestToSend(bool set)
{
    AndroidSerialBackend* const backend = _openBackend();
    if (!backend) {
        return false;
    }

    if (_flowControl == HardwareControl) {
        _setError(UnsupportedOperationError);
        return false;
    }

    if (!backend->setRequestToSend(set)) {
        return false;
    }

    if (_requestToSend != set) {
        _requestToSend = set;
        emit requestToSendChanged(set);
    }

    return true;
}

bool QSerialPort::isRequestToSend()
{
    return pinoutSignals() & RequestToSendSignal;
}

QSerialPort::PinoutSignals QSerialPort::pinoutSignals()
{
    AndroidSerialBackend* const backend = _openBackend();
    return backend ? backend->pinoutSignals() : PinoutSignals(NoSignal);
}

bool QSerialPort::setBreakEnabled(bool set)
{
    AndroidSerialBackend* const backend = _openBackend();
    if (!backend || !backend->setBreakEnabled(set)) {
        return false;
    }

    if (_breakEnabled != set) {
        _breakEnabled = set;
        emit breakEnabledChanged(set);
    }

    return true;
}

int QSerialPort::handle() const
{
    return _backend ? _backend->descriptor() : -1;
}

void QSerialPort::setReadBufferSize(qint64 size)
{
    {
        // The thread that delivers input reads the limit.
        QMutexLocker locker(&_arrivalMutex);
        _readBufferMaxSize = size;
    }

    if (isReadable()) {
        (void) _readOn();
    }
}

bool QSerialPort::isSequential() const
{
    return true;
}

qint64 QSerialPort::bytesAvailable() const
{
    // QIODevice holds input of its own only after a peek or within a transaction.
    return QIODevice::bytesAvailable() + _admitted;
}

qint64 QSerialPort::bytesToWrite() const
{
    // Output that the backend has taken is not written until it says so.
    return _output.size() + _bytesInFlight;
}

bool QSerialPort::canReadLine() const
{
    if (QIODevice::canReadLine()) {
        return true;
    }

    QMutexLocker locker(&_arrivalMutex);
    return QByteArrayView(_input).first(_admitted).contains('\n');
}

qint64 QSerialPort::readData(char* data, qint64 maxSize)
{
    qsizetype size = 0;
    {
        QMutexLocker locker(&_arrivalMutex);
        size = static_cast<qsizetype>(qMin<qint64>(maxSize, _admitted));
        if (size > 0) {
            std::memcpy(data, _input.constData(), static_cast<size_t>(size));
            _input.remove(0, size);
            _admitted -= size;
        }
    }

    // Every read comes here, also one that finds nothing. It may have made room for input that waited, and it
    // makes sure that the backend reads.
    (void) _readOn();

    return size;
}

bool QSerialPort::_readOn()
{
    if (!_backend) {
        return false;
    }

    bool inputWaits = false;
    bool hasRoom = false;
    bool resumed = false;
    {
        QMutexLocker locker(&_arrivalMutex);
        inputWaits = _hiddenSizeLocked() > 0;
        hasRoom = _hasRoomForInputLocked();
        if (hasRoom) {
            resumed = std::exchange(_readingHeldBack, false);
        }
    }

    if (resumed) {
        qCDebug(AndroidSerialPortTraceLog).noquote() << _systemLocation << "RX resumed";
    }
    if (inputWaits) {
        _scheduleAdmission();
    }

    // The backend reads only while the port has room. The admission that makes room asks it to read on.
    return !hasRoom || _backend->startReading();
}

bool QSerialPort::_readOnIfRoom()
{
    {
        QMutexLocker locker(&_arrivalMutex);
        if (!_readingHeldBack || !_hasRoomForInputLocked()) {
            return true;
        }
    }

    return _readOn();
}

qsizetype QSerialPort::_hiddenSizeLocked() const
{
    return _input.size() - _admitted;
}

bool QSerialPort::_hasRoomForInputLocked() const
{
    return (_readBufferMaxSize <= 0) || (_hiddenSizeLocked() < _readBufferMaxSize);
}

bool QSerialPort::_canAdmitInputLocked() const
{
    return (_hiddenSizeLocked() > 0) && ((_readBufferMaxSize <= 0) || (bytesAvailable() < _readBufferMaxSize));
}

bool QSerialPort::_admitInputLocked()
{
    if (!_canAdmitInputLocked()) {
        return false;
    }

    const qsizetype hidden = _hiddenSizeLocked();
    _admitted += (_readBufferMaxSize > 0)
                     ? static_cast<qsizetype>(qMin<qint64>(hidden, _readBufferMaxSize - bytesAvailable()))
                     : hidden;
    return true;
}

bool QSerialPort::_admitArrivedInput()
{
    QMutexLocker locker(&_arrivalMutex);
    return _admitInputLocked();
}

void QSerialPort::_scheduleAdmission()
{
    if (!_admissionPending.exchange(true)) {
        QMetaObject::invokeMethod(this, &QSerialPort::_admitQueuedInput, Qt::QueuedConnection);
    }
}

void QSerialPort::_admitQueuedInput()
{
    bool admitted = false;
    {
        QMutexLocker locker(&_arrivalMutex);
        admitted = isOpen() && _admitInputLocked();
        // Reset only now, so that input arriving meanwhile does not queue a second admission. Input that the
        // read buffer size keeps hidden waits for a read to make room, which schedules this again.
        _admissionPending.store(false);
    }

    if (admitted) {
        _announceAdmittedInput();
    }
}

void QSerialPort::_announceAdmittedInput()
{
    // Before the announcement, whose receiver may wait for more input.
    (void) _readOnIfRoom();
    // The port's owner may have closed it over an error of that.
    if (!isOpen() || _emittingReadyRead) {
        return;
    }

    // Like every QIODevice, not from inside one of its own slots, which may be waiting for more input.
    const QScopedValueRollback<bool> emitting(_emittingReadyRead, true);
    emit readyRead();
}

bool QSerialPort::_newDataArrived(const char* bytes, int length)
{
    qCDebug(AndroidSerialPortTraceLog).noquote() << _systemLocation << tracedTransfer("RX", bytes, length);

    bool hasRoom = false;
    bool paused = false;
    {
        QMutexLocker locker(&_arrivalMutex);
        // Never dropped, like the input of desktop QSerialPort, which leaves what its buffer has no room for with
        // the kernel. Here the backend leaves it with the device once it hears that there is no room.
        _input.append(bytes, length);
        hasRoom = _hasRoomForInputLocked();
        paused = !hasRoom && !_readingHeldBack;
        _readingHeldBack = !hasRoom;
        _arrivalCondition.wakeAll();
    }
    if (paused) {
        qCDebug(AndroidSerialPortTraceLog).noquote() << _systemLocation << "RX paused";
    }

    _scheduleAdmission();
    return hasRoom;
}

bool QSerialPort::_hasRoomForInput()
{
    QMutexLocker locker(&_arrivalMutex);
    return _hasRoomForInputLocked();
}

bool QSerialPort::_inputArrived()
{
    QMutexLocker locker(&_arrivalMutex);
    return _canAdmitInputLocked();
}

void QSerialPort::_waitForArrivedInput(const QDeadlineTimer& deadline)
{
    QMutexLocker locker(&_arrivalMutex);
    if (!_canAdmitInputLocked() && !_writeProgress && !_waitInterrupted) {
        (void) _arrivalCondition.wait(&_arrivalMutex, deadline);
    }
    _waitInterrupted = false;
}

void QSerialPort::_writeCompleted(qint64 bytes)
{
    {
        QMutexLocker locker(&_arrivalMutex);
        _completedWriteBytes += bytes;
        _writeProgress = true;
    }
    _arrivalCondition.wakeAll();

    _scheduleWrite();
}

void QSerialPort::_waitForWriteProgress(const QDeadlineTimer& deadline)
{
    QMutexLocker locker(&_arrivalMutex);
    if (!_writeProgress && !_waitInterrupted) {
        (void) _arrivalCondition.wait(&_arrivalMutex, deadline);
    }
    _waitInterrupted = false;
}

void QSerialPort::_interruptWait()
{
    {
        QMutexLocker locker(&_arrivalMutex);
        _waitInterrupted = true;
    }
    _arrivalCondition.wakeAll();
}

bool QSerialPort::waitForReadyRead(int msecs)
{
    const QDeadlineTimer deadline(msecs);

    // The backend looks for input at least once, even if the time is already up.
    bool expired = false;
    for (;;) {
        // Desktop QSerialPort also goes on sending pending writes while it waits for input, and gives the wait up
        // when one of them fails.
        if (!_pumpWrites()) {
            return false;
        }
        // Input that the owner may read already was announced before: only new input ends the wait.
        if (_admitArrivedInput()) {
            _announceAdmittedInput();
            return true;
        }
        if (expired) {
            qCDebug(AndroidSerialPortLog) << "Timeout while waiting for ready read on" << _systemLocation;
            _setError(TimeoutError, tr("Timeout while waiting for ready read"));
            return false;
        }

        if (!_backend) {
            // Like desktop QSerialPort, a port that is not open waits its time out.
            _waitForArrivedInput(deadline);
        } else if (!_backend->waitForInput(deadline)) {
            return false;
        }
        expired = deadline.hasExpired();
    }
}

bool QSerialPort::waitForBytesWritten(int msecs)
{
    if (bytesToWrite() == 0) {
        return false;
    }

    if (!_writeUntil(QDeadlineTimer(msecs))) {
        return false;
    }
    if (bytesToWrite() == 0) {
        return true;
    }

    qCDebug(AndroidSerialPortLog) << "Write timed out on" << _systemLocation << "with" << bytesToWrite()
                                  << "bytes left";
    _setError(TimeoutError, tr("Timeout while writing data"));
    return false;
}

qint64 QSerialPort::writeData(const char* data, qint64 maxSize)
{
    // QIODevice passes a write of nothing on, which is no error: one would keep close() from sending what waits.
    if (maxSize == 0) {
        return 0;
    }

    if (!data || (maxSize < 0)) {
        qCWarning(AndroidSerialPortLog) << "Invalid data or size in writeData for" << _systemLocation;
        _setError(WriteError, tr("Invalid data or size"));
        return -1;
    }

    // Like desktop QSerialPort, writes are buffered and handed to the device from the owner's event loop,
    // waitForBytesWritten(), waitForReadyRead(), flush() or close(), none of which waits for a device longer
    // than it was told to.
    _output.append(data, static_cast<qsizetype>(maxSize));
    _scheduleWrite();
    return maxSize;
}

void QSerialPort::_scheduleWrite()
{
    if (!_writePending.exchange(true)) {
        QMetaObject::invokeMethod(this, &QSerialPort::_pumpQueuedWrites, Qt::QueuedConnection);
    }
}

void QSerialPort::_pumpQueuedWrites()
{
    _writePending.store(false);
    if (isOpen()) {
        (void) _pumpWrites();
    }
}

bool QSerialPort::_pumpWrites()
{
    qint64 written = 0;
    {
        QMutexLocker locker(&_arrivalMutex);
        written = std::exchange(_completedWriteBytes, 0);
        _writeProgress = false;
    }
    const qint64 discarded = qMin(written, _discardedInFlight);
    _discardedInFlight -= discarded;
    written = qMin(written - discarded, _bytesInFlight);
    _bytesInFlight -= written;

    bool succeeded = true;
    if (_backend && (_bytesInFlight == 0) && !_output.isEmpty()) {
        const qint64 taken = _backend->startWrite(_output.constData(), _output.size());
        if (taken < 0) {
            succeeded = false;
        } else {
            if (taken > 0) {
                qCDebug(AndroidSerialPortTraceLog).noquote()
                    << _systemLocation << tracedTransfer("TX", _output.constData(), taken);
            }
            _output.remove(0, static_cast<qsizetype>(taken));
            _bytesInFlight += taken;
        }
    }

    if (written > 0) {
        emit bytesWritten(written);
    }

    return succeeded;
}

bool QSerialPort::_writeUntil(const QDeadlineTimer& deadline)
{
    for (;;) {
        if (!_pumpWrites()) {
            return false;
        }
        if (bytesToWrite() == 0) {
            return true;
        }

        AndroidSerialBackend* const backend = _openBackend();
        if (!backend) {
            return false;
        }

        bool progressWaiting = false;
        {
            QMutexLocker locker(&_arrivalMutex);
            progressWaiting = _writeProgress;
        }
        // Counted before the time is looked at: a device node takes output while it is handed over, and the
        // caller tells by bytesToWrite() whether the time ran out.
        if (progressWaiting) {
            continue;
        }
        if (deadline.hasExpired()) {
            return true;
        }
        if (!backend->waitForWritable(deadline)) {
            return false;
        }
    }
}

void QSerialPort::_flushBeforeClose()
{
    // Running out of time is no error here: the owner asked for the close, and a link shows every error to the
    // user.
    if (_writeUntil(QDeadlineTimer(CLOSE_WRITE_TIMEOUT_MS)) && (bytesToWrite() > 0)) {
        qCDebug(AndroidSerialPortLog) << "Closing" << _systemLocation << "with" << bytesToWrite() << "bytes unsent";
    }
}

bool QSerialPort::flush()
{
    if (!_openBackend()) {
        return false;
    }

    const qint64 queued = _output.size();
    return _pumpWrites() && (_output.size() < queued);
}

bool QSerialPort::clear(Directions directions)
{
    AndroidSerialBackend* const backend = _openBackend();
    if (!backend) {
        return false;
    }

    if (directions & Input) {
        QMutexLocker locker(&_arrivalMutex);
        _input.clear();
        _admitted = 0;
    }
    if (directions & Output) {
        _output.clear();
        _discardedInFlight += std::exchange(_bytesInFlight, 0);
    }

    const bool cleared = _clearDevice(backend, directions);
    // Only now, because a read of QIODevice has the backend read on.
    if (const qint64 held = QIODevice::bytesAvailable(); (directions & Input) && (held > 0)) {
        (void) skip(held);
    }

    return cleared;
}

bool QSerialPort::_clearDevice(AndroidSerialBackend* backend, Directions directions)
{
    // Before the port reads on, so that it reads nothing that the device held from before the clear. Input that
    // had already left a USB device by then still arrives: nothing can call it back.
    const bool cleared = backend->clear(directions);
    // The input that waited is discarded, and holds reading back no longer.
    const bool readingOn = !(directions & Input) || _readOnIfRoom();
    return cleared && readingOn;
}
