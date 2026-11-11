#include "ReplayGPSTransport.h"

#include <algorithm>
#include <cstring>
#include <thread>
#include <utility>

#include <QtCore/QList>

#include "GPSRecordingTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ReplayGPSTransportLog, "Test.GPS.Transport.ReplayGPSTransport")

namespace GPSTest {

namespace {
/// A start bit, eight data bits and a stop bit.
constexpr qint64 BITS_PER_BYTE = 10;
constexpr qint64 MICROSECONDS_PER_SECOND = 1'000'000;
/// Waits sleep in slices this long so a stop request ends them promptly.
constexpr std::chrono::milliseconds SLEEP_SLICE{20};

/// The bytes of a recording line that is a received record; empty for every other line.
QByteArray receivedBytes(const QByteArray& line)
{
    const QList<QByteArray> fields = line.trimmed().split(' ');
    return fields.size() == 3 && fields[1] == GPSRecordingTransport::RECEIVED ? QByteArray::fromHex(fields[2])
                                                                              : QByteArray();
}
}  // namespace

ReplayGPSTransport::ReplayGPSTransport(QString path, GPSCancelToken cancelToken, unsigned baudRate, int chunkBytes)
    : GPSTransport(std::move(cancelToken))
    , _file(std::move(path))
    , _baudRate(baudRate)
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
    // A recording starts with its header line, whichever line ending it has; anything else is raw receiver output.
    const QByteArray start = _file.peek(sizeof(GPSRecordingTransport::FILE_HEADER) + 1);
    const qsizetype lineEnd = start.indexOf('\n');
    _recording = lineEnd >= 0 && start.first(lineEnd).trimmed() == GPSRecordingTransport::FILE_HEADER;
    if (_recording) {
        (void) _file.skip(lineEnd + 1);
    }
    _pending.clear();
    _delivered = 0;
    _failed = false;
    _finished = false;
    _load();
    _clock.start();
    qCDebug(ReplayGPSTransportLog) << "Replaying" << (_recording ? "recording" : "raw capture") << _file.fileName()
                                   << "at baud" << _baudRate;
    return {GPSOpenStatus::Opened};
}

bool ReplayGPSTransport::fatalError() const
{
    return !_file.isOpen() || _failed || _finished;
}

GPSReadResult ReplayGPSTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (_failed) {
        return {GPSReadStatus::Error, 0, _file.errorString()};
    }
    if (!_file.isOpen()) {
        return {GPSReadStatus::Closed, 0, QStringLiteral("Receiver recording is not open")};
    }
    if (buffer.empty()) {
        return {GPSReadStatus::Data};
    }
    const auto length = static_cast<qint64>(buffer.size());
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
    std::memcpy(buffer.data(), _pending.constData(), static_cast<size_t>(count));
    _pending.remove(0, count);
    _delivered += count;
    // Loaded now, so that finished() is true as soon as the last byte was delivered.
    _load();
    return {GPSReadStatus::Data, static_cast<int>(count)};
}

bool ReplayGPSTransport::setBaudRate(unsigned)
{
    return !isCancelled();
}

GPSWriteResult ReplayGPSTransport::writeData(QByteArrayView bytes, QDeadlineTimer)
{
    const auto length = static_cast<int>(bytes.size());
    return {GPSWriteStatus::Completed, length, length};
}

void ReplayGPSTransport::_load()
{
    while (_pending.isEmpty() && !_file.atEnd()) {
        _pending = _recording ? receivedBytes(_file.readLine()) : _file.read(_chunkBytes);
        if (_file.error() != QFileDevice::NoError) {
            qCWarning(ReplayGPSTransportLog)
                << "Cannot read receiver recording" << _file.fileName() << _file.errorString();
            _pending.clear();
            _failed = true;
            return;
        }
    }
    _finished = _pending.isEmpty();
}

qint64 ReplayGPSTransport::_available(qint64 length) const
{
    qint64 available = std::min({length, static_cast<qint64>(_chunkBytes), static_cast<qint64>(_pending.size())});
    if (_baudRate > 0) {
        const qint64 elapsedUs = _clock.nsecsElapsed() / 1000;
        const qint64 due = elapsedUs * _baudRate / (BITS_PER_BYTE * MICROSECONDS_PER_SECOND);
        available = std::min(available, due - _delivered);
    }
    return std::max<qint64>(available, 0);
}

QDeadlineTimer ReplayGPSTransport::_nextByteDue() const
{
    if (_baudRate == 0) {
        return QDeadlineTimer(0);
    }
    const qint64 dueUs = ((_delivered + 1) * BITS_PER_BYTE * MICROSECONDS_PER_SECOND + _baudRate - 1) / _baudRate;
    return QDeadlineTimer(std::chrono::microseconds(dueUs - _clock.nsecsElapsed() / 1000), Qt::PreciseTimer);
}

void ReplayGPSTransport::_sleepUntil(QDeadlineTimer deadline) const
{
    while (!isCancelled() && !deadline.hasExpired()) {
        std::this_thread::sleep_for(
            std::min<std::chrono::nanoseconds>(deadline.remainingTimeAsDuration(), SLEEP_SLICE));
    }
}

}  // namespace GPSTest
