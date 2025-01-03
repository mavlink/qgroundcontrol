#include "qserialport_p.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMap>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <asm/termbits.h>

// TODO: Switch from device ID to serial number to support multiple USB connections

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "qgc.android.libs.qtandroidserialport.qserialport_android")

QT_BEGIN_NAMESPACE

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation.toLatin1().constData();

    _deviceId = AndroidSerial::open(systemLocation, this);
    if (_deviceId == INVALID_DEVICE_ID) {
        qCWarning(AndroidSerialPortLog) << "Error opening" << systemLocation.toLatin1().constData();
        setError(QSerialPortErrorInfo(QSerialPort::DeviceNotFoundError));
        return false;
    }

    descriptor = AndroidSerial::getDeviceHandle(_deviceId);
    if (descriptor == -1) {
        qCWarning(AndroidSerialPortLog) << "Failed to get device handle for" << systemLocation.toLatin1().constData();
        setError(QSerialPortErrorInfo(QSerialPort::OpenError));
        close();
        return false;
    }

    if (!_setParameters(inputBaudRate, dataBits, stopBits, parity)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port parameters for" << systemLocation.toLatin1().constData();
        close();
        return false;
    }

    if (!setFlowControl(flowControl)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port flow control for" << systemLocation.toLatin1().constData();
        close();
        return false;
    }

    if (mode & QIODevice::ReadOnly) {
        if (!startAsyncRead()) {
            qCWarning(AndroidSerialPortLog) << "Failed to start async read for" << systemLocation.toLatin1().constData();
            close();
            return false;
        }
    } else if (mode & QIODevice::WriteOnly) {
        if (!_stopAsyncRead()) {
            qCWarning(AndroidSerialPortLog) << "Failed to stop async read for" << systemLocation.toLatin1().constData();
        }
    }

    (void) clear(QSerialPort::AllDirections);

    return true;
}

void QSerialPortPrivate::close()
{
    qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation.toLatin1().constData();

    if (_readTimer) {
        _readTimer->stop();
        _readTimer->deleteLater();
        _readTimer = nullptr;
    }

    if (_deviceId != INVALID_DEVICE_ID) {
        if (!AndroidSerial::close(_deviceId)) {
            qCWarning(AndroidSerialPortLog) << "Failed to close device with ID" << _deviceId;
            setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Closing device failed")));
        }
        _deviceId = INVALID_DEVICE_ID;
    }

    descriptor = -1;
}

void QSerialPortPrivate::exceptionArrived(const QString &ex)
{
    qCWarning(AndroidSerialPortLog) << "Exception arrived on device ID" << _deviceId << ":" << ex;
    setError(QSerialPortErrorInfo(QSerialPort::UnknownError, ex));
}

bool QSerialPortPrivate::startAsyncRead()
{
    if (AndroidSerial::readThreadRunning(_deviceId)) {
        return true;
    }

    const bool result = AndroidSerial::startReadThread(_deviceId);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to start async read thread for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to start async read")));
    }

    Q_Q(QSerialPort);
    if (!_readTimer) {
        _readTimer = new QTimer(q);
        _readTimer->setInterval(EMIT_INTERVAL_MS);
        (void) QObject::connect(_readTimer, &QTimer::timeout, q, [this]() {
            Q_Q(QSerialPort);
            QMutexLocker locker(&_readMutex);
            if (!buffer.isEmpty()) {
                emit q->readyRead();
            }
        });
        _readTimer->start();
    }

    return result;
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

    if (_readTimer) {
        _readTimer->stop();
        _readTimer->deleteLater();
        _readTimer = nullptr;
    }

    return result;
}

