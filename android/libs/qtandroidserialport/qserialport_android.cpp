#include "qserialport_android_p.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QMap>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <termios.h>
#include <errno.h>

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "qgc.android.libs.qtandroidserialport.qserialport_android")

QT_BEGIN_NAMESPACE

QSerialPortPrivate::QSerialPortPrivate(QSerialPort *q)
    : QSerialPortPrivateData(q)
{
    qCDebug(AndroidSerialPortLog) << Q_FUNC_INFO << this;
    // readBufferMaxSize = MAX_READ_SIZE;
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation.toLatin1().data();

    deviceId = AndroidSerial::open(systemLocation, this);

    if (!isDeviceValid()) {
        qCWarning(AndroidSerialPortLog) << "Error opening" << systemLocation.toLatin1().data();
        q_ptr->setError(QSerialPort::DeviceNotFoundError);
        return false;
    }

    descriptor = AndroidSerial::getDeviceHandle(deviceId);

    isReadStopped = false;
    if (mode == QIODevice::WriteOnly) {
        stopReadThread();
    }

    return true;
}

void QSerialPortPrivate::close()
{
    qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation.toLatin1().data();

    if (!isDeviceValid()) return;

    descriptor = -1;
    pendingBytesWritten = 0;
    deviceId = BAD_PORT;

    const bool result = AndroidSerial::close(deviceId);
    if (!result) {
        q_ptr->setErrorString(QStringLiteral("Closing device failed"));
    }
}

bool QSerialPortPrivate::setParameters(int baudRate, int dataBits, int stopBits, int parity)
{
    if (!isDeviceValid()) {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }

    const bool result = AndroidSerial::setParameters(deviceId, baudRate, dataBits, stopBits, parity);
    if (result) {
        inputBaudRate = outputBaudRate = baudRate;
        m_dataBits = dataBits;
        m_stopBits = stopBits;
        m_parity = parity;
    }

    return result;
}

void QSerialPortPrivate::stopReadThread()
{
    if (isReadStopped) return;
    if (AndroidSerial::stopReadThread(deviceId)) {
        isReadStopped = true;
    }
}

void QSerialPortPrivate::startReadThread()
{
    if (!isReadStopped) return;
    if (AndroidSerial::startReadThread(deviceId)) {
        isReadStopped = false;
    }
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    if (!isDeviceValid()) {
        q_ptr->setError(QSerialPort::NotOpenError);
        return QSerialPort::NoSignal;
    }

    return AndroidSerial::getControlLines(deviceId);
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    if (!isDeviceValid()) {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }

    return AndroidSerial::setDataTerminalReady(deviceId, set);
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    if (!isDeviceValid()) {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }

    return AndroidSerial::setRequestToSend(deviceId, set);
}

bool QSerialPortPrivate::flush()
{
    return writeDataOneShot();
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    if (!isDeviceValid()) {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }

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

    return AndroidSerial::flush(deviceId, input, output);
}

bool QSerialPortPrivate::sendBreak(int duration)
{
    bool result = false;

    if (m_breakEnabled) {
        result = AndroidSerial::setBreak(deviceId, true);
        QTimer* const timer = new QTimer(q_ptr);
        timer->setSingleShot(true);
        timer->setInterval(duration);
        (void) timer->callOnTimeout([this, timer]() {
            (void) AndroidSerial::setBreak(deviceId, false);
            timer->deleteLater();
        });
        timer->start();
    }

    return result;
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    m_breakEnabled = set;
    return true;
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    const int orig = static_cast<int>(readBuffer.size());

    if (orig > 0) {
        return true;
    }

    for (int i = 0; i < msecs; i++) {
        if (orig < readBuffer.size()) {
            return true;
        } else {
            QThread::msleep(1);
        }
    }

    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    internalWriteTimeoutMsec = msecs;
    const bool ret = writeDataOneShot();
    internalWriteTimeoutMsec = 0;
    return ret;
}

bool QSerialPortPrivate::setBaudRate()
{
    return setBaudRate(inputBaudRate, QSerialPort::AllDirections);
}

