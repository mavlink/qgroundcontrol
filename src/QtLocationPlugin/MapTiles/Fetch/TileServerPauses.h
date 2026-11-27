#pragma once

#include <chrono>
#include <functional>
#include <optional>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class QDateTime;
class QHttpHeaders;
class RuntimeScheduler;

/// The tile servers that answered that they were asked too often (HTTP 429), or that they are unavailable for a time
/// they named (HTTP 503 with Retry-After), by the service they are of (MapProvider::service()): no request for a tile
/// (TileReply) asks such a server until its pause ends, as the requests share the pauses of the map engine
/// (MapEngine::serverPauses()). The servers of a service take turns by tile and serve all its maps, so all tiles of
/// the service wait, and those of other services do not.
///
/// A pause lasts as long as the server's Retry-After asks, or kDefaultPause without one, within kMinPause and
/// kMaxPause. A pause for a 429 that follows another without an answer of the server other than 429 in between lasts
/// at least twice as long as the one before, up to kMaxPause. A pause for a 503 lasts as long as the server asked, and
/// counts as an answer other than 429.
///
/// When a pause ends, the requests that it held ask the server one after the other, kReleaseInterval apart, in the
/// order they were held: the server that asked for quiet gets no burst. A request that would ask the server while they
/// are let through waits for its turn after them, and a 429 among their answers holds the rest for the next pause. The
/// n-th held request asks (n - 1) * kReleaseInterval after the pause ends, unless the server asks for another one.
///
/// For the main thread only, as the requests are.
class TileServerPauses : public QObject
{
    Q_OBJECT

public:
    static constexpr std::chrono::seconds kDefaultPause{30};
    static constexpr std::chrono::seconds kMinPause{1};
    static constexpr std::chrono::seconds kMaxPause{600};
    static constexpr std::chrono::milliseconds kReleaseInterval{250};

    /// What the server answered
    enum class Reason
    {
        TooManyRequests,  ///< HTTP 429
        Unavailable,      ///< HTTP 503, with the time to wait
    };

    /// \a scheduler, which is not owned, times the pauses: a timer of the event loop when it is null
    explicit TileServerPauses(RuntimeScheduler* scheduler = nullptr, QObject* parent = nullptr);
    ~TileServerPauses() override;

    /// Holds \a request, which would ask a server of \a service, while the service is paused or the requests that a
    /// pause held are let through: \a ask is called once, at its turn.
    /// \return False when the request can ask now, and \a ask is not called
    bool hold(const QString& service, QObject* request, std::function<void()> ask);

    /// \a request, which was held for \a service, asks no more: it takes no turn
    void withdraw(const QString& service, QObject* request);

    /// A server of \a service answered as \a reason tells, and asked to wait \a retryAfter. An answer during a pause,
    /// to a request made before it, lengthens the pause to what it asks, and does not count as another pause.
    void pause(const QString& service, std::optional<std::chrono::seconds> retryAfter,
               Reason reason = Reason::TooManyRequests);

    /// A server of \a service answered other than 429: its next pause is not longer than it asks for
    void answered(const QString& service);

    /// How long the Retry-After header in \a headers asks to wait at \a utcNow: delta-seconds or an HTTP date, which
    /// asks for none once it is past. None without a header that can be read.
    static std::optional<std::chrono::seconds> retryAfter(const QHttpHeaders& headers, const QDateTime& utcNow);

private:
    struct Held
    {
        QPointer<QObject> request;
        std::function<void()> ask;
    };

    struct Pause
    {
        quint64 untilUs = 0;             ///< The time of the scheduler at which it ends; 0 once it ended
        std::chrono::seconds length{0};  ///< Of the latest pause for a 429; 0 after an answer other than 429
        quint64 task = 0;                ///< The scheduler's task that ends it
        QList<Held> held;                ///< In the order they were held
        quint64 releaseTask = 0;         ///< The scheduler's task that lets the next held request ask
    };

    /// \return False when the end of \a pause cannot be scheduled: the pause then ended
    bool _schedule(const QString& service, Pause& pause, quint64 nowUs);
    void _end(const QString& service);
    /// Lets the first held request ask, unless the service is paused again
    void _release(const QString& service);
    /// Lets every held request ask at once, unless the service is paused again: for a pause or a turn that cannot be
    /// scheduled, which would hold them for ever
    void _releaseAll(const QString& service);

    QPointer<RuntimeScheduler> _scheduler;
    QHash<QString, Pause> _pauses;
};
