#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>

#include <iterator>

#include "QGCLoggingCategory.h"
#include "qserialport_p.h"

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "Android.AndroidSerialPort")

QT_BEGIN_NAMESPACE

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation;

    AndroidSerial::registerPointer(this);
    auto tokenGuard = qScopeGuard([this]() { AndroidSerial::unregisterPointer(this); });

    _deviceId = AndroidSerial::open(systemLocation, this);
    if (_deviceId == INVALID_DEVICE_ID) {
        qCWarning(AndroidSerialPortLog) << "Error opening" << systemLocation;
        setError(QSerialPortErrorInfo(QSerialPort::DeviceNotFoundError));
        return false;
    }

    descriptor = AndroidSerial::getDeviceHandle(_deviceId);
    if (descriptor == -1) {
        qCWarning(AndroidSerialPortLog) << "Failed to get device handle for" << systemLocation;
        setError(QSerialPortErrorInfo(QSerialPort::OpenError));
        close();
        return false;
    }

    if (!_setParameters(inputBaudRate, dataBits, stopBits, parity)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port parameters for" << systemLocation;
        close();
        return false;
    }

    if (!setFlowControl(flowControl)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port flow control for" << systemLocation;
        close();
        return false;
    }

    // Assert DTR — chips like CP210x and CH340 gate UART output on this line.
    // Linux kernel drivers assert DTR implicitly on open(); the Android USB host API does not.
    if (!AndroidSerial::setDataTerminalReady(_deviceId, true)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set DTR for" << systemLocation;
    }

    if (mode & QIODevice::ReadOnly) {
        if (!startAsyncRead()) {
            qCWarning(AndroidSerialPortLog) << "Failed to start async read for" << systemLocation;
            close();
            return false;
        }
    }

    (void)clear(QSerialPort::AllDirections);
    tokenGuard.dismiss();

    return true;
}

void QSerialPortPrivate::close()
{
    qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation;

    _stopAsyncRead();

    if (_deviceId != INVALID_DEVICE_ID) {
        if (!AndroidSerial::close(_deviceId)) {
            qCWarning(AndroidSerialPortLog) << "Failed to close device with ID" << _deviceId;
            setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Closing device failed")));
        }
        _deviceId = INVALID_DEVICE_ID;
    }

    descriptor = -1;

    AndroidSerial::unregisterPointer(this);
}

void QSerialPortPrivate::exceptionArrived(const QString& ex)
{
    qCWarning(AndroidSerialPortLog) << "Exception arrived on device ID" << _deviceId << ":" << ex;

    // Map Java IOExceptions to ResourceError for hot-unplug detection.
    // Code checking error() == ResourceError (standard Qt pattern) will otherwise never trigger.
    const bool isResourceError = ex.contains(QLatin1String("IOException"), Qt::CaseInsensitive)
                                 || ex.contains(QLatin1String("USB device disconnected"), Qt::CaseInsensitive)
                                 || ex.contains(QLatin1String("device not found"), Qt::CaseInsensitive)
                                 || ex.contains(QLatin1String("connection closed"), Qt::CaseInsensitive);

    setError(QSerialPortErrorInfo(isResourceError ? QSerialPort::ResourceError : QSerialPort::UnknownError, ex));
}

bool QSerialPortPrivate::startAsyncRead()
{
    if (!AndroidSerial::readThreadRunning(_deviceId)) {
        const bool result = AndroidSerial::startReadThread(_deviceId);
        if (!result) {
            qCWarning(AndroidSerialPortLog) << "Failed to start async read thread for device ID" << _deviceId;
            setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to start async read")));
            return false;
        }
    }

    // If pending bytes were left behind due to read buffer backpressure,
    // schedule another drain as soon as reads are active again.
    _scheduleReadyRead();

    return true;
}

bool QSerialPortPrivate::_stopAsyncRead()
{
    bool result = true;

    if (AndroidSerial::readThreadRunning(_deviceId)) {
        result = AndroidSerial::stopReadThread(_deviceId);
        if (!result) {
            qCWarning(AndroidSerialPortLog) << "Failed to stop async read thread for device ID" << _deviceId;
            setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to stop async read")));
        }
    }

    return result;
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

    _bufferBytesEstimate.store(buffer.size(), std::memory_order_relaxed);
    return toDrain;
}

