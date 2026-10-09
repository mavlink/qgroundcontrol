#include "GPSDeviceTransport.h"

#include <algorithm>
#include <limits>

#include <QtCore/QEventLoop>
#include <QtCore/QIODevice>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>
#ifndef QGC_NO_SERIAL_LINK
#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif
#endif

GPSReadResult GPSDeviceTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (readUnavailable()) {
        return readFailure();
    }
    if (buffer.empty()) {
        return fatalError() ? readFailure() : GPSReadResult{GPSReadStatus::Data};
    }
    if (!waitReadable(QDeadlineTimer((std::max) (timeout, std::chrono::milliseconds::zero()), Qt::PreciseTimer))) {
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        return fatalError() ? readFailure() : GPSReadResult{GPSReadStatus::TimedOut};
    }
    const qint64 count = take(buffer);
    if (count < 0) {
        return {GPSReadStatus::Error, 0, readFailure().detail};
    }
    return {GPSReadStatus::Data, static_cast<int>(count)};
}

bool GPSDeviceTransport::readUnavailable() const
{
    return !device();
}

bool GPSDeviceTransport::waitReadable(QDeadlineTimer deadline)
{
    QIODevice* const io = device();
    // bytesAvailable() only sees the device's buffer, so an immediate poll also services input not taken in yet.
    if (deadline.hasExpired() && io->bytesAvailable() == 0 && !fatalError()) {
        io->waitForReadyRead(0);
    }
    return io->bytesAvailable() > 0 || waitForDevice([io]() { return io->bytesAvailable() > 0; }, deadline);
}

qint64 GPSDeviceTransport::take(std::span<uint8_t> buffer)
{
    return device()->read(reinterpret_cast<char*>(buffer.data()), static_cast<qint64>(buffer.size()));
}

GPSReadResult GPSDeviceTransport::readFailure() const
{
    return {GPSReadStatus::Closed, 0, device() ? device()->errorString() : QString()};
}

GPSWriteResult GPSDeviceTransport::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    QIODevice* const io = device();
    if (fatalError() || !io || io->bytesToWrite() != 0) {
        return {GPSWriteStatus::Error, 0, 0, readFailure().detail};
    }
    qint64 drained = 0;
    const QMetaObject::Connection counter = QObject::connect(
        io, &QIODevice::bytesWritten, io, [&drained](qint64 count) { drained += count; }, Qt::DirectConnection);
    const int length = static_cast<int>(bytes.size());
    int accepted = 0;
    GPSWriteStatus status = GPSWriteStatus::Completed;
    while (accepted < length || io->bytesToWrite() > 0) {
        if (isCancelled() || fatalError() || deadline.hasExpired()) {
            break;
        }
        if (accepted < length && io->bytesToWrite() < WRITE_BUFFER_BYTES) {
            const qint64 count =
                io->write(bytes.data() + accepted,
                          (std::min) (qint64(length - accepted), WRITE_BUFFER_BYTES - io->bytesToWrite()));
            if (count < 0) {
                status = GPSWriteStatus::Error;
                break;
            }
            accepted += static_cast<int>(count);
        }
        if (io->bytesToWrite() > 0) {
            waitWritten(deadline);
        }
    }
    QObject::disconnect(counter);
    const int written = static_cast<int>(std::clamp(confirmedWritten(accepted, drained), qint64(0), qint64(accepted)));
    // A write confirmed in full succeeded, even if the deadline, a stop or a link failure followed it.
    if (written < length) {
        if (isCancelled()) {
            status = GPSWriteStatus::Cancelled;
        } else if (fatalError()) {
            status = GPSWriteStatus::Error;
        } else if (deadline.hasExpired()) {
            status = GPSWriteStatus::TimedOut;
        }
    }
    const GPSWriteResult result{status, accepted, written,
                                status == GPSWriteStatus::Error ? readFailure().detail : QString()};
    if (status != GPSWriteStatus::Completed && accepted > 0) {
        retire();
    }
    return result;
}

void GPSDeviceTransport::waitWritten(QDeadlineTimer& deadline)
{
    QIODevice* const io = device();
    (void) waitForDevice([io]() { return io->bytesToWrite() == 0; }, deadline);
}

qint64 GPSDeviceTransport::confirmedWritten(int accepted, qint64 drained)
{
    Q_UNUSED(accepted)
    return drained;
}

void GPSDeviceTransport::retire()
{
    device()->close();
}

bool GPSDeviceTransport::waitUntil(const std::function<bool()>& done, QDeadlineTimer deadline)
{
    if (!isCancelled() && !done() && !deadline.hasExpired()) {
        QEventLoop loop;
        QTimer timeout;
        const auto check = [&]() {
            if (isCancelled() || deadline.hasExpired() || done()) {
                loop.quit();
            }
        };
        if (QIODevice* const io = device()) {
            (void) QObject::connect(io, &QIODevice::readyRead, &loop, check);
            (void) QObject::connect(io, &QIODevice::bytesWritten, &loop, check);
            if (auto* const socket = qobject_cast<QAbstractSocket*>(io)) {
                (void) QObject::connect(socket, &QAbstractSocket::errorOccurred, &loop, check);
                (void) QObject::connect(socket, &QAbstractSocket::stateChanged, &loop, check);
            }
#ifndef QGC_NO_SERIAL_LINK
            if (auto* const port = qobject_cast<QSerialPort*>(io)) {
                (void) QObject::connect(port, &QSerialPort::errorOccurred, &loop, check);
            }
#endif
        }
        (void) QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeout.setSingleShot(true);
        timeout.setTimerType(Qt::PreciseTimer);
        if (!deadline.isForever()) {
            timeout.start(
                static_cast<int>((std::min) (deadline.remainingTime(), qint64((std::numeric_limits<int>::max)()))));
        }
        // cancel() runs this on the cancelling thread, so it only posts. Declared after the loop so it is destroyed
        // first: the destructor waits for a running invocation, and the loop's destructor discards an unhandled post.
        const GPSCancelCallback wake(
            cancelToken(), [&loop]() { QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection); });
        loop.exec();
    }
    return !isCancelled() && done();
}

bool GPSDeviceTransport::waitForDevice(const std::function<bool()>& ready, QDeadlineTimer deadline)
{
    if (!device() || fatalError()) {
        return false;
    }
    (void) waitUntil([&]() { return fatalError() || ready(); }, deadline);
    return !isCancelled() && !fatalError() && ready();
}