bool QSerialPortPrivate::setBaudRate(qint32 baudRate, QSerialPort::Directions directions)
{
    Q_UNUSED(directions);
    return setParameters(baudRate, m_dataBits, m_stopBits, m_parity);
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits)
{
    int numBits;

    switch (dataBits) {
    case QSerialPort::Data5:
        numBits = AndroidSerial::Data5;
        break;
    case QSerialPort::Data6:
        numBits = AndroidSerial::Data6;
        break;
    case QSerialPort::Data7:
        numBits = AndroidSerial::Data7;
        break;
    case QSerialPort::Data8:
    default:
        numBits = AndroidSerial::Data8;
        break;
    }

    return setParameters(inputBaudRate, numBits, m_stopBits, m_parity);
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity)
{
    int par;

    switch (parity) {
    case QSerialPort::SpaceParity:
        par = AndroidSerial::SpaceParity;
        break;
    case QSerialPort::MarkParity:
        par = AndroidSerial::MarkParity;
        break;
    case QSerialPort::EvenParity:
        par = AndroidSerial::EvenParity;
        break;
    case QSerialPort::OddParity:
        par = AndroidSerial::OddParity;
        break;
    case QSerialPort::NoParity:
    default:
        par = AndroidSerial::NoParity;
        break;
    }

    return setParameters(inputBaudRate, m_dataBits, m_stopBits, par);
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits)
{
    int stop;

    switch (stopBits) {
    case QSerialPort::TwoStop:
        stop = AndroidSerial::TwoStop;
        break;
    case QSerialPort::OneAndHalfStop:
        stop = AndroidSerial::OneAndHalfStop;
        break;
    case QSerialPort::OneStop:
    default:
        stop = AndroidSerial::OneStop;
        break;
    }

    return setParameters(inputBaudRate, m_dataBits, stop, m_parity);
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl)
{
    int control;

    switch (flowControl) {
    case QSerialPort::HardwareControl:
        control = AndroidSerial::RtsCtsFlowControl;
        break;
    case QSerialPort::SoftwareControl:
        control = AndroidSerial::XonXoffFlowControl;
        break;
    case QSerialPort::NoFlowControl:
    default:
        control = AndroidSerial::NoFlowControl;
        break;
    }

    m_flowControl = control;

    return AndroidSerial::setFlowControl(deviceId, control);
}

void QSerialPortPrivate::newDataArrived(char *bytes, int length)
{
    Q_Q(QSerialPort);

    int bytesToRead = length;

    // Always buffered, read data from the port into the read buffer
    if (readBufferMaxSize && (bytesToRead > (readBufferMaxSize - readBuffer.size()))) {
        bytesToRead = static_cast<int>(readBufferMaxSize - readBuffer.size());
        if (bytesToRead <= 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            stopReadThread();
            return;
        }
    }

    char *ptr = readBuffer.reserve(bytesToRead);
    memcpy(ptr, bytes, static_cast<size_t>(bytesToRead));

    emit q->readyRead();
}

void QSerialPortPrivate::exceptionArrived(QString str)
{
    q_ptr->setErrorString(str);
}

static QSerialPort::SerialPortError decodeSystemError()
{
    QSerialPort::SerialPortError error;

    switch (errno) {
    case ENODEV:
        error = QSerialPort::DeviceNotFoundError;
        break;
    case EACCES:
        error = QSerialPort::PermissionError;
        break;
    case EBUSY:
        error = QSerialPort::PermissionError;
        break;
    case EAGAIN:
        error = QSerialPort::ResourceError;
        break;
    case EIO:
        error = QSerialPort::ResourceError;
        break;
    case EBADF:
        error = QSerialPort::ResourceError;
        break;
    default:
        error = QSerialPort::UnknownError;
        break;
    }

    return error;
}

bool QSerialPortPrivate::writeDataOneShot()
{
    Q_Q(QSerialPort);

    pendingBytesWritten = -1;

    while (!writeBuffer.isEmpty()) {
        pendingBytesWritten = writeToPort(writeBuffer.readPointer(), writeBuffer.nextDataBlockSize());

        if (pendingBytesWritten <= 0) {
            QSerialPort::SerialPortError error = decodeSystemError();
            if (error != QSerialPort::ResourceError) {
                error = QSerialPort::WriteError;
            }
            q->setError(error);
            return false;
        }

        writeBuffer.free(pendingBytesWritten);

        emit q->bytesWritten(pendingBytesWritten);
    }

    return (pendingBytesWritten >= 0);
}

qint64 QSerialPortPrivate::writeToPort(const char *data, qint64 maxSize)
{
    if (!isDeviceValid()) {
        q_ptr->setError(QSerialPort::NotOpenError);
        return 0;
    }

    return AndroidSerial::write(deviceId, data, maxSize, internalWriteTimeoutMsec, false);
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

static const BaudRateMap& standardBaudRateMap()
{
    static const BaudRateMap baudRateMap = createStandardBaudRateMap();
    return baudRateMap;
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

qint64 QSerialPortPrivate::bytesToWrite() const
{
    return writeBuffer.size();
}

qint64 QSerialPortPrivate::writeData(const char *data, qint64 maxSize)
{
    return writeToPort(data, maxSize);
}

QT_END_NAMESPACE
