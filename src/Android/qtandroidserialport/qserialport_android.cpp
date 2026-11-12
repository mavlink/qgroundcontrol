#include <iterator>
#include <utility>

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QScopedValueRollback>

#include "QGCLoggingCategory.h"
#include "qserialport_p.h"
#include "qserialportinfo_p.h"

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "Android.AndroidSerialPort")

QT_BEGIN_NAMESPACE

QSerialPortBackend* QSerialPortPrivate::_backendFor(const QString& location)
{
    const bool usb = QSerialPortInfoPrivate::isUsbLocation(location);
    std::unique_ptr<QSerialPortBackend>& backend = usb ? _usbBackend : _posixBackend;
    if (!backend) {
        backend = usb ? createUsbSerialPortBackend(*this) : createPosixSerialPortBackend(*this);
    }

    return backend.get();
}

QSerialPortBackend* QSerialPortPrivate::_openBackend()
{
    if (!_backend) {
        setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
    }

    return _backend;
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation;

    QSerialPortBackend* const backend = _backendFor(systemLocation);
    if (!backend->open(mode)) {
        descriptor = -1;
        return false;
    }
    _backend = backend;

    if (!_setParameters(inputBaudRate, dataBits, stopBits, parity, QSerialPort::tr("Failed to set parameters"))) {
        close();
        return false;
    }

    if (!setFlowControl(flowControl)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port flow control for" << systemLocation;
        close();
        return false;
    }

    if (mode & QIODevice::ReadOnly) {
        if (!startAsyncRead()) {
            qCWarning(AndroidSerialPortLog) << "Failed to start async read for" << systemLocation;
            close();
            return false;
        }
    } else if (mode & QIODevice::WriteOnly) {
        if (!_stopAsyncRead()) {
            qCWarning(AndroidSerialPortLog) << "Failed to stop async read for" << systemLocation;
        }
    }

    (void) clear(QSerialPort::AllDirections);

    return true;
}

void QSerialPortPrivate::close()
{
    qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation;

    if (QSerialPortBackend* const backend = std::exchange(_backend, nullptr)) {
        backend->close();
    }
    descriptor = -1;
    _bytesInFlight = 0;
    _discardedInFlight = 0;

    // Only now can no more input or report arrive. Drop what was not delivered, so that neither a queued drain
    // nor a later re-open delivers stale bytes into the QIODevice buffers that close() clears.
    QMutexLocker locker(&_arrivalMutex);
    _discardPendingDataLocked();
    _readyReadPending.store(false);
    _completedWriteBytes = 0;
    _writeProgress = false;
    _waitInterrupted = false;
    _readingHeldBack = false;
}

bool QSerialPortPrivate::startAsyncRead()
{
    QSerialPortBackend* const backend = _openBackend();
    if (!backend) {
        return false;
    }

    bool inputWaits = false;
    bool hasRoom = false;
    {
        QMutexLocker locker(&_arrivalMutex);
        inputWaits = _pendingSizeLocked() > 0;
        hasRoom = _hasRoomForInputLocked();
        if (hasRoom) {
            _readingHeldBack = false;
        }
    }

    // A read may have made room for input that a full read buffer held back. Every read comes here, so a drain
    // is queued only when input waits. Whether the buffer has room for it is left to the drain: QIODevice may
    // call this with room reserved in the buffer for the read that it is carrying out.
    if (inputWaits) {
        _scheduleReadyRead();
    }

    // The backend reads only while the port has room. The drain that makes room asks it to read on.
    return !hasRoom || backend->startReading();
}

bool QSerialPortPrivate::_readOnIfRoom()
{
    {
        QMutexLocker locker(&_arrivalMutex);
        if (!_readingHeldBack || !_hasRoomForInputLocked()) {
            return true;
        }
    }

    return startAsyncRead();
}

bool QSerialPortPrivate::_stopAsyncRead()
{
    QSerialPortBackend* const backend = _openBackend();
    return backend && backend->stopReading();
}

qint64 QSerialPortPrivate::_drainPendingDataLocked(qint64 maxBytes)
{
    const qsizetype pendingSize = _pendingSizeLocked();
    if (pendingSize <= 0) {
        _pendingData.clear();
        _pendingDataOffset = 0;
        return 0;
    }

    qint64 toDrain = pendingSize;
    if (maxBytes >= 0) {
        toDrain = qMin(toDrain, maxBytes);
    }

    if (toDrain <= 0) {
        return 0;
    }

    buffer.append(_pendingData.constData() + _pendingDataOffset, toDrain);
    _pendingDataOffset += static_cast<qsizetype>(toDrain);

    if (_pendingDataOffset >= _pendingData.size()) {
        _pendingData.clear();
        _pendingDataOffset = 0;
    } else {
        // Compact occasionally to keep append operations efficient without
        // paying the cost on every drain.
        constexpr qsizetype kCompactThreshold = 4096;
        if (_pendingDataOffset >= kCompactThreshold && (_pendingDataOffset * 2) >= _pendingData.size()) {
            _compactPendingDataLocked();
        }
    }

    return toDrain;
}