qsizetype QSerialPortPrivate::_pendingSizeLocked() const
{
    return qMax<qsizetype>(0, _pendingData.size() - _pendingDataOffset);
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

void QSerialPortPrivate::newDataArrived(const char* bytes, int length)
{
    // qCDebug(AndroidSerialPortLog) << "newDataArrived" << length;

    qint64 droppedBytes = 0;

    QMutexLocker locker(&_readMutex);
    int bytesToRead = length;
    if (readBufferMaxSize) {
        const qint64 totalBuffered = _pendingSizeLocked() + _bufferBytesEstimate.load(std::memory_order_relaxed);
        const qint64 headroom = readBufferMaxSize - totalBuffered;
        if (bytesToRead > headroom) {
            bytesToRead = static_cast<int>(qMax(qint64(0), headroom));
            droppedBytes = static_cast<qint64>(length - bytesToRead);
        }
    }

    if (bytesToRead > 0) {
        constexpr qsizetype kCompactBeforeAppendThreshold = 8192;
        if (_pendingDataOffset >= kCompactBeforeAppendThreshold) {
            _compactPendingDataLocked();
        }
        _pendingData.append(bytes, bytesToRead);
        _readWaitCondition.wakeAll();
    }
    locker.unlock();

    if (droppedBytes > 0) {
        qCWarning(AndroidSerialPortLog) << "Read buffer full, dropping" << droppedBytes << "bytes";
    }

    if (bytesToRead <= 0) {
        return;
    }

    _scheduleReadyRead();
}

void QSerialPortPrivate::_scheduleReadyRead()
{
    Q_Q(QSerialPort);

    if (!_readyReadPending.exchange(true)) {
        QPointer<QSerialPort> guard(q);
        QMetaObject::invokeMethod(
            q,
            [this, guard]() {
                if (!guard) {
                    return;
                }

                QMutexLocker locker(&_readMutex);
                if (_pendingSizeLocked() <= 0) {
                    _readyReadPending.store(false);
                    return;
                }

                if (readBufferMaxSize > 0) {
                    const qint64 canAccept = readBufferMaxSize - buffer.size();
                    if (canAccept > 0) {
                        (void)_drainPendingDataLocked(canAccept);
                    }
                } else {
                    (void)_drainPendingDataLocked();
                }

                const bool more = (_pendingSizeLocked() > 0);

                _readWaitCondition.wakeAll();
                locker.unlock();

                emit guard->readyRead();

                // Reset flag AFTER emit to prevent re-entrant scheduling from
                // inside a readyRead slot (matches Qt's emittedReadyRead guard).
                // If pending data remains, reschedule for the next event loop pass.
                _readyReadPending.store(false);

                if (more) {
                    _scheduleReadyRead();
                }
            },
            Qt::QueuedConnection);
    }
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    QMutexLocker locker(&_readMutex);
    if (!buffer.isEmpty()) {
        return true;
    }

    if (_pendingSizeLocked() > 0) {
        (void)_drainPendingDataLocked();
        return true;
    }

    QDeadlineTimer deadline(msecs);
    while (buffer.isEmpty() && (_pendingSizeLocked() <= 0)) {
        if (!_readWaitCondition.wait(&_readMutex, deadline)) {
            break;
        }

        if (!buffer.isEmpty()) {
            return true;
        }

        if (_pendingSizeLocked() > 0) {
            (void)_drainPendingDataLocked();
            return true;
        }
    }
    locker.unlock();

    qCWarning(AndroidSerialPortLog) << "Timeout while waiting for ready read on device ID" << _deviceId;
    setError(QSerialPortErrorInfo(QSerialPort::TimeoutError, QSerialPort::tr("Timeout while waiting for ready read")));

    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs);
    // Writes are sent to the Java IO manager immediately in writeData() via async write.
    // There is no internal write buffer to drain, so this is always satisfied.
    return true;
}

qint64 QSerialPortPrivate::_writeToPort(const char* data, qint64 maxSize, int timeout, bool async)
{
    const qint64 result = AndroidSerial::write(_deviceId, data, maxSize, timeout, async);
    if (result < 0) {
        qCWarning(AndroidSerialPortLog) << "Failed to write to port ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write to port")));
    }

    return result;
}

qint64 QSerialPortPrivate::writeData(const char* data, qint64 maxSize)
{
    if (!data || (maxSize <= 0)) {
        qCWarning(AndroidSerialPortLog) << "Invalid data or size in writeData for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Invalid data or size")));
        return -1;
    }

    // Use async write to avoid blocking the caller. Qt's contract requires writeData()
    // to return immediately after accepting data. The Java IO manager handles the
    // actual USB transfer on its own thread.
    const qint64 written = _writeToPort(data, maxSize, DEFAULT_WRITE_TIMEOUT, /*async=*/true);
    if (written < 0) {
        return -1;
    }

    if (written > 0) {
        Q_Q(QSerialPort);
        const qint64 w = written;
        QMetaObject::invokeMethod(
            q, [q, w]() { emit q->bytesWritten(w); }, Qt::QueuedConnection);
    }

    return written;
}

bool QSerialPortPrivate::flush()
{
    // Writes are sent to the Java IO manager immediately in writeData() via async write.
    // There is no internal write buffer to flush.
    return true;
}

