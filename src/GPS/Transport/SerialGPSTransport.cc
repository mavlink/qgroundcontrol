#include "SerialGPSTransport.h"

#include "QGCLoggingCategory.h"

#ifndef Q_OS_ANDROID
#include "GPSStreamWrite_p.h"
#endif

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>

#include <algorithm>
#include <utility>

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.Transport.SerialGPSTransport")

SerialGPSTransport::SerialGPSTransport(QString device, const std::atomic_bool& requestStop)
    : GPSTransport(requestStop), _device(std::move(device))
{
    qCDebug(SerialGPSTransportLog) << this;
}

SerialGPSTransport::~SerialGPSTransport()
{
    qCDebug(SerialGPSTransportLog) << this;
}

GPSOpenResult SerialGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    _serial = std::make_unique<QSerialPort>();
    _inputOverflow = false;
    _serial->setReadBufferSize(kReadBufferBytes);
    QObject::connect(
        _serial.get(), &QSerialPort::readyRead, _serial.get(),
        [this]() {
            // With no serial flow control, exhausting our budget makes stream continuity unverifiable.
            // Retire the attempt rather than decode bytes across a possible overrun.
            _inputOverflow = _inputOverflow || _serial->bytesAvailable() >= kReadBufferBytes;
        },
        Qt::DirectConnection);
#ifndef Q_OS_ANDROID
    _acceptedTotal = 0;
    _writtenTotal = 0;
    QObject::connect(
        _serial.get(), &QSerialPort::bytesWritten, _serial.get(), [this](qint64 count) { _writtenTotal += count; },
        Qt::DirectConnection);
#endif
    _serial->setPortName(_device);
    const QDeadlineTimer openDeadline(kOpenTimeoutMs);
    while (!_serial->open(QIODevice::ReadWrite)) {
        if (isCancelled()) {
            return {GPSOpenStatus::Cancelled};
        }
        // Device can take 10-20s to become accessible after startup.
        if (_serial->error() != QSerialPort::PermissionError || openDeadline.hasExpired()) {
            qCWarning(SerialGPSTransportLog)
                << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return {openDeadline.hasExpired() ? GPSOpenStatus::TimedOut : GPSOpenStatus::Error, _serial->errorString()};
        }
        qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
        const QDeadlineTimer retryDeadline((std::min) (openDeadline.remainingTime(), qint64(kOpenRetryMs)));
        while (!retryDeadline.hasExpired() && !isCancelled()) {
            QThread::msleep(
                static_cast<unsigned long>((std::min) (retryDeadline.remainingTime(), qint64(kCancellationPollMs))));
        }
        if (isCancelled()) {
            return {GPSOpenStatus::Cancelled};
        }
    }
    _serial->clearError();

    const bool configured = _serial->setBaudRate(QSerialPort::Baud9600) && _serial->setDataBits(QSerialPort::Data8) &&
                            _serial->setParity(QSerialPort::NoParity) && _serial->setStopBits(QSerialPort::OneStop) &&
                            _serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!configured || isCancelled()) {
        const GPSOpenResult result{isCancelled() ? GPSOpenStatus::Cancelled : GPSOpenStatus::Error,
                                   _serial->errorString()};
        _serial->close();
        return result;
    }
    return {GPSOpenStatus::Opened};
}

bool SerialGPSTransport::fatalError() const
{
    return _inputBudgetExhausted() || !_serial || !_serial->isOpen() ||
           ((_serial->error() != QSerialPort::NoError) && (_serial->error() != QSerialPort::TimeoutError));
}

