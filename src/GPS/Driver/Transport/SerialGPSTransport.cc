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

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.Driver.Transport.SerialGPSTransport")

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
    _acceptedTotal = 0;
    _writtenTotal = 0;
    QObject::connect(
        _serial.get(), &QSerialPort::bytesWritten, _serial.get(), [this](qint64 count) { _writtenTotal += count; },
        Qt::DirectConnection);
    _serial->setPortName(_device);
    const QDeadlineTimer openDeadline(kOpenTimeoutMs);
    while (!_serial->open(QIODevice::ReadWrite)) {
        if (isCancelled()) {
            return {OpenStatus::Cancelled};
        }
        // Device can take 10-20s to become accessible after startup.
        if (_serial->error() != QSerialPort::PermissionError || openDeadline.hasExpired()) {
            qCWarning(SerialGPSTransportLog) << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
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

    (void) _serial->setBaudRate(QSerialPort::Baud9600);
    (void) _serial->setDataBits(QSerialPort::Data8);
    (void) _serial->setParity(QSerialPort::NoParity);
    (void) _serial->setStopBits(QSerialPort::OneStop);
    (void) _serial->setFlowControl(QSerialPort::NoFlowControl);

    return {isCancelled() ? OpenStatus::Cancelled : OpenStatus::Opened};
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
    if (_inputOverflow || (_serial && _serial->bytesAvailable() >= kReadBufferBytes)) {
        return true;
    }
#ifdef Q_OS_ANDROID
    // Android receives into a separate producer buffer before its worker-side QIODevice buffer.
    return _serial && _serial->inputOverflowed();
#else
    return false;
#endif
}

QString SerialGPSTransport::_errorDetail() const
{
    return _inputBudgetExhausted()
               ? QStringLiteral("Serial GPS input buffer exhausted; reconnect required to restore stream continuity")
           : _serial ? _serial->errorString()
                     : QStringLiteral("Serial GPS connection is closed");
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
        return {WriteStatus::Error, 0, 0, 0, _errorDetail()};
    }
    if (length == 0) {
        return {WriteStatus::Completed};
    }
#ifdef Q_OS_ANDROID
    const auto result = _serial->writeBounded(reinterpret_cast<const char*>(buffer), length, deadline,
                                              [this]() { return isCancelled() || fatalError(); });
    WriteStatus status = WriteStatus::Error;
    switch (result.status) {
        case AndroidSerialWrite::Status::Completed:
            status = WriteStatus::Completed;
            break;
        case AndroidSerialWrite::Status::TimedOut:
            status = WriteStatus::TimedOut;
            break;
        case AndroidSerialWrite::Status::Cancelled:
            status = WriteStatus::Cancelled;
            break;
        case AndroidSerialWrite::Status::InvalidData:
            status = WriteStatus::InvalidData;
            break;
        case AndroidSerialWrite::Status::Error:
            break;
    }
    if (!isCancelled() && fatalError()) {
        status = WriteStatus::Error;
    }
    return {status, static_cast<int>(result.writtenBytes + result.uncertainBytes),
            static_cast<int>(result.writtenBytes), static_cast<int>(result.uncertainBytes),
            status == WriteStatus::Error ? _errorDetail() : QString()};
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
    return {status, accepted, written, accepted - written, status == WriteStatus::Error ? _errorDetail() : QString()};
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