void QSerialPortPrivate::newDataArrived(const char *bytes, int length)
{
    Q_Q(QSerialPort);

    // qCDebug(AndroidSerialPortLog) << "newDataArrived" << length;

    QMutexLocker locker(&_readMutex);
    int bytesToRead = length;
    if (readBufferMaxSize && (bytesToRead > (readBufferMaxSize - buffer.size()))) {
        bytesToRead = static_cast<int>(readBufferMaxSize - buffer.size());
        if (bytesToRead <= 0) {
            qCWarning(AndroidSerialPortLog) << "Read buffer exceeded maximum size. Stopping async read.";
            if (!_stopAsyncRead()) {
                qCWarning(AndroidSerialPortLog) << "Failed to stop async read.";
            }
            return;
        }
    }

    // qCDebug(AndroidSerialPortLog) << "nextDataBlockSize" << buffer.nextDataBlockSize();
    (void) buffer.append(bytes, bytesToRead);
    _readWaitCondition.wakeAll();
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    QMutexLocker locker(&_readMutex);
    if (!buffer.isEmpty()) {
        return true;
    }

    QDeadlineTimer deadline(msecs);
    while (buffer.isEmpty()) {
        if (!_readWaitCondition.wait(&_readMutex, deadline)) {
            break;
        }

        if (!buffer.isEmpty()) {
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
            // setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write data one shot")));
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
        // setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write to port")));
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

    const qint64 result = _writeToPort(data, maxSize);
    if (result < 0) {
        setError(QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write data")));
    }

    return result;
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
    bool input = false;
    bool output = false;

    if (directions == QSerialPort::AllDirections) {
        input = output = true;
    } else {
        if (directions & QSerialPort::Input) {
            input = true;
        }

        if (directions & QSerialPort::Output) {
            output = true;
        }
    }

    const bool result = AndroidSerial::purgeBuffers(_deviceId, input, output);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to purge buffers for device ID" << _deviceId;
        // setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to purge buffers")));
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

bool QSerialPortPrivate::_setParameters(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits, QSerialPort::Parity parity)
{
    const bool result = AndroidSerial::setParameters(_deviceId, baudRate, _dataBitsToAndroidDataBits(dataBits), _stopBitsToAndroidStopBits(stopBits), _parityToAndroidParity(parity));
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

    const qint32 standardBaudRate = _settingFromBaudRate(baudRate);
    if (standardBaudRate <= 0) {
        qCWarning(AndroidSerialPortLog) << "Unsupported Baud Rate:" << baudRate;
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Invalid Baud Rate")));
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


int QSerialPortPrivate::_dataBitsToAndroidDataBits(QSerialPort::DataBits dataBits)
{
    switch (dataBits) {
    case QSerialPort::Data5:
        return AndroidSerial::Data5;
    case QSerialPort::Data6:
        return AndroidSerial::Data6;
    case QSerialPort::Data7:
        return AndroidSerial::Data7;
    case QSerialPort::Data8:
        return AndroidSerial::Data8;
    default:
        qCWarning(AndroidSerialPortLog) << "Invalid Data Bits" << dataBits;
        return AndroidSerial::Data8; // Default to Data8
    }
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits)
{
    const bool result = _setParameters(inputBaudRate, dataBits, stopBits, parity);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set data bits for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set data bits")));
    }

    return result;
}

int QSerialPortPrivate::_parityToAndroidParity(QSerialPort::Parity parity)
{
    switch (parity) {
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
        qCWarning(AndroidSerialPortLog) << "Invalid parity type:" << parity;
        return AndroidSerial::NoParity; // Default to NoParity
    }
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity)
{
    const bool result = _setParameters(inputBaudRate, dataBits, stopBits, parity);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set parity for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set parity")));
    }

    return result;
}

int QSerialPortPrivate::_stopBitsToAndroidStopBits(QSerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case QSerialPort::TwoStop:
        return AndroidSerial::TwoStop;
    case QSerialPort::OneAndHalfStop:
        return AndroidSerial::OneAndHalfStop;
    case QSerialPort::OneStop:
        return AndroidSerial::OneStop;
    default:
        qCWarning(AndroidSerialPortLog) << "Invalid Stop Bits type:" << stopBits;
        return AndroidSerial::OneStop; // Default to OneStop
    }
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits)
{
    const bool result = _setParameters(inputBaudRate, dataBits, stopBits, parity);
    if (!result) {
        qCWarning(AndroidSerialPortLog) << "Failed to set StopBits for device ID" << _deviceId;
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set StopBits")));
    }

    return result;
}

int QSerialPortPrivate::_flowControlToAndroidFlowControl(QSerialPort::FlowControl flowControl)
{
    switch (flowControl) {
    case QSerialPort::HardwareControl:
        return AndroidSerial::RtsCtsFlowControl;
    case QSerialPort::SoftwareControl:
        return AndroidSerial::XonXoffFlowControl;
    case QSerialPort::NoFlowControl:
        return AndroidSerial::NoFlowControl;
    default:
        qCWarning(AndroidSerialPortLog) << "Invalid Flow Control type:" << flowControl;
        return AndroidSerial::NoFlowControl; // Default to NoFlowControl
    }
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl)
{
    const bool result = AndroidSerial::setFlowControl(_deviceId, _flowControlToAndroidFlowControl(flowControl));
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

typedef QMap<qint32, qint32> BaudRateMap;

static const BaudRateMap createStandardBaudRateMap()
{
    BaudRateMap baudRateMap;

#ifdef B50
    (void) baudRateMap.insert(50, B50);
#endif

#ifdef B75
    (void) baudRateMap.insert(75, B75);
#endif

#ifdef B110
    (void) baudRateMap.insert(110, B110);
#endif

#ifdef B134
    (void) baudRateMap.insert(134, B134);
#endif

#ifdef B150
    (void) baudRateMap.insert(150, B150);
#endif

#ifdef B200
    (void) baudRateMap.insert(200, B200);
#endif

#ifdef B300
    (void) baudRateMap.insert(300, B300);
#endif

#ifdef B600
    (void) baudRateMap.insert(600, B600);
#endif

#ifdef B1200
    (void) baudRateMap.insert(1200, B1200);
#endif

#ifdef B1800
    (void) baudRateMap.insert(1800, B1800);
#endif

#ifdef B2400
    (void) baudRateMap.insert(2400, B2400);
#endif

#ifdef B4800
    (void) baudRateMap.insert(4800, B4800);
#endif

#ifdef B7200
    (void) baudRateMap.insert(7200, B7200);
#endif

#ifdef B9600
    (void) baudRateMap.insert(9600, B9600);
#endif

#ifdef B14400
    (void) baudRateMap.insert(14400, B14400);
#endif

#ifdef B19200
    (void) baudRateMap.insert(19200, B19200);
#endif

#ifdef B28800
    (void) baudRateMap.insert(28800, B28800);
#endif

#ifdef B38400
    (void) baudRateMap.insert(38400, B38400);
#endif

#ifdef B57600
    (void) baudRateMap.insert(57600, B57600);
#endif

#ifdef B76800
    (void) baudRateMap.insert(76800, B76800);
#endif

#ifdef B115200
    (void) baudRateMap.insert(115200, B115200);
#endif

#ifdef B230400
    (void) baudRateMap.insert(230400, B230400);
#endif

#ifdef B460800
    (void) baudRateMap.insert(460800, B460800);
#endif

#ifdef B500000
    (void) baudRateMap.insert(500000, B500000);
#endif

#ifdef B576000
    (void) baudRateMap.insert(576000, B576000);
#endif

#ifdef B921600
    (void) baudRateMap.insert(921600, B921600);
#endif

#ifdef B1000000
    (void) baudRateMap.insert(1000000, B1000000);
#endif

#ifdef B1152000
    (void) baudRateMap.insert(1152000, B1152000);
#endif

#ifdef B1500000
    (void) baudRateMap.insert(1500000, B1500000);
#endif

#ifdef B2000000
    (void) baudRateMap.insert(2000000, B2000000);
#endif

#ifdef B2500000
    (void) baudRateMap.insert(2500000, B2500000);
#endif

#ifdef B3000000
    (void) baudRateMap.insert(3000000, B3000000);
#endif

#ifdef B3500000
    (void) baudRateMap.insert(3500000, B3500000);
#endif

#ifdef B4000000
    (void) baudRateMap.insert(4000000, B4000000);
#endif

    return baudRateMap;
}

static const BaudRateMap &standardBaudRateMap()
{
    static const BaudRateMap baudRateMap = createStandardBaudRateMap();
    return baudRateMap;
}

qint32 QSerialPortPrivate::_settingFromBaudRate(qint32 baudRate)
{
    return standardBaudRateMap().value(baudRate, 0);
}

QList<qint32> QSerialPortPrivate::standardBaudRates()
{
    return standardBaudRateMap().keys();
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->descriptor;
}

QT_END_NAMESPACE