bool QSerialPortPrivate::sendBreak(int duration)
{
    Q_UNUSED(duration);
    // Timed break is not supported over Android USB host API.
    // setBreakEnabled(true/false) is available for untimed break.
    return false;
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    const bool input = directions & QSerialPort::Input;
    const bool output = directions & QSerialPort::Output;

    const bool result = AndroidSerial::purgeBuffers(_deviceId, input, output);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to purge buffers for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to purge buffers")));
    }

    return result;
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    return AndroidSerial::getControlLines(_deviceId);
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    const bool result = AndroidSerial::setDataTerminalReady(_deviceId, set);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set DTR for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set DTR")));
    }

    return result;
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    const bool result = AndroidSerial::setRequestToSend(_deviceId, set);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set RTS for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set RTS")));
    }

    return result;
}

bool QSerialPortPrivate::_setParameters(qint32 baudRate, QSerialPort::DataBits dataBits_,
                                        QSerialPort::StopBits stopBits_, QSerialPort::Parity parity_)
{
    const bool result =
        AndroidSerial::setParameters(_deviceId, baudRate, _dataBitsToAndroidDataBits(dataBits_),
                                     _stopBitsToAndroidStopBits(stopBits_), _parityToAndroidParity(parity_));
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set Parameters for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set parameters")));
    }

    return result;
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

    const bool result = _setParameters(baudRate, dataBits, stopBits, parity);
    if (result) {
        inputBaudRate = outputBaudRate = baudRate;
    } else {
        qCWarning(AndroidSerialPortLog) << "Failed to set baud rate for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set baud rate")));
    }

    return result;
}

int QSerialPortPrivate::_dataBitsToAndroidDataBits(QSerialPort::DataBits dataBits_)
{
    switch (dataBits_) {
        case QSerialPort::Data5:
            return AndroidSerial::Data5;
        case QSerialPort::Data6:
            return AndroidSerial::Data6;
        case QSerialPort::Data7:
            return AndroidSerial::Data7;
        case QSerialPort::Data8:
            return AndroidSerial::Data8;
        default:
            qCWarning(AndroidSerialPortLog) << "Invalid Data Bits" << dataBits_;
            return AndroidSerial::Data8;  // Default to Data8
    }
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits_)
{
    const bool result = _setParameters(inputBaudRate, dataBits_, stopBits, parity);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set data bits for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set data bits")));
    }

    return result;
}

int QSerialPortPrivate::_parityToAndroidParity(QSerialPort::Parity parity_)
{
    switch (parity_) {
        case QSerialPort::SpaceParity:
            return AndroidSerial::SpaceParity;
        case QSerialPort::MarkParity:
            return AndroidSerial::MarkParity;
        case QSerialPort::EvenParity:
            return AndroidSerial::EvenParity;
        case QSerialPort::OddParity:
            return AndroidSerial::OddParity;
        case QSerialPort::NoParity:
            return AndroidSerial::NoParity;
        default:
            qCWarning(AndroidSerialPortLog) << "Invalid parity type:" << parity_;
            return AndroidSerial::NoParity;  // Default to NoParity
    }
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity_)
{
    const bool result = _setParameters(inputBaudRate, dataBits, stopBits, parity_);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set parity for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set parity")));
    }

    return result;
}

int QSerialPortPrivate::_stopBitsToAndroidStopBits(QSerialPort::StopBits stopBits_)
{
    switch (stopBits_) {
        case QSerialPort::TwoStop:
            return AndroidSerial::TwoStop;
        case QSerialPort::OneAndHalfStop:
            return AndroidSerial::OneAndHalfStop;
        case QSerialPort::OneStop:
            return AndroidSerial::OneStop;
        default:
            qCWarning(AndroidSerialPortLog) << "Invalid Stop Bits type:" << stopBits_;
            return AndroidSerial::OneStop;  // Default to OneStop
    }
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits_)
{
    const bool result = _setParameters(inputBaudRate, dataBits, stopBits_, parity);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set StopBits for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set StopBits")));
    }

    return result;
}

int QSerialPortPrivate::_flowControlToAndroidFlowControl(QSerialPort::FlowControl flowControl_)
{
    switch (flowControl_) {
        case QSerialPort::HardwareControl:
            return AndroidSerial::RtsCtsFlowControl;
        case QSerialPort::SoftwareControl:
            return AndroidSerial::XonXoffFlowControl;
        case QSerialPort::NoFlowControl:
            return AndroidSerial::NoFlowControl;
        default:
            qCWarning(AndroidSerialPortLog) << "Invalid Flow Control type:" << flowControl_;
            return AndroidSerial::NoFlowControl;  // Default to NoFlowControl
    }
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl_)
{
    const bool result = AndroidSerial::setFlowControl(_deviceId, _flowControlToAndroidFlowControl(flowControl_));
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set Flow Control for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set Flow Control")));
    }

    return result;
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    const bool result = AndroidSerial::setBreak(_deviceId, set);
    if (!result) {
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set Break Enabled")));
    }

    return result;
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
