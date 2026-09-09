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
    _acceptedTotal = 0;
    _writtenTotal = 0;
    QObject::connect(
        _serial.get(), &QSerialPort::bytesWritten, _serial.get(), [this](qint64 count) { _writtenTotal += count; },
        Qt::DirectConnection);
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

int SerialGPSTransport::write(const uint8_t* buffer, int length)
{
    const auto result = writeBounded(buffer, length, QDeadlineTimer(kWriteTimeoutMs));
    return result.status == WriteStatus::Completed ? result.writtenBytes : -1;
}

GPSTransport::WriteResult SerialGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {WriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {WriteStatus::InvalidData};
    }
    if (fatalError() || _serial->bytesToWrite() != 0) {
        return {WriteStatus::Error};
    }
    if (length == 0) {
        return {WriteStatus::Completed};
    }
    int accepted = 0;
    const qint64 previousAccepted = _acceptedTotal;
    WriteStatus status = WriteStatus::Completed;
    while (accepted < length || _serial->bytesToWrite() > 0) {
        if (isCancelled() || fatalError() || deadline.hasExpired()) {
            status = isCancelled() ? WriteStatus::Cancelled : fatalError() ? WriteStatus::Error : WriteStatus::TimedOut;
            break;
        }
        if (accepted < length) {
            const qint64 count = _serial->write(reinterpret_cast<const char*>(buffer) + accepted, length - accepted);
            if (count < 0) {
                status = WriteStatus::Error;
                break;
            }
            accepted += static_cast<int>(count);
            _acceptedTotal += count;
        }
        if (_serial->bytesToWrite() > 0) {
            _serial->waitForBytesWritten(
                static_cast<int>((std::min) (deadline.remainingTime(), qint64(kCancellationPollMs))));
        }
    }
    if (isCancelled()) {
        status = WriteStatus::Cancelled;
    } else if (fatalError()) {
        status = WriteStatus::Error;
    }
    // Qt can empty its write buffer before emitting bytesWritten; its later signal belongs to that earlier write.
    const qint64 confirmed =
        !fatalError() ? (std::max) (_writtenTotal, _acceptedTotal - _serial->bytesToWrite()) : _writtenTotal;
    const int written = static_cast<int>(std::clamp(confirmed - previousAccepted, qint64(0), qint64(accepted)));
    return {status, accepted, written, accepted - written};
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && _serial->setBaudRate(baudrate);
}

std::chrono::milliseconds SerialGPSTransport::correctionWriteTimeout(int length) const
{
    return serialCorrectionWriteTimeout(length, _serial ? _serial->baudRate() : 0);
}