GPSReadResult SerialGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSReadStatus::InvalidData};
    }
    if (fatalError()) {
        return {_inputBudgetExhausted() ? GPSReadStatus::Overflow : GPSReadStatus::Error, 0, _errorDetail()};
    }
    if (length == 0) {
        return {GPSReadStatus::Data};
    }
    const QDeadlineTimer deadline((std::max) (timeoutMs, 0));
    while (_serial->bytesAvailable() == 0) {
        _serial->waitForReadyRead(static_cast<int>((std::min) (deadline.remainingTime(), qint64(kCancellationPollMs))));
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (fatalError()) {
            return {_inputBudgetExhausted() ? GPSReadStatus::Overflow : GPSReadStatus::Error, 0, _errorDetail()};
        }
        if (_serial->bytesAvailable() == 0 && deadline.hasExpired()) {
            return {GPSReadStatus::TimedOut};
        }
    }
    const qint64 count = _serial->read(reinterpret_cast<char*>(buffer), length);
    return {count < 0 ? GPSReadStatus::Error : GPSReadStatus::Data, static_cast<int>((std::max) (count, qint64(0))),
            count < 0 ? _errorDetail() : QString()};
}

std::chrono::milliseconds SerialGPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(kWriteTimeoutMs);
}

bool SerialGPSTransport::_inputBudgetExhausted() const
{
    return _inputOverflow || (_serial && _serial->bytesAvailable() >= kReadBufferBytes);
}

QString SerialGPSTransport::_errorDetail() const
{
    return _inputBudgetExhausted()
               ? QStringLiteral("Serial GPS input buffer exhausted; reconnect required to restore stream continuity")
           : _serial ? _serial->errorString()
                     : QStringLiteral("Serial GPS connection is closed");
}

#ifdef Q_OS_ANDROID
GPSWriteResult SerialGPSTransport::writeConfiguration(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSWriteStatus::InvalidData};
    }
    if (deadline.hasExpired()) {
        return {GPSWriteStatus::TimedOut};
    }
    if (fatalError()) {
        return {GPSWriteStatus::Error, 0, 0, _errorDetail()};
    }
    if (length == 0) {
        return {GPSWriteStatus::Completed};
    }
    // The synchronous Android backend can honor the caller's deadline only before submission.
    const qint64 count = _serial->write(reinterpret_cast<const char*>(buffer), length);
    const int written = static_cast<int>(std::clamp(count, qint64(0), qint64(length)));
    GPSWriteStatus status = GPSWriteStatus::Error;
    if (isCancelled()) {
        status = GPSWriteStatus::Cancelled;
    } else if (count == length && !fatalError()) {
        status = GPSWriteStatus::Completed;
    }
    // A failed backend write can have delivered bytes without reporting their count.
    const GPSWriteResult result{status, length, written, status == GPSWriteStatus::Error ? _errorDetail() : QString()};
    if (status != GPSWriteStatus::Completed) {
        _serial->close();
    }
    return result;
}
#endif

GPSWriteResult SerialGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
#ifdef Q_OS_ANDROID
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSWriteStatus::InvalidData};
    }
    if (fatalError() || _serial->bytesToWrite() != 0) {
        return {GPSWriteStatus::Error, 0, 0, _errorDetail()};
    }
    if (length == 0) {
        return {GPSWriteStatus::Completed};
    }
    Q_UNUSED(deadline);
    return {GPSWriteStatus::Unsupported, 0, 0, QStringLiteral("Android serial does not support bounded writes")};
#else
    const qint64 previousAccepted = _acceptedTotal;
    return GPSStreamWrite::writeBounded(
        *this, _serial.get(), buffer, length, deadline, kWriteBufferBytes,
        [this](QDeadlineTimer remaining) {
            _serial->waitForBytesWritten(
                remaining.isForever()
                    ? kCancellationPollMs
                    : static_cast<int>((std::min) (remaining.remainingTime(), qint64(kCancellationPollMs))));
        },
        [this, previousAccepted](int accepted) {
            _acceptedTotal += accepted;
            // Late bytesWritten signals belong to earlier connection-wide submissions.
            const qint64 confirmed =
                !fatalError() ? (std::max) (_writtenTotal, _acceptedTotal - _serial->bytesToWrite()) : _writtenTotal;
            return confirmed - previousAccepted;
        },
        [this]() { return _errorDetail(); }, [this]() { _serial->close(); });
#endif
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && _serial->setBaudRate(baudrate);
}

std::chrono::milliseconds SerialGPSTransport::correctionWriteTimeout(int length) const
{
    return serialCorrectionWriteTimeout(length, _serial ? _serial->baudRate() : 0);
}
