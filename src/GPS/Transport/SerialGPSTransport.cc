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

GPSTransport::OpenResult SerialGPSTransport::open()
{
    if (isCancelled()) {
        return {OpenStatus::Cancelled};
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
            return {OpenStatus::Cancelled};
        }
        // Device can take 10-20s to become accessible after startup.
        if (_serial->error() != QSerialPort::PermissionError || openDeadline.hasExpired()) {
            qCWarning(SerialGPSTransportLog)
                << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return {openDeadline.hasExpired() ? OpenStatus::TimedOut : OpenStatus::Error, _serial->errorString()};
        }
        qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
        const QDeadlineTimer retryDeadline((std::min) (openDeadline.remainingTime(), qint64(kOpenRetryMs)));
        while (!retryDeadline.hasExpired() && !isCancelled()) {
            QThread::msleep(
                static_cast<unsigned long>((std::min) (retryDeadline.remainingTime(), qint64(kCancellationPollMs))));
        }
        if (isCancelled()) {
            return {OpenStatus::Cancelled};
        }
    }
    _serial->clearError();

    const bool configured = _serial->setBaudRate(QSerialPort::Baud9600) && _serial->setDataBits(QSerialPort::Data8) &&
                            _serial->setParity(QSerialPort::NoParity) && _serial->setStopBits(QSerialPort::OneStop) &&
                            _serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!configured || isCancelled()) {
        const OpenResult result{isCancelled() ? OpenStatus::Cancelled : OpenStatus::Error, _serial->errorString()};
        _serial->close();
        return result;
    }
    return {OpenStatus::Opened};
}

bool SerialGPSTransport::fatalError() const
{
    return _inputBudgetExhausted() || !_serial || !_serial->isOpen() ||
           ((_serial->error() != QSerialPort::NoError) && (_serial->error() != QSerialPort::TimeoutError));
}

GPSTransport::ReadResult SerialGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled()) {
        return {ReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {ReadStatus::InvalidData};
    }
    if (fatalError()) {
        return {_inputBudgetExhausted() ? ReadStatus::Overflow : ReadStatus::Error, 0, _errorDetail()};
    }
    if (length == 0) {
        return {ReadStatus::Data};
    }
    const QDeadlineTimer deadline((std::max) (timeoutMs, 0));
    while (_serial->bytesAvailable() == 0) {
        _serial->waitForReadyRead(static_cast<int>((std::min) (deadline.remainingTime(), qint64(kCancellationPollMs))));
        if (isCancelled()) {
            return {ReadStatus::Cancelled};
        }
        if (fatalError()) {
            return {_inputBudgetExhausted() ? ReadStatus::Overflow : ReadStatus::Error, 0, _errorDetail()};
        }
        if (_serial->bytesAvailable() == 0 && deadline.hasExpired()) {
            return {ReadStatus::TimedOut};
        }
    }
    const qint64 count = _serial->read(reinterpret_cast<char*>(buffer), length);
    return {count < 0 ? ReadStatus::Error : ReadStatus::Data, static_cast<int>((std::max) (count, qint64(0))),
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
GPSTransport::WriteResult SerialGPSTransport::write(const uint8_t* buffer, int length)
{
    if (isCancelled()) {
        return {WriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {WriteStatus::InvalidData};
    }
    if (fatalError()) {
        return {WriteStatus::Error, 0, 0, 0, _errorDetail()};
    }
    if (length == 0) {
        return {WriteStatus::Completed};
    }
    // The legacy Android backend writes synchronously with its own timeout and cannot be interrupted.
    const qint64 count = _serial->write(reinterpret_cast<const char*>(buffer), length);
    const int written = static_cast<int>(std::clamp(count, qint64(0), qint64(length)));
    WriteStatus status = WriteStatus::Error;
    if (isCancelled()) {
        status = WriteStatus::Cancelled;
    } else if (count == length && !fatalError()) {
        status = WriteStatus::Completed;
    }
    // A failed legacy write can have delivered bytes without reporting their count.
    const WriteResult result{status, length, written, length - written,
                             status == WriteStatus::Error ? _errorDetail() : QString()};
    if (status != WriteStatus::Completed) {
        _serial->close();
    }
    return result;
}
#endif

GPSTransport::WriteResult SerialGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {WriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {WriteStatus::InvalidData};
    }
    if (fatalError() || _serial->bytesToWrite() != 0) {
        return {WriteStatus::Error, 0, 0, 0, _errorDetail()};
    }
    if (length == 0) {
        return {WriteStatus::Completed};
    }
#ifdef Q_OS_ANDROID
    Q_UNUSED(deadline);
    return {WriteStatus::Unsupported, 0, 0, 0, QStringLiteral("Android serial does not support bounded writes")};
#else
    int accepted = 0;
    const qint64 previousAccepted = _acceptedTotal;
    WriteStatus status = WriteStatus::Completed;
    while (accepted < length || _serial->bytesToWrite() > 0) {
        if (isCancelled() || fatalError() || deadline.hasExpired()) {
            status = isCancelled() ? WriteStatus::Cancelled : fatalError() ? WriteStatus::Error : WriteStatus::TimedOut;
            break;
        }
        if (accepted < length) {
            const qint64 count =
                _serial->write(reinterpret_cast<const char*>(buffer) + accepted,
                               qMin(qint64(length - accepted), kWriteBufferBytes - _serial->bytesToWrite()));
            if (count < 0) {
                status = WriteStatus::Error;
                break;
            }
            accepted += static_cast<int>(count);
            _acceptedTotal += count;
        }
        if (_serial->bytesToWrite() > 0) {
            _serial->waitForBytesWritten(
                deadline.isForever()
                    ? kCancellationPollMs
                    : static_cast<int>((std::min) (deadline.remainingTime(), qint64(kCancellationPollMs))));
        }
    }
    if (isCancelled()) {
        status = WriteStatus::Cancelled;
    } else if (fatalError()) {
        status = WriteStatus::Error;
    } else if (deadline.hasExpired()) {
        status = WriteStatus::TimedOut;
    }
    // Qt can empty its write buffer before emitting bytesWritten; its later signal belongs to that earlier write.
    const qint64 confirmed =
        !fatalError() ? (std::max) (_writtenTotal, _acceptedTotal - _serial->bytesToWrite()) : _writtenTotal;
    const int written = static_cast<int>(std::clamp(confirmed - previousAccepted, qint64(0), qint64(accepted)));
    const WriteResult result{status, accepted, written, accepted - written,
                             status == WriteStatus::Error ? _errorDetail() : QString()};
    if (status != WriteStatus::Completed && accepted > 0) {
        // Closing discards Qt's remaining output; it must not drain after this operation returns.
        _serial->close();
    }
    return result;
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