qsizetype QSerialPortPrivate::_pendingSizeLocked() const
{
    return qMax<qsizetype>(0, _pendingData.size() - _pendingDataOffset);
}

void QSerialPortPrivate::_discardPendingDataLocked()
{
    _pendingData.clear();
    _pendingDataOffset = 0;
}

void QSerialPortPrivate::_compactPendingDataLocked()
{
    if (_pendingDataOffset <= 0) {
        return;
    }

    if (_pendingDataOffset >= _pendingData.size()) {
        _pendingData.clear();
        _pendingDataOffset = 0;
        return;
    }

    _pendingData.remove(0, _pendingDataOffset);
    _pendingDataOffset = 0;
}

bool QSerialPortPrivate::newDataArrived(const char* bytes, int length)
{
    // qCDebug(AndroidSerialPortLog) << "newDataArrived" << length;

    bool hasRoom = false;
    {
        QMutexLocker locker(&_arrivalMutex);
        constexpr qsizetype kCompactBeforeAppendThreshold = 8192;
        if (_pendingDataOffset >= kCompactBeforeAppendThreshold) {
            _compactPendingDataLocked();
        }
        // Never dropped, like the input of desktop QSerialPort, which leaves what its buffer has no room for with
        // the kernel. Here the backend leaves it with the device once it hears that there is no room.
        _pendingData.append(bytes, length);
        hasRoom = _hasRoomForInputLocked();
        _readingHeldBack = !hasRoom;
        _arrivalCondition.wakeAll();
    }

    _scheduleReadyRead();
    return hasRoom;
}

bool QSerialPortPrivate::hasRoomForInput()
{
    QMutexLocker locker(&_arrivalMutex);
    return _hasRoomForInputLocked();
}

bool QSerialPortPrivate::_hasRoomForInputLocked() const
{
    // The read buffer is filled on the port's thread, so its size cannot be read here, on the thread that
    // delivers. The limit therefore applies to the input that waits for the buffer as well, by itself. Reading
    // is held back only while that input alone has reached the limit, which ends when the buffer takes it.
    return (readBufferMaxSize <= 0) || (_pendingSizeLocked() < readBufferMaxSize);
}

void QSerialPortPrivate::_scheduleReadyRead()
{
    Q_Q(QSerialPort);

    if (!_readyReadPending.exchange(true)) {
        QPointer<QSerialPort> guard(q);
        QMetaObject::invokeMethod(
            q,
            [this, guard]() {
                if (!guard || !guard->isOpen()) {
                    // Device closed before the queued drain ran. close() clears the
                    // QIODevice read buffers, so draining into them would assert.
                    _readyReadPending.store(false);
                    return;
                }

                QMutexLocker locker(&_arrivalMutex);
                const bool taken = _takeInputLocked();
                // Reset only after the drain, so that input arriving during it does not queue a second one.
                // Input that the read buffer has no room for waits for a read to make room and schedule this.
                _readyReadPending.store(false);
                locker.unlock();

                if (taken) {
                    _announceTakenInput();
                }
            },
            Qt::QueuedConnection);
    }
}

bool QSerialPortPrivate::_canTakeInputLocked() const
{
    return (_pendingSizeLocked() > 0) && ((readBufferMaxSize <= 0) || (buffer.size() < readBufferMaxSize));
}

bool QSerialPortPrivate::_takeInputLocked()
{
    if (!_canTakeInputLocked()) {
        return false;
    }

    (void) _drainPendingDataLocked((readBufferMaxSize > 0) ? (readBufferMaxSize - buffer.size()) : -1);
    return true;
}

void QSerialPortPrivate::_announceTakenInput()
{
    Q_Q(QSerialPort);

    // Here, and not only in QSerialPort::readData(), which QIODevice calls only for a read that wants more than
    // its buffer holds or that empties it. Before the announcement, whose receiver may wait for more input.
    (void) _readOnIfRoom();
    // The port's owner may have closed it over an error of that.
    if (q->isOpen()) {
        _emitReadyRead();
    }
}

void QSerialPortPrivate::_emitReadyRead()
{
    Q_Q(QSerialPort);

    // Like every QIODevice, not from inside one of its own slots, which may be waiting for more input.
    if (_emittingReadyRead) {
        return;
    }

    const QScopedValueRollback<bool> emitting(_emittingReadyRead, true);
    emit q->readyRead();
}

