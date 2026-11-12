#include "PipeTransportChecks.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QEvent>
#include <QtCore/QEventLoop>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include "GPSCancellation.h"
#include "GPSPipe.h"
#include "GPSPipeLink.h"
#include "GPSPipeTransport.h"
#include "Protocols/Support/GPSTestClock.h"

using namespace std::chrono_literals;

namespace GPSTest {
namespace {

/// A wait that no check lets run out: only what the check does ends it.
constexpr std::chrono::milliseconds NEVER = std::chrono::hours(1);
constexpr GPSPipeTransport::Timeouts NO_TIMEOUTS{.open = NEVER, .write = NEVER};
/// How long a check waits for another thread before it reports the wait as its failure.
constexpr int WATCHDOG_MS = 30000;
/// The thread checks repeat: the thread that acts may run before the other one waits, or after.
constexpr int ROUNDS = 100;

#define PIPE_CHECK(condition)                                                                                        \
    do {                                                                                                             \
        if (!(condition)) {                                                                                          \
            return QStringLiteral("PipeTransportChecks.cc:%1: %2").arg(__LINE__).arg(QLatin1StringView(#condition)); \
        }                                                                                                            \
    } while (false)

QByteArray bytesOf(std::span<const uint8_t> buffer, const GPSReadResult& result)
{
    return QByteArray(reinterpret_cast<const char*>(buffer.data()), result.bytesRead);
}

/// @a size bytes that differ from one position to the next, and with @a seed.
QByteArray pattern(qsizetype size, int seed = 0)
{
    QByteArray bytes(size, Qt::Uninitialized);
    for (qsizetype i = 0; i < size; ++i) {
        bytes[i] = static_cast<char>((i * 31 + seed * 7) % 251);
    }
    return bytes;
}

/// The link's side for a check that acts on its own thread: the pipe's notifier runs on the session's thread and
/// only releases a latch.
class LinkLatch
{
public:
    explicit LinkLatch(std::shared_ptr<GPSPipe> pipe)
        : _pipe(std::move(pipe))
    {
        _pipe->setLinkNotifier([this] { _notified.release(); });
    }

    ~LinkLatch() { _pipe->setLinkNotifier({}); }

    Q_DISABLE_COPY_MOVE(LinkLatch)

    /// Waits for the session's side to make @a state hold. @return whether it holds.
    bool waitFor(const std::function<bool()>& state)
    {
        const QDeadlineTimer deadline(WATCHDOG_MS);
        while (!state()) {
            if (!_notified.tryAcquire(1, deadline)) {
                return state();
            }
        }
        return true;
    }

    bool notified() const { return _notified.available() > 0; }

private:
    const std::shared_ptr<GPSPipe> _pipe;
    QSemaphore _notified;
};

/// Runs a session's body as a receiver worker runs its transport: on a thread of its own. Destruction stops the
/// session and joins the thread, so a check that fails leaves no thread waiting.
class SessionThread
{
public:
    SessionThread(GPSCancelSource stop, std::function<void()> body)
        : _stop(std::move(stop))
        , _thread([this, body = std::move(body)] {
            body();
            _finished.release();
        })
    {}

    ~SessionThread()
    {
        _stop.cancel();
        _thread.join();
    }

    Q_DISABLE_COPY_MOVE(SessionThread)

    bool finished() { return _finished.tryAcquire(1, WATCHDOG_MS); }

private:
    GPSCancelSource _stop;
    QSemaphore _finished;
    std::thread _thread;
};

enum class Wake
{
    Input,
    LinkClosed,
    Stopped,
};

QString threadsOpenFollowsLink()
{
    for (int round = 0; round < ROUNDS; ++round) {
        // The link comes up, is refused, or stays away until the session is stopped.
        for (const auto answer : {GPSOpenStatus::Opened, GPSOpenStatus::Error, GPSOpenStatus::Cancelled}) {
            const auto pipe = std::make_shared<GPSPipe>();
            LinkLatch link(pipe);
            GPSCancelSource stop;
            GPSOpenResult opened;
            bool fatal = false;
            SessionThread session(stop, [&] {
                GPSPipeTransport transport(pipe, stop.token(), NO_TIMEOUTS);
                opened = transport.open();
                fatal = transport.fatalError();
            });
            PIPE_CHECK(link.waitFor([&] { return pipe->openRequested(); }));
            if (answer == GPSOpenStatus::Opened) {
                pipe->opened();
            } else if (answer == GPSOpenStatus::Error) {
                pipe->close(QStringLiteral("refused"));
            } else {
                stop.cancel();
            }
            PIPE_CHECK(session.finished());
            PIPE_CHECK(opened.status == answer);
            PIPE_CHECK(opened.detail == (answer == GPSOpenStatus::Error ? QStringLiteral("refused") : QString()));
            PIPE_CHECK(fatal == (answer != GPSOpenStatus::Opened));
            // The session's side ends with its transport, which the link is told.
            PIPE_CHECK(link.waitFor([&] { return pipe->sessionClosed(); }));
        }
    }
    return {};
}

QString threadsReadWakes()
{
    for (int round = 0; round < ROUNDS; ++round) {
        for (const auto wake : {Wake::Input, Wake::LinkClosed, Wake::Stopped}) {
            const auto pipe = std::make_shared<GPSPipe>();
            LinkLatch link(pipe);
            GPSCancelSource stop;
            QSemaphore reading;
            GPSReadResult read;
            QByteArray received;
            GPSReadResult reason;
            bool fatal = false;
            SessionThread session(stop, [&] {
                GPSPipeTransport transport(pipe, stop.token(), NO_TIMEOUTS);
                if (transport.open().status != GPSOpenStatus::Opened) {
                    return;
                }
                std::array<uint8_t, 8> buffer{};
                reading.release();
                read = transport.read(buffer, NEVER);
                received = bytesOf(buffer, read);
                // The empty read with which the driver asks a failed link for its reason.
                reason = transport.read({}, 0ms);
                fatal = transport.fatalError();
            });
            PIPE_CHECK(link.waitFor([&] { return pipe->openRequested(); }));
            pipe->opened();
            PIPE_CHECK(reading.tryAcquire(1, WATCHDOG_MS));
            switch (wake) {
                case Wake::Input:
                    pipe->feed("abc");
                    break;
                case Wake::LinkClosed:
                    pipe->close(QStringLiteral("lost"));
                    break;
                case Wake::Stopped:
                    stop.cancel();
                    break;
            }
            PIPE_CHECK(session.finished());
            switch (wake) {
                case Wake::Input:
                    PIPE_CHECK(read.status == GPSReadStatus::Data);
                    PIPE_CHECK(received == "abc");
                    PIPE_CHECK(reason.status == GPSReadStatus::Data && reason.bytesRead == 0);
                    PIPE_CHECK(!fatal);
                    break;
                case Wake::LinkClosed:
                    PIPE_CHECK(read.status == GPSReadStatus::Closed && read.bytesRead == 0);
                    PIPE_CHECK(read.detail == QStringLiteral("lost"));
                    PIPE_CHECK(reason.status == GPSReadStatus::Closed && reason.detail == QStringLiteral("lost"));
                    PIPE_CHECK(fatal);
                    break;
                case Wake::Stopped:
                    PIPE_CHECK(read.status == GPSReadStatus::Cancelled && read.bytesRead == 0);
                    PIPE_CHECK(reason.status == GPSReadStatus::Cancelled);
                    break;
            }
        }
    }
    return {};
}

QString threadsWriteWaitsForLink()
{
    const QByteArray payload = pattern(3 * GPSTransport::WRITE_BUFFER_BYTES + 100);
    for (int round = 0; round < ROUNDS; ++round) {
        const auto pipe = std::make_shared<GPSPipe>();
        LinkLatch link(pipe);
        GPSCancelSource stop;
        GPSWriteResult written;
        SessionThread session(stop, [&] {
            GPSPipeTransport transport(pipe, stop.token(), NO_TIMEOUTS);
            if (transport.open().status == GPSOpenStatus::Opened) {
                written = transport.write(payload, QDeadlineTimer(QDeadlineTimer::Forever));
            }
        });
        PIPE_CHECK(link.waitFor([&] { return pipe->openRequested(); }));
        pipe->opened();
        QByteArray sent;
        while (sent.size() < payload.size()) {
            QByteArray chunk;
            PIPE_CHECK(link.waitFor([&] {
                chunk = pipe->takeOutput();
                return !chunk.isEmpty();
            }));
            // The pipe never holds more output than a link queues at once.
            PIPE_CHECK(chunk.size() <= GPSTransport::WRITE_BUFFER_BYTES);
            sent += chunk;
            pipe->confirmWritten(chunk.size() / 2);
            pipe->confirmWritten(chunk.size() - chunk.size() / 2);
        }
        PIPE_CHECK(session.finished());
        PIPE_CHECK(written.status == GPSWriteStatus::Completed);
        PIPE_CHECK(written.acceptedBytes == payload.size() && written.writtenBytes == payload.size());
        PIPE_CHECK(sent == payload);
    }
    return {};
}

QString threadsWriteEndsWithLinkOrStop()
{
    const QByteArray command("command");
    for (int round = 0; round < ROUNDS; ++round) {
        for (const auto wake : {Wake::LinkClosed, Wake::Stopped}) {
            const auto pipe = std::make_shared<GPSPipe>();
            LinkLatch link(pipe);
            GPSCancelSource stop;
            GPSWriteResult written;
            GPSWriteResult next;
            bool fatal = false;
            SessionThread session(stop, [&] {
                GPSPipeTransport transport(pipe, stop.token(), NO_TIMEOUTS);
                if (transport.open().status != GPSOpenStatus::Opened) {
                    return;
                }
                written = transport.write(command, QDeadlineTimer(QDeadlineTimer::Forever));
                next = transport.write(command, QDeadlineTimer(QDeadlineTimer::Forever));
                fatal = transport.fatalError();
            });
            PIPE_CHECK(link.waitFor([&] { return pipe->openRequested(); }));
            pipe->opened();
            // The link takes the command and ends, or the session is stopped, before any of it is confirmed.
            PIPE_CHECK(link.waitFor([&] { return !pipe->takeOutput().isEmpty(); }));
            if (wake == Wake::LinkClosed) {
                pipe->close(QStringLiteral("lost"));
            } else {
                stop.cancel();
            }
            PIPE_CHECK(session.finished());
            const bool lost = wake == Wake::LinkClosed;
            PIPE_CHECK(written.status == (lost ? GPSWriteStatus::Error : GPSWriteStatus::Cancelled));
            PIPE_CHECK(written.acceptedBytes == command.size() && written.writtenBytes == 0);
            PIPE_CHECK(written.uncertainBytes() == command.size());
            PIPE_CHECK(written.detail == (lost ? QStringLiteral("lost") : QString()));
            // A failed write that left bytes with the link retired the session's side.
            PIPE_CHECK(next.status == written.status && next.acceptedBytes == 0);
            PIPE_CHECK(fatal);
        }
    }
    return {};
}

QString threadsBytesCrossInOrder()
{
    constexpr int MESSAGES = 40;
    const auto pipe = std::make_shared<GPSPipe>();
    LinkLatch link(pipe);
    GPSCancelSource stop;
    qsizetype expected = 0;
    QList<QByteArray> messages;
    for (int i = 0; i < MESSAGES; ++i) {
        // Some are longer than the pipe queues for writing at once.
        messages.append(pattern(1 + (i * 733) % 6000, i));
        expected += messages.last().size();
    }
    bool ordered = false;
    SessionThread session(stop, [&] {
        GPSPipeTransport transport(pipe, stop.token(), NO_TIMEOUTS);
        if (transport.open().status != GPSOpenStatus::Opened) {
            return;
        }
        std::array<uint8_t, 512> buffer{};
        for (const QByteArray& message : std::as_const(messages)) {
            if (transport.write(message, QDeadlineTimer(QDeadlineTimer::Forever)).status != GPSWriteStatus::Completed) {
                return;
            }
            // The link sends back what it was sent. The whole reply is read before the next message is written, so
            // the unread input stays far below the pipe's limit.
            QByteArray reply;
            while (reply.size() < message.size()) {
                const GPSReadResult read = transport.read(buffer, NEVER);
                if (read.status != GPSReadStatus::Data) {
                    return;
                }
                reply += bytesOf(buffer, read);
            }
            if (reply != message) {
                return;
            }
        }
        ordered = true;
    });
    PIPE_CHECK(link.waitFor([&] { return pipe->openRequested(); }));
    pipe->opened();
    for (qsizetype echoed = 0; echoed < expected;) {
        QByteArray chunk;
        PIPE_CHECK(link.waitFor([&] {
            chunk = pipe->takeOutput();
            return !chunk.isEmpty();
        }));
        pipe->confirmWritten(chunk.size());
        pipe->feed(chunk);
        echoed += chunk.size();
    }
    PIPE_CHECK(session.finished());
    PIPE_CHECK(ordered);
    return {};
}

/// A link that is up as soon as the session asks, on the session's own thread, for the checks that run on one.
class ReadyLink
{
public:
    explicit ReadyLink(std::shared_ptr<GPSPipe> pipe)
        : _pipe(std::move(pipe))
    {
        _pipe->setLinkNotifier([this] {
            if (_pipe->openRequested()) {
                _pipe->opened();
            }
            taken += _pipe->takeOutput();
        });
    }

    ~ReadyLink() { _pipe->setLinkNotifier({}); }

    Q_DISABLE_COPY_MOVE(ReadyLink)

    /// The output the link took and has not confirmed.
    QByteArray taken;

private:
    const std::shared_ptr<GPSPipe> _pipe;
};

QString pipeOverflowEndsSession()
{
    const auto pipe = std::make_shared<GPSPipe>();
    ReadyLink link(pipe);
    GPSCancelSource stop;
    GPSPipeTransport transport(pipe, stop.token());
    PIPE_CHECK(transport.open().status == GPSOpenStatus::Opened);
    std::array<uint8_t, 16> buffer{};

    // As much unread input as a link buffers is held.
    pipe->feed(pattern(GPSTransport::READ_BUFFER_BYTES));
    GPSReadResult read = transport.read(buffer, 0ms);
    PIPE_CHECK(read.status == GPSReadStatus::Data && bytesOf(buffer, read) == pattern(16));
    PIPE_CHECK(!transport.fatalError());

    // One byte more than that ends the session, which gets nothing of a stream that has a gap.
    pipe->feed(QByteArray(17, 'x'));
    PIPE_CHECK(!transport.fatalError());
    read = transport.read(buffer, 0ms);
    PIPE_CHECK(read.status == GPSReadStatus::Overflow && read.bytesRead == 0);
    PIPE_CHECK(!read.detail.isEmpty());
    PIPE_CHECK(transport.fatalError());
    PIPE_CHECK(transport.read({}, 0ms).status == GPSReadStatus::Overflow);
    pipe->feed("late");
    PIPE_CHECK(transport.read(buffer, 0ms).status == GPSReadStatus::Overflow);
    const GPSWriteResult written = transport.write("command", QDeadlineTimer(QDeadlineTimer::Forever));
    PIPE_CHECK(written.status == GPSWriteStatus::Error && written.acceptedBytes == 0);
    PIPE_CHECK(!transport.setBaudRate(GPSTransport::BRIDGE_BAUD_RATE));
    return {};
}

QString pipeSessionEndReachesLink()
{
    {
        // The transport's destruction ends the session's side.
        const auto pipe = std::make_shared<GPSPipe>();
        LinkLatch link(pipe);
        {
            GPSCancelSource stop;
            const GPSPipeTransport transport(pipe, stop.token());
            // Nothing is read, written or set on a link that was not opened.
            PIPE_CHECK(transport.fatalError());
            PIPE_CHECK(!pipe->sessionClosed());
        }
        PIPE_CHECK(pipe->sessionClosed());
        PIPE_CHECK(link.notified());
    }
    {
        // So does the destruction of a factory that never created its transport, with its last copy.
        auto pipe = std::make_shared<GPSPipe>();
        LinkLatch link(pipe);
        {
            auto factory = GPSPipeTransport::factory(pipe, {});
            const auto copy = factory;
            factory = {};
            PIPE_CHECK(!pipe->sessionClosed());
        }
        PIPE_CHECK(pipe->sessionClosed());
        PIPE_CHECK(link.notified());
    }
    {
        // The factory keeps the pipe for its transport.
        auto pipe = std::make_shared<GPSPipe>();
        const std::weak_ptr<GPSPipe> watched = pipe;
        auto factory = GPSPipeTransport::factory(std::move(pipe), {});
        GPSCancelSource stop;
        auto transport = factory(stop.token());
        factory = {};
        PIPE_CHECK(!watched.expired() && !watched.lock()->sessionClosed());
        PIPE_CHECK(transport->fixedBaudRate() == GPSTransport::BRIDGE_BAUD_RATE);
        transport.reset();
        PIPE_CHECK(watched.expired());
    }
    return {};
}

/// A virtual clock whose waits let the link act: a wait moves the time to the link's next act that is due before
/// the wait ends and performs it, and otherwise to its own end.
class ScriptedClock
{
public:
    ScriptedClock() = default;
    Q_DISABLE_COPY_MOVE(ScriptedClock)

    /// Performs @a act once the clock has run for @a time.
    void at(std::chrono::milliseconds time, std::function<void()> act)
    {
        _acts.emplace(GPSTestClock::START_US + static_cast<uint64_t>(std::chrono::microseconds(time).count()),
                      std::move(act));
    }

    std::chrono::microseconds elapsed() const
    {
        return std::chrono::microseconds(_clock.nowUs() - GPSTestClock::START_US);
    }

    GPSClock source()
    {
        return {.nowUs = [this] { return _clock.nowUs(); },
                .wait =
                    [this](std::chrono::microseconds duration) {
                        const uint64_t endUs = _clock.nowUs() + static_cast<uint64_t>(duration.count());
                        const auto next = _acts.begin();
                        if (next == _acts.end() || next->first > endUs) {
                            _clock.advanceTo(endUs);
                            return true;
                        }
                        _clock.advanceTo(next->first);
                        const std::function<void()> act = std::move(next->second);
                        _acts.erase(next);
                        act();
                        return true;
                    }};
    }

private:
    GPSTestClock _clock{GPSTestClock::START_US};
    std::multimap<uint64_t, std::function<void()>> _acts;
};

QString clockReadDeadline()
{
    const auto pipe = std::make_shared<GPSPipe>();
    ReadyLink link(pipe);
    ScriptedClock clock;
    GPSCancelSource stop;
    GPSPipeTransport transport(pipe, stop.token(), {}, clock.source());
    PIPE_CHECK(transport.open().status == GPSOpenStatus::Opened);
    PIPE_CHECK(clock.elapsed() == 0ms);
    std::array<uint8_t, 8> buffer{};

    // A read without input lasts as long as its timeout, and leaves the link usable.
    PIPE_CHECK(transport.read(buffer, 250ms).status == GPSReadStatus::TimedOut);
    PIPE_CHECK(clock.elapsed() == 250ms);
    PIPE_CHECK(!transport.fatalError());
    // A timeout that is not positive polls.
    PIPE_CHECK(transport.read(buffer, 0ms).status == GPSReadStatus::TimedOut);
    PIPE_CHECK(transport.read(buffer, -5ms).status == GPSReadStatus::TimedOut);
    PIPE_CHECK(clock.elapsed() == 250ms);

    // Input ends the wait when it arrives, and input that waits is read without waiting.
    clock.at(400ms, [&] { pipe->feed("abcdefghij"); });
    GPSReadResult read = transport.read(buffer, 1000ms);
    PIPE_CHECK(read.status == GPSReadStatus::Data && bytesOf(buffer, read) == "abcdefgh");
    PIPE_CHECK(clock.elapsed() == 400ms);
    read = transport.read(buffer, 1000ms);
    PIPE_CHECK(read.status == GPSReadStatus::Data && bytesOf(buffer, read) == "ij");
    PIPE_CHECK(clock.elapsed() == 400ms);

    // So does the link's end, after the input it fed before.
    clock.at(500ms, [&] {
        pipe->feed("k");
        pipe->close(QStringLiteral("lost"));
    });
    read = transport.read(buffer, 1000ms);
    PIPE_CHECK(read.status == GPSReadStatus::Data && bytesOf(buffer, read) == "k");
    // The session sees the end where it is in the stream: with the read that finds nothing before it.
    PIPE_CHECK(!transport.fatalError());
    PIPE_CHECK(transport.read({}, 0ms).status == GPSReadStatus::Data);
    read = transport.read(buffer, 1000ms);
    PIPE_CHECK(read.status == GPSReadStatus::Closed && read.bytesRead == 0 && read.detail == QStringLiteral("lost"));
    PIPE_CHECK(transport.fatalError());
    PIPE_CHECK(transport.read({}, 0ms).status == GPSReadStatus::Closed);
    PIPE_CHECK(clock.elapsed() == 500ms);
    return {};
}

QString clockOpenDeadline()
{
    constexpr GPSPipeTransport::Timeouts timeouts{.open = 5000ms, .write = 5000ms};
    {
        // A link that does not answer is waited for as long as the open timeout, and then told to stop.
        const auto pipe = std::make_shared<GPSPipe>();
        ScriptedClock clock;
        GPSCancelSource stop;
        GPSPipeTransport transport(pipe, stop.token(), timeouts, clock.source());
        const GPSOpenResult opened = transport.open();
        PIPE_CHECK(opened.status == GPSOpenStatus::TimedOut);
        PIPE_CHECK(opened.detail == QStringLiteral("Receiver connection timed out"));
        PIPE_CHECK(clock.elapsed() == 5000ms);
        PIPE_CHECK(pipe->sessionClosed());
        PIPE_CHECK(transport.fatalError());
        // The link's answer comes too late for this session.
        pipe->opened();
        PIPE_CHECK(transport.open().status == GPSOpenStatus::Error);
    }
    for (const bool up : {true, false}) {
        // Its answer ends the wait.
        const auto pipe = std::make_shared<GPSPipe>();
        ScriptedClock clock;
        clock.at(3000ms, [&] {
            if (up) {
                pipe->opened();
            } else {
                pipe->close(QStringLiteral("refused"));
            }
        });
        GPSCancelSource stop;
        GPSPipeTransport transport(pipe, stop.token(), timeouts, clock.source());
        const GPSOpenResult opened = transport.open();
        PIPE_CHECK(opened.status == (up ? GPSOpenStatus::Opened : GPSOpenStatus::Error));
        PIPE_CHECK(opened.detail == (up ? QString() : QStringLiteral("refused")));
        PIPE_CHECK(clock.elapsed() == 3000ms);
        PIPE_CHECK(pipe->sessionClosed() == !up);
    }
    return {};
}

QString clockWriteDeadline()
{
    enum class Link
    {
        Confirms,
        ConfirmsHalf,
        Silent,
        ConfirmsAndEnds,
        Ends,
    };
    const QByteArray command = pattern(600);
    const int length = static_cast<int>(command.size());
    for (const auto behaviour : {Link::Confirms, Link::ConfirmsHalf, Link::Silent, Link::ConfirmsAndEnds, Link::Ends}) {
        const auto pipe = std::make_shared<GPSPipe>();
        ReadyLink link(pipe);
        ScriptedClock clock;
        GPSCancelSource stop;
        GPSPipeTransport transport(pipe, stop.token(), {.open = 5000ms, .write = NEVER}, clock.source());
        PIPE_CHECK(transport.open().status == GPSOpenStatus::Opened);
        // The link takes the command at once and answers a second later; the write may take a minute.
        clock.at(1000ms, [&] {
            if (behaviour != Link::Silent && behaviour != Link::Ends) {
                pipe->confirmWritten(behaviour == Link::ConfirmsHalf ? length / 2 : length);
            }
            if (behaviour == Link::ConfirmsAndEnds || behaviour == Link::Ends) {
                pipe->close(QStringLiteral("lost"));
            }
        });
        const GPSWriteResult written = transport.write(command, QDeadlineTimer(60s));
        PIPE_CHECK(link.taken == command);
        PIPE_CHECK(written.acceptedBytes == length);
        switch (behaviour) {
            case Link::Confirms:
            case Link::ConfirmsAndEnds:
                // A write the link confirmed in full succeeded, whatever became of the link afterwards.
                PIPE_CHECK(written.status == GPSWriteStatus::Completed && written.writtenBytes == length);
                PIPE_CHECK(clock.elapsed() == 1000ms);
                PIPE_CHECK(!pipe->sessionClosed());
                break;
            case Link::ConfirmsHalf:
            case Link::Silent:
                PIPE_CHECK(written.status == GPSWriteStatus::TimedOut);
                PIPE_CHECK(written.writtenBytes == (behaviour == Link::Silent ? 0 : length / 2));
                PIPE_CHECK(written.detail.isEmpty());
                // The deadline is in real time, and what was left of it when the write began ran on the clock.
                PIPE_CHECK(clock.elapsed() > 30s && clock.elapsed() <= 60s);
                break;
            case Link::Ends:
                PIPE_CHECK(written.status == GPSWriteStatus::Error && written.writtenBytes == 0);
                PIPE_CHECK(written.detail == QStringLiteral("lost"));
                PIPE_CHECK(clock.elapsed() == 1000ms);
                break;
        }
        if (written.status != GPSWriteStatus::Completed) {
            // The bytes left with the link make the stream uncertain, so the session's side is retired.
            PIPE_CHECK(pipe->sessionClosed());
            PIPE_CHECK(transport.fatalError());
            PIPE_CHECK(transport.write(command, QDeadlineTimer(60s)).acceptedBytes == 0);
        }
    }
    {
        // The transport's own write timeout caps a longer deadline.
        const auto pipe = std::make_shared<GPSPipe>();
        ReadyLink link(pipe);
        ScriptedClock clock;
        GPSCancelSource stop;
        GPSPipeTransport transport(pipe, stop.token(), {.open = 5000ms, .write = 200ms}, clock.source());
        PIPE_CHECK(transport.open().status == GPSOpenStatus::Opened);
        PIPE_CHECK(transport.configurationWriteTimeout() == 200ms);
        const GPSWriteResult written = transport.write(command, QDeadlineTimer(QDeadlineTimer::Forever));
        PIPE_CHECK(written.status == GPSWriteStatus::TimedOut);
        PIPE_CHECK(clock.elapsed() > 0ms && clock.elapsed() <= 200ms);
    }
    return {};
}

/// What a link's owner was asked for, which outlives the owner.
struct LinkRecord
{
    int opens = 0;
    int closes = 0;
    QByteArray written;
    /// The session was served on another thread than the owner's.
    bool servedElsewhere = false;
};

/// A link's owner with an event loop, whose link the check scripts.
class ScriptedPipeLink final : public GPSPipeLink
{
public:
    using GPSPipeLink::GPSPipeLink;
    using GPSPipeLink::linkClosed;
    using GPSPipeLink::linkOpened;
    using GPSPipeLink::linkReceived;
    using GPSPipeLink::linkWrote;

    std::function<void()> onOpen;
    std::function<void(const QByteArray&)> onWrite;
    const std::shared_ptr<LinkRecord> record = std::make_shared<LinkRecord>();

protected:
    void openLink() override
    {
        _served();
        ++record->opens;
        if (onOpen) {
            onOpen();
        }
    }

    void writeLink(const QByteArray& bytes) override
    {
        _served();
        record->written += bytes;
        if (onWrite) {
            onWrite(bytes);
        }
    }

    void closeLink() override
    {
        _served();
        ++record->closes;
    }

private:
    void _served() { record->servedElsewhere = record->servedElsewhere || QThread::currentThread() != thread(); }
};

/// Runs the calling thread's event loop until @a loop is quit; false when that takes too long.
bool runUntilQuit(QEventLoop& loop)
{
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, &loop, [&loop] { loop.exit(1); });
    watchdog.start(WATCHDOG_MS);
    return loop.exec() == 0;
}

QString linkServesSessionOnItsThread()
{
    PIPE_CHECK(QCoreApplication::instance());
    const auto pipe = std::make_shared<GPSPipe>();
    const QPointer<ScriptedPipeLink> link = new ScriptedPipeLink(pipe);
    const std::shared_ptr<LinkRecord> asked = link->record;
    link->onOpen = [link] { link->linkOpened(); };
    link->onWrite = [link](const QByteArray& bytes) {
        link->linkWrote(bytes.size());
        link->linkReceived("reply");
    };
    QEventLoop loop;
    GPSCancelSource stop;
    GPSOpenResult opened;
    GPSWriteResult written;
    GPSReadResult read;
    QByteArray reply;
    SessionThread session(stop, [&] {
        {
            const auto transport = GPSPipeTransport::factory(pipe, NO_TIMEOUTS)(stop.token());
            opened = transport->open();
            written = transport->write("command", QDeadlineTimer(QDeadlineTimer::Forever));
            std::array<uint8_t, 16> buffer{};
            read = transport->read(buffer, NEVER);
            reply = bytesOf(buffer, read);
        }
        // Posted after what the session's end posted to the link, so the link hears of the end first.
        (void) QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection);
    });
    PIPE_CHECK(runUntilQuit(loop));
    PIPE_CHECK(session.finished());
    PIPE_CHECK(opened.status == GPSOpenStatus::Opened);
    PIPE_CHECK(written.status == GPSWriteStatus::Completed && written.writtenBytes == 7);
    PIPE_CHECK(read.status == GPSReadStatus::Data && reply == "reply");
    // The link is brought up once, on its owner's thread, and taken down when the session is over.
    PIPE_CHECK(asked->opens == 1 && asked->closes == 1);
    PIPE_CHECK(asked->written == "command");
    PIPE_CHECK(!asked->servedElsewhere);
    // Its owner is done with it then: it goes when control is back in the event loop, if it has not gone already.
    QCoreApplication::sendPostedEvents(link, QEvent::DeferredDelete);
    PIPE_CHECK(!link);
    return {};
}

QString linkEndsWithItsOwner()
{
    PIPE_CHECK(QCoreApplication::instance());
    {
        // An owner that goes while the session reads ends the link for it.
        const auto pipe = std::make_shared<GPSPipe>();
        auto* const link = new ScriptedPipeLink(pipe);
        link->onOpen = [link] { link->linkOpened(); };
        QEventLoop loop;
        GPSCancelSource stop;
        GPSReadResult read;
        SessionThread session(stop, [&] {
            GPSPipeTransport transport(pipe, stop.token(), NO_TIMEOUTS);
            const GPSOpenResult opened = transport.open();
            (void) QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection);
            if (opened.status == GPSOpenStatus::Opened) {
                std::array<uint8_t, 8> buffer{};
                read = transport.read(buffer, NEVER);
            }
        });
        PIPE_CHECK(runUntilQuit(loop));
        delete link;
        PIPE_CHECK(session.finished());
        PIPE_CHECK(read.status == GPSReadStatus::Closed);
    }
    {
        // A session that never created its transport is over too: the link is taken down without having been
        // brought up, and its owner is done with it.
        const auto pipe = std::make_shared<GPSPipe>();
        const QPointer<ScriptedPipeLink> link = new ScriptedPipeLink(pipe);
        const std::shared_ptr<LinkRecord> asked = link->record;
        {
            const auto factory = GPSPipeTransport::factory(pipe, {});
        }
        QCoreApplication::sendPostedEvents(link, QEvent::MetaCall);
        PIPE_CHECK(asked->opens == 0 && asked->closes == 1);
        QCoreApplication::sendPostedEvents(link, QEvent::DeferredDelete);
        PIPE_CHECK(!link);
    }
    return {};
}

constexpr PipeTransportCheck CHECKS[] = {
    {"threads-open-follows-link", threadsOpenFollowsLink},
    {"threads-read-wakes", threadsReadWakes},
    {"threads-write-waits-for-link", threadsWriteWaitsForLink},
    {"threads-write-ends-with-link-or-stop", threadsWriteEndsWithLinkOrStop},
    {"threads-bytes-cross-in-order", threadsBytesCrossInOrder},
    {"pipe-overflow-ends-session", pipeOverflowEndsSession},
    {"pipe-session-end-reaches-link", pipeSessionEndReachesLink},
    {"clock-read-deadline", clockReadDeadline},
    {"clock-open-deadline", clockOpenDeadline},
    {"clock-write-deadline", clockWriteDeadline},
    {"link-serves-session-on-its-thread", linkServesSessionOnItsThread},
    {"link-ends-with-its-owner", linkEndsWithItsOwner},
};

}  // namespace

std::span<const PipeTransportCheck> pipeTransportChecks()
{
    return CHECKS;
}

}  // namespace GPSTest
