#include "qserialport_p.h"
#include <QGC.h>
#include <QGCLoggingCategory.h>

#include <QtCore/QMap>
#include <QtCore/QTimer>

#include <asm/termbits.h>

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "qgc.android.libs.qtandroidserialport.qserialport_android")

QT_BEGIN_NAMESPACE

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation.toLatin1().constData();

    m_deviceId = AndroidSerial::open(systemLocation, this);
    if (m_deviceId == BAD_PORT) {
        qCWarning(AndroidSerialPortLog) << "Error opening" << systemLocation.toLatin1().constData();
        setError(QSerialPortErrorInfo(QSerialPort::DeviceNotFoundError));
        return false;
    }

    descriptor = AndroidSerial::getDeviceHandle(m_deviceId);
    if (descriptor == -1) {
        setError(QSerialPortErrorInfo(QSerialPort::OpenError));
        close();
        return false;
    }

    if (!setDataBits(dataBits) ||
        !setParity(parity) ||
        !setStopBits(stopBits) ||
        !setFlowControl(flowControl) ||
        !setBaudRate()) {
        close();
        return false;
    }

    if (mode & QIODevice::ReadOnly) {
        // readBufferMaxSize = AndroidSerial::getReadBufferSize();
        startAsyncRead();
    }

    /*if (!clear(QSerialPort::AllDirections)) {
        close();
        return false;
    }*/

    return true;
}

void QSerialPortPrivate::close()
{
    qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation.toLatin1().data();

    descriptor = -1;
    m_pendingBytesWritten = 0;
    m_deviceId = BAD_PORT;

    if (!AndroidSerial::close(m_deviceId)) {
        setError(QSerialPortErrorInfo(QSerialPort::ResourceError, QStringLiteral("Closing device failed")));
    }
}

bool QSerialPortPrivate::_stopAsyncRead()
{
    if (!AndroidSerial::readThreadRunning(m_deviceId)) {
        return true;
    }
    return AndroidSerial::stopReadThread(m_deviceId);
}

bool QSerialPortPrivate::startAsyncRead()
{
    if (AndroidSerial::readThreadRunning(m_deviceId)) {
        return true;
    }
    return AndroidSerial::startReadThread(m_deviceId);
}

void QSerialPortPrivate::newDataArrived(char *bytes, int length)
{
    Q_Q(QSerialPort);

    if (!q->isOpen()) {
        return;
    }

    int bytesToRead = length;
    if (readBufferMaxSize && (bytesToRead > (readBufferMaxSize - buffer.size()))) {
        bytesToRead = static_cast<int>(readBufferMaxSize - buffer.size());
        if (bytesToRead <= 0) {
            (void) _stopAsyncRead();
            return;
        }
    }

    char* const ptr = buffer.reserve(bytesToRead);
    (void) memcpy(ptr, bytes, static_cast<size_t>(bytesToRead));

    emit q->readyRead();
}

void QSerialPortPrivate::exceptionArrived(const QString &ex)
{
    setError(QSerialPortErrorInfo(QSerialPort::UnknownError, ex));
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    const qint64 orig = static_cast<int>(buffer.size());

    if (orig > 0) {
        return true;
    }

    for (int i = 0; i < msecs; i++) {
        if (orig < buffer.size()) {
            return true;
        } else {
            QGC::SLEEP::msleep(1);
        }
    }

    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    // if (writeBuffer.isEmpty() && (m_pendingBytesWritten <= 0)) {
    //     return false;
    // }

    return _writeDataOneShot(msecs);
}

bool QSerialPortPrivate::_writeDataOneShot(int msecs)
{
    Q_Q(QSerialPort);

    m_pendingBytesWritten = -1;

    while (!writeBuffer.isEmpty()) {
        m_pendingBytesWritten = _writeToPort(writeBuffer.readPointer(), writeBuffer.nextDataBlockSize(), msecs, false);

        if (m_pendingBytesWritten <= 0) {
            setError(QSerialPortErrorInfo(QSerialPort::WriteError));
            return false;
        }

        writeBuffer.free(m_pendingBytesWritten);

        emit q->bytesWritten(m_pendingBytesWritten);
    }

    return (m_pendingBytesWritten >= 0);
}

qint64 QSerialPortPrivate::_writeToPort(const char *data, qint64 maxSize, int timeout, bool async)
{
    return AndroidSerial::write(m_deviceId, data, maxSize, timeout, async);
}

qint64 QSerialPortPrivate::writeData(const char *data, qint64 maxSize)
{
    (void) writeBuffer.append(data, maxSize);
    return _writeDataOneShot(0);
}

bool QSerialPortPrivate::flush()
{
    return _writeDataOneShot(0);
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

    return AndroidSerial::purgeBuffers(m_deviceId, input, output);
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    return AndroidSerial::getControlLines(m_deviceId);
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    return AndroidSerial::setDataTerminalReady(m_deviceId, set);
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    return AndroidSerial::setRequestToSend(m_deviceId, set);
}

bool QSerialPortPrivate::_setParameters(int baudRate, int dataBits, int stopBits, int parity)
{
    const bool result = AndroidSerial::setParameters(m_deviceId, baudRate, dataBits, stopBits, parity);
    if (result) {
        inputBaudRate = outputBaudRate = baudRate;
        m_dataBits = dataBits;
        m_stopBits = stopBits;
        m_parity = parity;
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
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Invalid baud rate value")));
        return false;
    }

    if (directions != QSerialPort::AllDirections) {
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Custom baud rate direction is unsupported")));
        return false;
    }

    const qint32 standardBaudRate = QSerialPortPrivate::_settingFromBaudRate(baudRate);
    if (standardBaudRate <= 0) {
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Invalid Baud Rate")));
        return false;
    }

    return _setParameters(baudRate, m_dataBits, m_stopBits, m_parity);
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

    return _setParameters(inputBaudRate, numBits, m_stopBits, m_parity);
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

    return _setParameters(inputBaudRate, m_dataBits, m_stopBits, par);
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

    return _setParameters(inputBaudRate, m_dataBits, stop, m_parity);
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

    return AndroidSerial::setFlowControl(m_deviceId, control);
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    return AndroidSerial::setBreak(m_deviceId, set);
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
