#include "GPSPipe.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <QtCore/QCoreApplication>

namespace {

/// The longest single wait: a deadline that never comes stays inside what the standard library's clock can add.
constexpr std::chrono::microseconds PIPE_WAIT_SLICE = std::chrono::hours(1);

/// @a durationUs after @a nowUs on the session's clock, or never when that is past the clock's range.
uint64_t pipeDeadlineAfter(uint64_t nowUs, uint64_t durationUs)
{
    return durationUs > UINT64_MAX - nowUs ? UINT64_MAX : nowUs + durationUs;
}

/// The deadline of a wait of @a timeout that starts at @a nowUs; a timeout that is not positive is due at once.
uint64_t pipeDeadlineAfter(uint64_t nowUs, std::chrono::milliseconds timeout)
{
    if (timeout <= std::chrono::milliseconds::zero()) {
        return nowUs;
    }
    const auto milliseconds = static_cast<uint64_t>(timeout.count());
    return milliseconds > UINT64_MAX / 1000 ? UINT64_MAX : pipeDeadlineAfter(nowUs, milliseconds * 1000);
}

}  // namespace

GPSPipe::GPSPipe() = default;

GPSPipe::~GPSPipe() = default;

void GPSPipe::setLinkNotifier(std::function<void()> notifier)
{
    const std::lock_guard lock(_notifierMutex);
    _notifier = std::move(notifier);
}

void GPSPipe::_notifyLink()
{
    const std::lock_guard lock(_notifierMutex);
    if (_notifier) {
        _notifier();
    }
}

bool GPSPipe::openRequested() const
{
    const std::lock_guard lock(_mutex);
    return _openRequested;
}

bool GPSPipe::sessionClosed() const
{
    const std::lock_guard lock(_mutex);
    return _sessionClosed;
}

void GPSPipe::opened()
{
    {
        const std::lock_guard lock(_mutex);
        if (_link != Link::Connecting) {
            return;
        }
        _link = Link::Open;
    }
    _changed.notify_all();
}

void GPSPipe::close(const QString& detail)
{
    {
        const std::lock_guard lock(_mutex);
        if (_link == Link::Ended) {
            return;
        }
        _link = Link::Ended;
        _detail = detail;
    }
    _changed.notify_all();
}

void GPSPipe::feed(QByteArrayView bytes)
{
    {
        const std::lock_guard lock(_mutex);
        if (_link != Link::Open || _sessionClosed || _overflow || bytes.isEmpty()) {
            return;
        }
        if (_input.size() + bytes.size() > GPSTransport::READ_BUFFER_BYTES) {
            // What is buffered is not delivered either: the session ends, as after a serial overrun.
            _overflow = true;
            _input.clear();
        } else {
            _input.append(bytes);
        }
    }
    _changed.notify_all();
}

QByteArray GPSPipe::takeOutput()
{
    const std::lock_guard lock(_mutex);
    _unconfirmed += _output.size();
    return std::exchange(_output, {});
}

void GPSPipe::confirmWritten(qsizetype bytes)
{
    {
        const std::lock_guard lock(_mutex);
        const qsizetype confirmed = std::clamp<qsizetype>(bytes, 0, _unconfirmed);
        _unconfirmed -= confirmed;
        _confirmedTotal += confirmed;
    }
    _changed.notify_all();
}

bool GPSPipe::_fatal() const
{
    const std::lock_guard lock(_mutex);
    return !_sessionOpened || _failureReported || _sessionClosed;
}

GPSReadResult GPSPipe::_reportFailureLocked()
{
    _failureReported = true;
    return _failureLocked();
}

GPSReadResult GPSPipe::_failureLocked() const
{
    if (_overflow) {
        return {GPSReadStatus::Overflow, 0,
                QStringLiteral("Receiver input buffer exhausted; reconnect required to restore stream continuity")};
    }
    return {GPSReadStatus::Closed, 0, _detail};
}

template <typename Done>
bool GPSPipe::_waitUntil(std::unique_lock<std::mutex>& lock, const Session& session, uint64_t deadlineUs, Done done)
{
    while (!done()) {
        if (session.cancelToken.isCancelled()) {
            return false;
        }
        const uint64_t nowUs = session.clock.nowUs();
        if (nowUs >= deadlineUs) {
            return false;
        }
        const std::chrono::microseconds remaining(
            std::min<uint64_t>(deadlineUs - nowUs, static_cast<uint64_t>(PIPE_WAIT_SLICE.count())));
        if (session.clock.wait) {
            // A virtual clock's wait moves the time itself; what it lets the link do meanwhile needs the lock.
            lock.unlock();
            (void) session.clock.wait(remaining);
            lock.lock();
        } else {
            _changed.wait_for(lock, remaining);
        }
    }
    return true;
}

void GPSPipe::_wakeSession()
{
    // Taken so that the wake cannot fall between a wait's check of the cancellation and its sleep.
    {
        const std::lock_guard lock(_mutex);
    }
    _changed.notify_all();
}

