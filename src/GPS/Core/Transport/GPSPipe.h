#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

#include "GPSCancellation.h"
#include "GPSClock.h"
#include "GPSTransport.h"

/// Carries the bytes of one receiver session to and from a link the session's thread cannot wait on, such as a
/// Bluetooth socket, which reports only through an event loop. The session blocks on its side through
/// GPSPipeTransport. The link's owner feeds and drains the other side from its own thread as the link's events
/// arrive, which GPSPipeLink does for an owner with an event loop. The public members are the link's side and are
/// safe to call from any thread.
///
/// Input waits for the session in up to GPSTransport::READ_BUFFER_BYTES. A link that outruns the session cannot be
/// held back, so input beyond that ends the session with GPSReadStatus::Overflow, as a serial link's overrun does,
/// instead of handing it a stream with a gap. Output waits for the link in up to GPSTransport::WRITE_BUFFER_BYTES,
/// and a write lasts until the link confirmed it.
///
/// The session learns of the link's end from its own reads and writes, in order with the input fed before it, as
/// it learns of a socket's on its own thread: the reason comes with the read or write that fails.
class GPSPipe
{
public:
    GPSPipe();
    ~GPSPipe();
    Q_DISABLE_COPY_MOVE(GPSPipe)

    /// Runs @a notifier whenever the session's side changed: it asked for the link (openRequested()), queued output
    /// (takeOutput()) or ended (sessionClosed()). The notifier runs on the thread that made the change, so it only
    /// posts to the link's thread. Setting another one, or none, waits for a running call.
    void setLinkNotifier(std::function<void()> notifier);

    /// The session waits for the link: answer with opened() or close().
    [[nodiscard]] bool openRequested() const;
    /// The session is over, or will never start: nothing reads the pipe any more.
    [[nodiscard]] bool sessionClosed() const;

    /// The link is up.
    void opened();
    /// The link ended, or could not be brought up, for the reason @a detail gives the user. A session that waits for
    /// the link fails to open. One that reads gets the input fed before and then GPSReadStatus::Closed.
    void close(const QString& detail = {});
    /// Input from the open link, in the order it arrived.
    void feed(QByteArrayView bytes);
    /// Takes the output the session queued, for the link to send and then confirm.
    [[nodiscard]] QByteArray takeOutput();
    /// The link sent @a bytes of the output it took.
    void confirmWritten(qsizetype bytes);

private:
    friend class GPSPipeTransport;

    enum class Link
    {
        Connecting,
        Open,
        Ended,
    };

    /// The session that blocks on the pipe: what cancels it and the clock its waits run on.
    struct Session
    {
        const GPSCancelToken& cancelToken;
        const GPSClock& clock;
    };

    GPSOpenResult _open(const Session& session, std::chrono::milliseconds timeout);
    GPSReadResult _read(const Session& session, std::span<uint8_t> buffer, std::chrono::milliseconds timeout);
    GPSWriteResult _write(const Session& session, QByteArrayView bytes, QDeadlineTimer deadline);
    /// The session reads and writes no more; the link is told.
    void _closeSession();
    /// Wakes a wait of the session, for it to see its cancellation.
    void _wakeSession();
    /// The session has no link to read or write: it has not opened one, a read or a write of its own failed, or its
    /// side is closed.
    [[nodiscard]] bool _fatal() const;

    /// Waits on the session's clock until @a done holds, the session is cancelled or @a deadlineUs passes, with
    /// @a lock released meanwhile. @return whether @a done holds.
    template <typename Done>
    bool _waitUntil(std::unique_lock<std::mutex>& lock, const Session& session, uint64_t deadlineUs, Done done);
    void _notifyLink();

    /// The link carries nothing any more: it is not up, it ended or overflowed, or the session's side is closed.
    [[nodiscard]] bool _unusableLocked() const { return _sessionClosed || _overflow || _link != Link::Open; }

    /// Why the link is unusable, as a read reports it, which the session has seen from then on.
    [[nodiscard]] GPSReadResult _reportFailureLocked();
    [[nodiscard]] GPSReadResult _failureLocked() const;

    [[nodiscard]] qsizetype _pendingOutputLocked() const { return _output.size() + _unconfirmed; }

    mutable std::mutex _mutex;
    std::condition_variable _changed;
    Link _link = Link::Connecting;
    bool _openRequested = false;
    /// The session's open() found the link up.
    bool _sessionOpened = false;
    /// A read or a write of the session reported the link unusable.
    bool _failureReported = false;
    bool _sessionClosed = false;
    bool _overflow = false;
    QString _detail;
    QByteArray _input;
    /// Queued by the session and not taken by the link yet.
    QByteArray _output;
    /// Taken by the link and not confirmed yet.
    qsizetype _unconfirmed = 0;
    /// Every byte the link confirmed.
    qint64 _confirmedTotal = 0;

    /// Held while the notifier runs, so replacing it waits for a running call.
    std::mutex _notifierMutex;
    std::function<void()> _notifier;
};
