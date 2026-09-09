#include "SerialGPSTransport.h"

#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>

#include <algorithm>
#include <utility>

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.Driver.SerialGPSTransport")

SerialGPSTransport::SerialGPSTransport(QString device, const std::atomic_bool &requestStop)
    : GPSTransport(requestStop)
    , _device(std::move(device))
{
    qCDebug(SerialGPSTransportLog) << this;
}

SerialGPSTransport::~SerialGPSTransport()
{
    qCDebug(SerialGPSTransportLog) << this;
}

bool SerialGPSTransport::open()
{
    if (isCancelled()) {
        return false;
    }
    _serial = std::make_unique<QSerialPort>();
    _serial->setPortName(_device);
    const QDeadlineTimer openDeadline(kOpenTimeoutMs);
    while (!_serial->open(QIODevice::ReadWrite)) {
        if (isCancelled()) {
            return false;
        }
        // Device can take 10-20s to become accessible after startup.
        if (_serial->error() != QSerialPort::PermissionError || openDeadline.hasExpired()) {
            qCWarning(SerialGPSTransportLog) << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return false;
        }
        qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
        const QDeadlineTimer retryDeadline((std::min) (openDeadline.remainingTime(), qint64(kOpenRetryMs)));
        while (!retryDeadline.hasExpired() && !isCancelled()) {
            QThread::msleep(
                static_cast<unsigned long>((std::min) (retryDeadline.remainingTime(), qint64(kCancellationPollMs))));
        }
        if (isCancelled()) {
            return false;
        }
    }
    _serial->clearError();

    (void) _serial->setBaudRate(QSerialPort::Baud9600);
    (void) _serial->setDataBits(QSerialPort::Data8);
    (void) _serial->setParity(QSerialPort::NoParity);
    (void) _serial->setStopBits(QSerialPort::OneStop);
    (void) _serial->setFlowControl(QSerialPort::NoFlowControl);

    return !isCancelled();
}

bool SerialGPSTransport::fatalError() const
{
    return !_serial || !_serial->isOpen() ||
           ((_serial->error() != QSerialPort::NoError) && (_serial->error() != QSerialPort::TimeoutError));
}

int SerialGPSTransport::read(uint8_t *buffer, int length, int timeoutMs)
{
    if (isCancelled() || fatalError() || !buffer || length < 0) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    const QDeadlineTimer deadline((std::max) (timeoutMs, 0));
    while (_serial->bytesAvailable() == 0) {
        _serial->waitForReadyRead(static_cast<int>((std::min) (deadline.remainingTime(), qint64(kCancellationPollMs))));
        if (isCancelled() || fatalError()) {
            return -1;
        }
        if (_serial->bytesAvailable() == 0 && deadline.hasExpired()) {
            return 0;
        }
    }
    return _serial->read(reinterpret_cast<char *>(buffer), length);
}

int SerialGPSTransport::write(const uint8_t *buffer, int length)
{
    if (isCancelled() || fatalError() || !buffer || length < 0) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    const QDeadlineTimer deadline(kWriteTimeoutMs);
    int written = 0;
    while (written < length || _serial->bytesToWrite() > 0) {
        if (isCancelled() || fatalError() || deadline.hasExpired()) {
            return -1;
        }
        if (written < length) {
            const qint64 n = _serial->write(reinterpret_cast<const char*>(buffer) + written, length - written);
            if (n < 0) {
                return -1;
            }
            written += static_cast<int>(n);
        }
        if (_serial->bytesToWrite() > 0) {
            _serial->waitForBytesWritten(
                static_cast<int>((std::min) (deadline.remainingTime(), qint64(kCancellationPollMs))));
        }
    }
    return isCancelled() || fatalError() ? -1 : written;
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && _serial->setBaudRate(baudrate);
}
