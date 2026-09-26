#include "ReplayGPSTransport.h"

#include <algorithm>
#include <thread>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ReplayGPSTransportLog, "GPS.Transport.ReplayGPSTransport")

namespace {
/// A start bit, eight data bits and a stop bit.
constexpr qint64 BITS_PER_BYTE = 10;
constexpr qint64 MICROSECONDS_PER_SECOND = 1'000'000;
/// Waits sleep in slices this long so a stop request ends them promptly.
constexpr std::chrono::milliseconds SLEEP_SLICE{20};
}  // namespace

ReplayGPSTransport::ReplayGPSTransport(QString path, std::stop_token stopToken, unsigned baudrate, int chunkBytes)
    : GPSTransport(std::move(stopToken))
    , _file(std::move(path))
    , _baudrate(baudrate)
    , _chunkBytes(std::max(chunkBytes, 1))
{
    qCDebug(ReplayGPSTransportLog) << this;
}

ReplayGPSTransport::~ReplayGPSTransport()
{
    qCDebug(ReplayGPSTransportLog) << this;
}

GPSOpenResult ReplayGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    if (_file.isOpen()) {
        return {GPSOpenStatus::Opened};
    }
    if (!_file.open(QIODevice::ReadOnly)) {
        return {GPSOpenStatus::Error, _file.errorString()};
    }
    _size = _file.size();
    _delivered = 0;
    _failed = false;
    _finished = _size == 0;
    _clock.start();
    qCDebug(ReplayGPSTransportLog) << "Replaying" << _size << "bytes from" << _file.fileName() << "at baud"
                                   << _baudrate;
    return {GPSOpenStatus::Opened};
}

bool ReplayGPSTransport::fatalError() const
{
    return !_file.isOpen() || _failed || _finished;
}

GPSReadResult ReplayGPSTransport::read(uint8_t* buffer, int length, std::chrono::milliseconds timeout)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSReadStatus::InvalidData};
    }
    if (_failed) {
        return {GPSReadStatus::Error, 0, _file.errorString()};
    }
    if (!_file.isOpen()) {
        return {GPSReadStatus::Closed, 0, QStringLiteral("Receiver recording is not open")};
    }
    if (length == 0) {
        return {GPSReadStatus::Data};
    }
    const QDeadlineTimer deadline((std::max) (timeout, std::chrono::milliseconds::zero()), Qt::PreciseTimer);
    qint64 count = _available(length);
    while (count == 0) {
        // The end of the capture is an idle link: it times out rather than failing the read.
        _sleepUntil(_finished ? deadline : std::min(deadline, _nextByteDue()));
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        count = _available(length);
        if (count == 0 && deadline.hasExpired()) {
            return {GPSReadStatus::TimedOut};
        }
    }
    const qint64 taken = _file.read(reinterpret_cast<char*>(buffer), count);
    if (taken < 0) {
        _failed = true;
        qCWarning(ReplayGPSTransportLog) << "Cannot read receiver recording" << _file.fileName() << _file.errorString();
        return {GPSReadStatus::Error, 0, _file.errorString()};
    }
    _delivered += taken;
    _finished = taken == 0 || _delivered >= _size;
    if (taken == 0) {
        return {GPSReadStatus::TimedOut};
    }
    return {GPSReadStatus::Data, static_cast<int>(taken)};
}

bool ReplayGPSTransport::setBaudrate(unsigned)
{
    return !isCancelled();
}

GPSWriteResult ReplayGPSTransport::writeData(const uint8_t*, int length, QDeadlineTimer)
{
    return {GPSWriteStatus::Completed, length, length};
}

qint64 ReplayGPSTransport::_available(int length) const
{
    if (_finished) {
        return 0;
    }
    qint64 available = std::min({static_cast<qint64>(length), static_cast<qint64>(_chunkBytes), _size - _delivered});
    if (_baudrate > 0) {
        const qint64 elapsedUs = _clock.nsecsElapsed() / 1000;
        const qint64 due = elapsedUs * _baudrate / (BITS_PER_BYTE * MICROSECONDS_PER_SECOND);
        available = std::min(available, due - _delivered);
    }
    return std::max<qint64>(available, 0);
}

QDeadlineTimer ReplayGPSTransport::_nextByteDue() const
{
    if (_baudrate == 0) {
        return QDeadlineTimer(0);
    }
    const qint64 dueUs = ((_delivered + 1) * BITS_PER_BYTE * MICROSECONDS_PER_SECOND + _baudrate - 1) / _baudrate;
    return QDeadlineTimer(std::chrono::microseconds(dueUs - _clock.nsecsElapsed() / 1000), Qt::PreciseTimer);
}

void ReplayGPSTransport::_sleepUntil(QDeadlineTimer deadline) const
{
    while (!isCancelled() && !deadline.hasExpired()) {
        std::this_thread::sleep_for(
            std::min<std::chrono::nanoseconds>(deadline.remainingTimeAsDuration(), SLEEP_SLICE));
    }
}
