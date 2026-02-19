#include "qserialport_p.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <iterator>

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "Android.AndroidSerialPort")

QT_BEGIN_NAMESPACE

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation;

    AndroidSerial::registerPointer(this);

    _deviceId = AndroidSerial::open(systemLocation, this);
    if (_deviceId == INVALID_DEVICE_ID) {
        qCWarning(AndroidSerialPortLog) << "Error opening" << systemLocation;
        setError(QSerialPortErrorInfo(QSerialPort::DeviceNotFoundError));
        AndroidSerial::unregisterPointer(this);
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

void QSerialPortPrivate::exceptionArrived(const QString &ex)
{
    qCWarning(AndroidSerialPortLog) << "Exception arrived on device ID" << _deviceId << ":" << ex;
    setError(QSerialPortErrorInfo(QSerialPort::UnknownError, ex));
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

void QSerialPortPrivate::newDataArrived(const char *bytes, int length)
{
    // qCDebug(AndroidSerialPortLog) << "newDataArrived" << length;

    bool shouldStop = false;

    QMutexLocker locker(&_readMutex);
    int bytesToRead = length;
    if (readBufferMaxSize) {
        const qint64 totalBuffered = _pendingData.size()
            + _bufferBytesEstimate.load(std::memory_order_relaxed);
        const qint64 headroom = readBufferMaxSize - totalBuffered;
        if (bytesToRead > headroom) {
            bytesToRead = static_cast<int>(qMax(qint64(0), headroom));
            if (bytesToRead <= 0) {
                qCWarning(AndroidSerialPortLog) << "Read buffer exceeded maximum size. Stopping async read.";
                shouldStop = true;
            }
        }
    }

    if (!shouldStop) {
        _pendingData.append(bytes, bytesToRead);
        _readWaitCondition.wakeAll();
    }
    locker.unlock();

    if (shouldStop) {
        _stopAsyncRead();
        return;
    }

    _scheduleReadyRead();
}

void QSerialPortPrivate::_scheduleReadyRead()
{
    Q_Q(QSerialPort);

    if (!_readyReadPending.exchange(true)) {
        QPointer<QSerialPort> guard(q);
        QMetaObject::invokeMethod(q, [this, guard]() {
            if (!guard) {
                return;
            }

            bool shouldStop = false;
            _readyReadPending.store(false);
            QMutexLocker locker(&_readMutex);
            if (_pendingData.isEmpty()) {
                return;
            }

            if (readBufferMaxSize > 0) {
                const qint64 canAccept = readBufferMaxSize - buffer.size();
                if (canAccept <= 0) {
                    shouldStop = true;
                } else {
                    const qint64 toDrain = qMin(qint64(_pendingData.size()), canAccept);
                    buffer.append(_pendingData.constData(), toDrain);
                    if (toDrain >= _pendingData.size()) {
                        _pendingData.clear();
                    } else {
                        _pendingData = _pendingData.mid(static_cast<int>(toDrain));
                    }
                    if (!_pendingData.isEmpty()) {
                        shouldStop = true;
                    }
                }
            } else {
                buffer.append(_pendingData.constData(), _pendingData.size());
                _pendingData.clear();
            }

            _bufferBytesEstimate.store(buffer.size(), std::memory_order_relaxed);
            _readWaitCondition.wakeAll();
            locker.unlock();

            if (shouldStop) {
                _stopAsyncRead();
            }

            emit guard->readyRead();
        }, Qt::QueuedConnection);
    }
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    QMutexLocker locker(&_readMutex);
    if (!buffer.isEmpty()) {
        return true;
    }

    if (!_pendingData.isEmpty()) {
        buffer.append(_pendingData.constData(), _pendingData.size());
        _pendingData.clear();
        _bufferBytesEstimate.store(buffer.size(), std::memory_order_relaxed);
        return true;
    }

    QDeadlineTimer deadline(msecs);
    while (buffer.isEmpty() && _pendingData.isEmpty()) {
        if (!_readWaitCondition.wait(&_readMutex, deadline)) {
            break;
        }

        if (!buffer.isEmpty()) {
            return true;
        }

        if (!_pendingData.isEmpty()) {
            buffer.append(_pendingData.constData(), _pendingData.size());
            _pendingData.clear();
            _bufferBytesEstimate.store(buffer.size(), std::memory_order_relaxed);
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
    const bool result = _writeDataOneShot(msecs);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Timeout while waiting for bytes written on device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::TimeoutError, QSerialPort::tr("Timeout while waiting for bytes written")));
    }

    return result;
}

bool QSerialPortPrivate::_writeDataOneShot(int msecs)
{
    if (writeBuffer.isEmpty()) {
        return true;
    }

    qint64 pendingBytesWritten = 0;

    while (!writeBuffer.isEmpty()) {
        const char *dataPtr = writeBuffer.readPointer();
        const qint64 dataSize = writeBuffer.nextDataBlockSize();

        const qint64 written = _writeToPort(dataPtr, dataSize, msecs);
        if (written < 0) {
            qCWarning(AndroidSerialPortLog) << "Failed to write data one shot on device ID" << _deviceId;
            setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write data one shot")));
            return false;
        }

        writeBuffer.free(written);
        pendingBytesWritten += written;
    }

    const bool result = (pendingBytesWritten > 0);
    if (result) {
        Q_Q(QSerialPort);
        emit q->bytesWritten(pendingBytesWritten);
    }

    return result;
}

qint64 QSerialPortPrivate::_writeToPort(const char *data, qint64 maxSize, int timeout, bool async)
{
    const qint64 result = AndroidSerial::write(_deviceId, data, maxSize, timeout, async);
    if (result < 0) {
        qCWarning(AndroidSerialPortLog) << "Failed to write to port ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write to port")));
    }

    return result;
}

qint64 QSerialPortPrivate::writeData(const char *data, qint64 maxSize)
{
    if (!data || (maxSize <= 0)) {
        qCWarning(AndroidSerialPortLog) << "Invalid data or size in writeData for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Invalid data or size")));
        return -1;
    }

    return _writeToPort(data, maxSize);
}

bool QSerialPortPrivate::flush()
{
    const bool result = _writeDataOneShot();
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Flush operation failed for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to flush")));
    }

    return result;
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

bool QSerialPortPrivate::_setParameters(qint32 baudRate, QSerialPort::DataBits dataBits_, QSerialPort::StopBits stopBits_, QSerialPort::Parity parity_)
{
    const bool result = AndroidSerial::setParameters(_deviceId, baudRate, _dataBitsToAndroidDataBits(dataBits_), _stopBitsToAndroidStopBits(stopBits_), _parityToAndroidParity(parity_));
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
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Invalid baud rate value")));
        return false;
    }

    if (directions != QSerialPort::AllDirections) {
        qCWarning(AndroidSerialPortLog) << "Custom baud rate direction is unsupported:" << directions;
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Custom baud rate direction is unsupported")));
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
        return AndroidSerial::Data8; // Default to Data8
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
        return AndroidSerial::NoParity; // Default to NoParity
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
        return AndroidSerial::OneStop; // Default to OneStop
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
        return AndroidSerial::NoFlowControl; // Default to NoFlowControl
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
    50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
    9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000,
    576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000,
    3000000, 3500000, 4000000,
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