bool QSerialPortPrivate::_takeArrivedInput()
{
    QMutexLocker locker(&_arrivalMutex);
    return _takeInputLocked();
}

bool QSerialPortPrivate::inputArrived()
{
    QMutexLocker locker(&_arrivalMutex);
    return _canTakeInputLocked();
}

void QSerialPortPrivate::waitForArrivedInput(const QDeadlineTimer& deadline)
{
    QMutexLocker locker(&_arrivalMutex);
    if (!_canTakeInputLocked() && !_writeProgress && !_waitInterrupted) {
        (void) _arrivalCondition.wait(&_arrivalMutex, deadline);
    }
    _waitInterrupted = false;
}

void QSerialPortPrivate::writeCompleted(qint64 bytes)
{
    {
        QMutexLocker locker(&_arrivalMutex);
        _completedWriteBytes += bytes;
        _writeProgress = true;
    }
    _arrivalCondition.wakeAll();

    _scheduleWrite();
}

void QSerialPortPrivate::waitForWriteProgress(const QDeadlineTimer& deadline)
{
    QMutexLocker locker(&_arrivalMutex);
    if (!_writeProgress && !_waitInterrupted) {
        (void) _arrivalCondition.wait(&_arrivalMutex, deadline);
    }
    _waitInterrupted = false;
}

void QSerialPortPrivate::interruptWait()
{
    {
        QMutexLocker locker(&_arrivalMutex);
        _waitInterrupted = true;
    }
    _arrivalCondition.wakeAll();
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
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
        // Input that is already in the read buffer was announced before: only new input ends the wait.
        if (_takeArrivedInput()) {
            _announceTakenInput();
            return true;
        }
        if (expired) {
            qCDebug(AndroidSerialPortLog) << "Timeout while waiting for ready read on" << systemLocation;
            setError(QSerialPortErrorInfo(QSerialPort::TimeoutError,
                                          QSerialPort::tr("Timeout while waiting for ready read")));
            return false;
        }

        if (!_backend) {
            // Like desktop QSerialPort, a port that is not open waits its time out.
            waitForArrivedInput(deadline);
        } else if (!_backend->waitForInput(deadline)) {
            return false;
        }
        expired = deadline.hasExpired();
    }
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    if (writeBuffer.isEmpty() && (_bytesInFlight == 0)) {
        return false;
    }

    if (!_writeUntil(QDeadlineTimer(msecs))) {
        return false;
    }
    if (writeBuffer.isEmpty() && (_bytesInFlight == 0)) {
        return true;
    }

    qCDebug(AndroidSerialPortLog) << "Write timed out on" << systemLocation << "with"
                                  << (writeBuffer.size() + _bytesInFlight) << "bytes left";
    setError(QSerialPortErrorInfo(QSerialPort::TimeoutError, QSerialPort::tr("Timeout while writing data")));
    return false;
}

void QSerialPortPrivate::flushBeforeClose()
{
    // Running out of time is no error here: the owner asked for the close, and a link shows every error to the
    // user.
    if (_writeUntil(QDeadlineTimer(CLOSE_WRITE_TIMEOUT)) && (!writeBuffer.isEmpty() || (_bytesInFlight > 0))) {
        qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation << "with" << (writeBuffer.size() + _bytesInFlight)
                                      << "bytes unsent";
    }
}

bool QSerialPortPrivate::_pumpWrites()
{
    Q_Q(QSerialPort);

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
    if (_backend && (_bytesInFlight == 0) && !writeBuffer.isEmpty()) {
        const qint64 taken = _backend->startWrite(writeBuffer.readPointer(), writeBuffer.nextDataBlockSize());
        if (taken < 0) {
            succeeded = false;
        } else {
            writeBuffer.free(taken);
            _bytesInFlight += taken;
        }
    }

    if (written > 0) {
        emit q->bytesWritten(written);
    }

    return succeeded;
}

bool QSerialPortPrivate::_writeUntil(const QDeadlineTimer& deadline)
{
    for (;;) {
        if (!_pumpWrites()) {
            return false;
        }
        if (writeBuffer.isEmpty() && (_bytesInFlight == 0)) {
            return true;
        }

        QSerialPortBackend* const backend = _openBackend();
        if (!backend) {
            return false;
        }
        if (deadline.hasExpired()) {
            return true;
        }

        bool progressWaiting = false;
        {
            QMutexLocker locker(&_arrivalMutex);
            progressWaiting = _writeProgress;
        }
        if (!progressWaiting && !backend->waitForWritable(deadline)) {
            return false;
        }
    }
}