GPSOpenResult GPSPipe::_open(const Session& session, std::chrono::milliseconds timeout)
{
    {
        const std::lock_guard lock(_mutex);
        if (session.cancelToken.isCancelled()) {
            return {GPSOpenStatus::Cancelled};
        }
        if (_sessionClosed) {
            return {GPSOpenStatus::Error, _detail};
        }
        _openRequested = true;
    }
    _notifyLink();

    std::unique_lock lock(_mutex);
    const bool answered = _waitUntil(lock, session, pipeDeadlineAfter(session.clock.nowUs(), timeout),
                                     [this] { return _link != Link::Connecting; });
    if (session.cancelToken.isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    if (!answered) {
        return {GPSOpenStatus::TimedOut, QCoreApplication::translate("GPSTransport", "Receiver connection timed out")};
    }
    if (_link != Link::Open) {
        return {GPSOpenStatus::Error, _detail};
    }
    _sessionOpened = true;
    return {GPSOpenStatus::Opened};
}

GPSReadResult GPSPipe::_read(const Session& session, std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    std::unique_lock lock(_mutex);
    if (session.cancelToken.isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!_sessionOpened || _sessionClosed) {
        return _failureLocked();
    }
    if (buffer.empty()) {
        // Asks for the reason of a failure the session has seen, without reading.
        return _failureReported ? _failureLocked() : GPSReadResult{GPSReadStatus::Data};
    }
    // Input fed before the link ended is still delivered; an overflow left none.
    (void) _waitUntil(lock, session, pipeDeadlineAfter(session.clock.nowUs(), timeout),
                      [this] { return !_input.isEmpty() || _unusableLocked(); });
    if (session.cancelToken.isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!_input.isEmpty()) {
        const qsizetype taken = (std::min) (static_cast<qsizetype>(buffer.size()), _input.size());
        std::memcpy(buffer.data(), _input.constData(), static_cast<size_t>(taken));
        _input.remove(0, taken);
        return {GPSReadStatus::Data, static_cast<int>(taken)};
    }
    return _unusableLocked() ? _reportFailureLocked() : GPSReadResult{GPSReadStatus::TimedOut};
}

GPSWriteResult GPSPipe::_write(const Session& session, QByteArrayView bytes, QDeadlineTimer deadline)
{
    std::unique_lock lock(_mutex);
    // A failed write that left output behind closed the session's side, so a healthy pipe has none pending.
    if (!_sessionOpened || _unusableLocked() || _pendingOutputLocked() != 0) {
        return {GPSWriteStatus::Error, 0, 0, _reportFailureLocked().detail};
    }
    // The deadline is in real time; what is left of it carries over to the session's clock.
    const uint64_t deadlineUs =
        deadline.isForever()
            ? UINT64_MAX
            : pipeDeadlineAfter(session.clock.nowUs(),
                                static_cast<uint64_t>((std::max) (deadline.remainingTimeNSecs(), qint64(0)) / 1000));
    const qsizetype length = bytes.size();
    const qint64 confirmedBefore = _confirmedTotal;
    qsizetype accepted = 0;
    while (accepted < length || _pendingOutputLocked() > 0) {
        if (session.cancelToken.isCancelled() || _unusableLocked() || session.clock.nowUs() >= deadlineUs) {
            break;
        }
        const qsizetype pending = _pendingOutputLocked();
        if (accepted < length && pending < GPSTransport::WRITE_BUFFER_BYTES) {
            const qsizetype count =
                (std::min) (length - accepted, static_cast<qsizetype>(GPSTransport::WRITE_BUFFER_BYTES) - pending);
            _output.append(bytes.sliced(accepted, count));
            accepted += count;
            lock.unlock();
            _notifyLink();
            lock.lock();
            continue;
        }
        (void) _waitUntil(lock, session, deadlineUs,
                          [this, pending] { return _unusableLocked() || _pendingOutputLocked() < pending; });
    }
    const int written = static_cast<int>(std::clamp<qint64>(_confirmedTotal - confirmedBefore, 0, accepted));
    GPSWriteStatus status = GPSWriteStatus::Completed;
    // A write confirmed in full succeeded, even if the deadline, a stop or the link's end followed it.
    if (written < length) {
        status = session.cancelToken.isCancelled() ? GPSWriteStatus::Cancelled
                 : _unusableLocked()               ? GPSWriteStatus::Error
                                                   : GPSWriteStatus::TimedOut;
    }
    GPSWriteResult result{status, static_cast<int>(accepted), written,
                          status == GPSWriteStatus::Error ? _reportFailureLocked().detail : QString()};
    lock.unlock();
    if (status != GPSWriteStatus::Completed && accepted > 0) {
        _closeSession();
    }
    return result;
}

void GPSPipe::_closeSession()
{
    {
        const std::lock_guard lock(_mutex);
        if (std::exchange(_sessionClosed, true)) {
            return;
        }
        _input.clear();
        _output.clear();
        _unconfirmed = 0;
    }
    _notifyLink();
}
