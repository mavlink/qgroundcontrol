#include "SerialGPSTransport.h"

#include <algorithm>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QFileInfo>
#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.Transport.SerialGPSTransport")

namespace {

/// Why @a serial could not open @a device, as users read it after "Failed to open the receiver: ".
QString openFailure(const QSerialPort& serial, const QString& device)
{
    switch (serial.error()) {
        case QSerialPort::DeviceNotFoundError:
            //: %1 is a serial device, such as /dev/ttyACM0 or COM3
            return QCoreApplication::translate("SerialGPSTransport", "%1 was not found").arg(device);
        case QSerialPort::PermissionError:
#ifdef Q_OS_UNIX
            // A device held by another program reports a permission error too.
            if (const QFileInfo file(device); file.exists() && !(file.isReadable() && file.isWritable())) {
                //: %1 is a serial device, such as /dev/ttyACM0
                return QCoreApplication::translate("SerialGPSTransport", "permission denied for %1").arg(device);
            }
#endif
            //: %1 is a serial device, such as /dev/ttyACM0 or COM3
            return QCoreApplication::translate("SerialGPSTransport", "%1 is in use by another program").arg(device);
        default:
            return serial.errorString();
    }
}

}  // namespace

SerialGPSTransport::SerialGPSTransport(QString device, GPSCancelToken cancelToken,
                                       std::chrono::milliseconds openTimeout)
    : GPSDeviceTransport(std::move(cancelToken))
    , _device(std::move(device))
    , _openTimeout(openTimeout)
{}

SerialGPSTransport::~SerialGPSTransport() = default;

GPSOpenResult SerialGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    _serial = std::make_unique<QSerialPort>();
    _inputOverflow = false;
    _serial->setReadBufferSize(READ_BUFFER_BYTES);
    QObject::connect(
        _serial.get(), &QSerialPort::readyRead, _serial.get(),
        [this]() {
            // With no serial flow control, exhausting our budget makes stream continuity unverifiable.
            // Retire the attempt rather than decode bytes across a possible overrun.
            _inputOverflow = _inputOverflow || _serial->bytesAvailable() >= READ_BUFFER_BYTES;
        },
        Qt::DirectConnection);
    _acceptedTotal = 0;
    _writtenTotal = 0;
    QObject::connect(
        _serial.get(), &QSerialPort::bytesWritten, _serial.get(), [this](qint64 count) { _writtenTotal += count; },
        Qt::DirectConnection);
    _serial->setPortName(_device);
    const QDeadlineTimer openDeadline(_openTimeout);
    while (!_serial->open(QIODevice::ReadWrite)) {
        if (isCancelled()) {
            return {GPSOpenStatus::Cancelled};
        }
        // Device can take 10-20s to become accessible after startup.
        if (_serial->error() != QSerialPort::PermissionError || openDeadline.hasExpired()) {
            qCWarning(SerialGPSTransportLog)
                << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return {openDeadline.hasExpired() ? GPSOpenStatus::TimedOut : GPSOpenStatus::Error,
                    openFailure(*_serial, _device)};
        }
        qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
        const QDeadlineTimer retryDeadline(
            (std::min) (std::chrono::milliseconds(openDeadline.remainingTime()), OPEN_RETRY));
        (void) waitUntil([]() { return false; }, retryDeadline);
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

QIODevice* SerialGPSTransport::device() const
{
    return _serial.get();
}

bool SerialGPSTransport::readUnavailable() const
{
    return fatalError();
}

bool SerialGPSTransport::waitReadable(QDeadlineTimer deadline)
{
    // Unlike a socket, a serial port delivers no input once a stop or an overrun was seen.
    return GPSDeviceTransport::waitReadable(deadline) && !isCancelled() && !fatalError();
}

GPSReadResult SerialGPSTransport::readFailure() const
{
    return {_inputBudgetExhausted() ? GPSReadStatus::Overflow : GPSReadStatus::Error, 0, _errorDetail()};
}

bool SerialGPSTransport::_inputBudgetExhausted() const
{
    return _inputOverflow || (_serial && _serial->bytesAvailable() >= READ_BUFFER_BYTES);
}

QString SerialGPSTransport::_errorDetail() const
{
    return _inputBudgetExhausted()
               ? QStringLiteral("Serial GPS input buffer exhausted; reconnect required to restore stream continuity")
           : _serial ? _serial->errorString()
                     : QStringLiteral("Serial GPS connection is closed");
}

void SerialGPSTransport::waitWritten(QDeadlineTimer& deadline)
{
    _serial->waitForBytesWritten(static_cast<int>(
        (deadline.isForever() ? CANCELLATION_POLL
                              : (std::min) (std::chrono::milliseconds(deadline.remainingTime()), CANCELLATION_POLL))
            .count()));
}

qint64 SerialGPSTransport::confirmedWritten(int accepted, qint64 drained)
{
    Q_UNUSED(drained)
    const qint64 previousAccepted = _acceptedTotal;
    _acceptedTotal += accepted;
    // Late bytesWritten signals belong to earlier connection-wide submissions.
    const qint64 confirmed =
        !fatalError() ? (std::max) (_writtenTotal, _acceptedTotal - _serial->bytesToWrite()) : _writtenTotal;
    return confirmed - previousAccepted;
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    if (isCancelled() || fatalError()) {
        return false;
    }
    if (_serial->setBaudRate(static_cast<qint32>(baudrate))) {
        return true;
    }
    // A refused rate leaves the port open at its previous rate; only a lost device ends the link.
    if (_serial->isOpen() && _serial->error() != QSerialPort::ResourceError) {
        qCDebug(SerialGPSTransportLog) << "Link refused" << baudrate << "baud:" << _serial->errorString();
        _serial->clearError();
    }
    return false;
}