void QSerialPortPrivate::_scheduleWrite()
{
    Q_Q(QSerialPort);

    if (_writePending.exchange(true)) {
        return;
    }

    QPointer<QSerialPort> guard(q);
    QMetaObject::invokeMethod(
        q,
        [this, guard]() {
            if (!guard) {
                return;
            }
            _writePending.store(false);
            if (guard->isOpen()) {
                (void) _pumpWrites();
            }
        },
        Qt::QueuedConnection);
}

qint64 QSerialPortPrivate::writeData(const char* data, qint64 maxSize)
{
    // QIODevice passes a write of nothing on, which is no error: one would keep close() from sending what waits.
    if (maxSize == 0) {
        return 0;
    }

    if (!data || (maxSize < 0)) {
        qCWarning(AndroidSerialPortLog) << "Invalid data or size in writeData for" << systemLocation;
        setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Invalid data or size")));
        return -1;
    }

    // Like desktop QSerialPort, writes are buffered and handed to the device from the owner's event loop,
    // waitForBytesWritten(), waitForReadyRead(), flush() or close(), none of which waits for a device longer
    // than it was told to.
    writeBuffer.append(data, maxSize);
    _scheduleWrite();
    return maxSize;
}

bool QSerialPortPrivate::flush()
{
    const qint64 buffered = writeBuffer.size();
    return _pumpWrites() && (writeBuffer.size() < buffered);
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    if (directions & QSerialPort::Output) {
        _discardedInFlight += std::exchange(_bytesInFlight, 0);
    }

    QSerialPortBackend* const backend = _openBackend();
    if (!backend) {
        return false;
    }
    // Before the port reads on, so that it reads nothing that the device held from before the clear. Input that
    // had already left a USB device by then still arrives: nothing can call it back.
    const bool cleared = backend->clear(directions);
    // The input that waited for the read buffer is discarded, and holds reading back no longer.
    const bool readingOn = !(directions & QSerialPort::Input) || _readOnIfRoom();
    return cleared && readingOn;
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    QSerialPortBackend* const backend = _openBackend();
    return backend ? backend->pinoutSignals() : QSerialPort::NoSignal;
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    QSerialPortBackend* const backend = _openBackend();
    return backend && backend->setDataTerminalReady(set);
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    QSerialPortBackend* const backend = _openBackend();
    return backend && backend->setRequestToSend(set);
}

bool QSerialPortPrivate::_setParameters(qint32 baudRate, QSerialPort::DataBits dataBits_,
                                        QSerialPort::StopBits stopBits_, QSerialPort::Parity parity_,
                                        const QString& failureText)
{
    QSerialPortBackend* const backend = _openBackend();
    return backend && backend->setParameters(baudRate, dataBits_, stopBits_, parity_, failureText);
}

bool QSerialPortPrivate::setBaudRate()
{
    return setBaudRate(inputBaudRate, QSerialPort::AllDirections);
}

bool QSerialPortPrivate::setBaudRate(qint32 baudRate, QSerialPort::Directions directions)
{
    if (baudRate <= 0) {
        qCWarning(AndroidSerialPortLog) << "Invalid baud rate value:" << baudRate;
        setError(
            QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Invalid baud rate value")));
        return false;
    }

    if (directions != QSerialPort::AllDirections) {
        qCWarning(AndroidSerialPortLog) << "Custom baud rate direction is unsupported:" << directions;
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError,
                                      QSerialPort::tr("Custom baud rate direction is unsupported")));
        return false;
    }

    const bool result =
        _setParameters(baudRate, dataBits, stopBits, parity, QSerialPort::tr("Failed to set baud rate"));
    if (result) {
        inputBaudRate = outputBaudRate = baudRate;
    }

    return result;
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits_)
{
    return _setParameters(inputBaudRate, dataBits_, stopBits, parity, QSerialPort::tr("Failed to set data bits"));
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity_)
{
    return _setParameters(inputBaudRate, dataBits, stopBits, parity_, QSerialPort::tr("Failed to set parity"));
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits_)
{
    return _setParameters(inputBaudRate, dataBits, stopBits_, parity, QSerialPort::tr("Failed to set stop bits"));
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl_)
{
    QSerialPortBackend* const backend = _openBackend();
    return backend && backend->setFlowControl(flowControl_);
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    QSerialPortBackend* const backend = _openBackend();
    return backend && backend->setBreakEnabled(set);
}

static constexpr qint32 kStandardBaudRates[] = {
    50,     75,     110,     134,     150,     200,     300,     600,     1200,    1800,
    2400,   4800,   9600,    19200,   38400,   57600,   115200,  230400,  460800,  500000,
    576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000,
};

QList<qint32> QSerialPortPrivate::standardBaudRates()
{
    return QList<qint32>(std::begin(kStandardBaudRates), std::end(kStandardBaudRates));
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->descriptor;
}

QT_END_NAMESPACE
