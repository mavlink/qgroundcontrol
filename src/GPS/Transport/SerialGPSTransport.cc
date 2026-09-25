#include "SerialGPSTransport.h"

#include "GPSStreamRead_p.h"
#include "GPSStreamWrite_p.h"
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

SerialGPSTransport::SerialGPSTransport(QString device, std::stop_token stopToken)
    : GPSTransport(std::move(stopToken))
    , _device(std::move(device))
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
    _acceptedTotal = 0;
    _writtenTotal = 0;
    QObject::connect(
        _serial.get(), &QSerialPort::bytesWritten, _serial.get(), [this](qint64 count) { _writtenTotal += count; },
        Qt::DirectConnection);
    _serial->setPortName(_device);
    const QDeadlineTimer openDeadline(kOpenTimeout);
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
        const QDeadlineTimer retryDeadline(
            (std::min) (std::chrono::milliseconds(openDeadline.remainingTime()), kOpenRetry));
        while (!retryDeadline.hasExpired() && !isCancelled()) {
            QThread::sleep((std::min) (std::chrono::milliseconds(retryDeadline.remainingTime()), kCancellationPoll));
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

GPSReadResult SerialGPSTransport::read(uint8_t* buffer, int length, std::chrono::milliseconds timeout)
{
    return GPSStreamRead::readBounded(
        *this, buffer, length, timeout, [this]() { return fatalError(); },
        [this](QDeadlineTimer deadline) {
            while (_serial->bytesAvailable() == 0) {
                _serial->waitForReadyRead(static_cast<int>(
                    (std::min) (std::chrono::milliseconds(deadline.remainingTime()), kCancellationPoll).count()));
                if (isCancelled() || fatalError() || (_serial->bytesAvailable() == 0 && deadline.hasExpired())) {
                    return false;
                }
            }
            return true;
        },
        [this](uint8_t* bytes, int count) { return _serial->read(reinterpret_cast<char*>(bytes), count); },
        [this]() {
            return GPSReadResult{_inputBudgetExhausted() ? GPSReadStatus::Overflow : GPSReadStatus::Error, 0,
                                 _errorDetail()};
        });
}

std::chrono::milliseconds SerialGPSTransport::configurationWriteTimeout() const
{
    return kWriteTimeout;
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

GPSWriteResult SerialGPSTransport::writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    const qint64 previousAccepted = _acceptedTotal;
    return GPSStreamWrite::writeBounded(
        *this, _serial.get(), buffer, length, deadline, kWriteBufferBytes,
        [this](QDeadlineTimer remaining) {
            _serial->waitForBytesWritten(static_cast<int>(
                (remaining.isForever()
                     ? kCancellationPoll
                     : (std::min) (std::chrono::milliseconds(remaining.remainingTime()), kCancellationPoll))
                    .count()));
        },
        [this, previousAccepted](int accepted) {
            _acceptedTotal += accepted;
            // Late bytesWritten signals belong to earlier connection-wide submissions.
            const qint64 confirmed =
                !fatalError() ? (std::max) (_writtenTotal, _acceptedTotal - _serial->bytesToWrite()) : _writtenTotal;
            return confirmed - previousAccepted;
        },
        [this]() { return _errorDetail(); }, [this]() { _serial->close(); });
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && _serial->setBaudRate(baudrate);
}
