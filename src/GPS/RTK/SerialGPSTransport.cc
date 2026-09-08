#include "SerialGPSTransport.h"

#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#include <QtCore/QThread>

#include <utility>

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.SerialGPSTransport")

SerialGPSTransport::SerialGPSTransport(QString device, const std::atomic_bool &requestStop)
    : _device(std::move(device))
    , _requestStop(requestStop)
{
}

SerialGPSTransport::~SerialGPSTransport() = default;

bool SerialGPSTransport::open()
{
    _serial = std::make_unique<QSerialPort>();
    _serial->setPortName(_device);
    if (!_serial->open(QIODevice::ReadWrite)) {
        // Device can take 10-20s to become accessible after startup.
        uint32_t retries = 60;
        while ((retries-- > 0) && !_requestStop && (_serial->error() == QSerialPort::PermissionError)) {
            qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
            QThread::msleep(500);
            if (_serial->open(QIODevice::ReadWrite)) {
                _serial->clearError();
                break;
            }
        }

        if (_serial->error() != QSerialPort::NoError) {
            qCWarning(SerialGPSTransportLog) << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return false;
        }
    }

    (void) _serial->setBaudRate(QSerialPort::Baud9600);
    (void) _serial->setDataBits(QSerialPort::Data8);
    (void) _serial->setParity(QSerialPort::NoParity);
    (void) _serial->setStopBits(QSerialPort::OneStop);
    (void) _serial->setFlowControl(QSerialPort::NoFlowControl);

    return true;
}

bool SerialGPSTransport::fatalError() const
{
    return _serial
        && (_serial->error() != QSerialPort::NoError)
        && (_serial->error() != QSerialPort::TimeoutError);
}

int SerialGPSTransport::read(uint8_t *buffer, int length, int timeoutMs)
{
    if (_requestStop) {
        return -1; // abort an in-flight configure/receive so disconnect joins promptly
    }
    if (_serial->bytesAvailable() == 0) {
        if (!_serial->waitForReadyRead(timeoutMs)) {
            return 0;
        }
    }
    return _serial->read(reinterpret_cast<char *>(buffer), length);
}

int SerialGPSTransport::write(const uint8_t *buffer, int length)
{
    if (_requestStop) {
        return -1;
    }
    int written = 0;
    while (written < length) {
        const qint64 n = _serial->write(reinterpret_cast<const char *>(buffer) + written, length - written);
        if (n < 0) {
            return -1;
        }
        written += static_cast<int>(n);
        if (!_serial->waitForBytesWritten(kWriteTimeoutMs)) {
            return -1;
        }
    }
    return written;
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    return _serial->setBaudRate(baudrate);
}
