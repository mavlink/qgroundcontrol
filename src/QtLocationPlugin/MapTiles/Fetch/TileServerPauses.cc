#include "TileServerPauses.h"

#include <algorithm>

#include <QtCore/QDateTime>
#include <QtNetwork/QHttpHeaders>

#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(TileServerPausesLog, "MapTiles.TileServerPauses")

namespace {

quint64 tileServerPauseUs(std::chrono::seconds length)
{
    return static_cast<quint64>(std::chrono::duration_cast<std::chrono::microseconds>(length).count());
}

}  // namespace

TileServerPauses::TileServerPauses(RuntimeScheduler* scheduler, QObject* parent)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
{
    qCDebug(TileServerPausesLog) << this;
}

TileServerPauses::~TileServerPauses()
{
    qCDebug(TileServerPausesLog) << this;
}

bool TileServerPauses::hold(const QString& service, QObject* request, std::function<void()> ask)
{
    // Without its scheduler a pause would never end
    const auto pause = _pauses.find(service);
    if (!_scheduler || (pause == _pauses.end()) || ((pause->untilUs == 0) && pause->held.isEmpty())) {
        return false;
    }
    pause->held.append({request, std::move(ask)});
    qCDebug(TileServerPausesLog) << "a request for the servers of" << service << "waits, as" << pause->held.size()
                                 << "do";
    return true;
}

void TileServerPauses::withdraw(const QString& service, QObject* request)
{
    const auto pause = _pauses.find(service);
    if (pause == _pauses.end()) {
        return;
    }
    (void) pause->held.removeIf([request](const Held& held) { return held.request == request; });
    if (pause->held.isEmpty() && (pause->releaseTask != 0)) {
        if (_scheduler) {
            _scheduler->cancel(pause->releaseTask);
        }
        pause->releaseTask = 0;
    }
}

void TileServerPauses::pause(const QString& service, std::optional<std::chrono::seconds> retryAfter, Reason reason)
{
    if (!_scheduler) {
        return;
    }

    const std::chrono::seconds asked = std::clamp(retryAfter.value_or(kDefaultPause), kMinPause, kMaxPause);
    const bool tooManyRequests = reason == Reason::TooManyRequests;
    const quint64 nowUs = _scheduler->nowUs();
    Pause& pause = _pauses[service];
    if (pause.untilUs != 0) {
        // An answer other than 429
        if (!tooManyRequests) {
            pause.length = std::chrono::seconds{0};
        }
        const quint64 untilUs = nowUs + tileServerPauseUs(asked);
        if (untilUs > pause.untilUs) {
            if (tooManyRequests) {
                pause.length = std::max(pause.length, asked);
            }
            pause.untilUs = untilUs;
            if (!_schedule(service, pause, nowUs)) {
                _releaseAll(service);
                return;
            }
            qCDebug(TileServerPausesLog) << "the pause of the servers of" << service << "lengthened to" << asked
                                         << "from now";
        }
        return;
    }

    // The requests held by the pause before that are not let through any more: this one holds them
    if (pause.releaseTask != 0) {
        _scheduler->cancel(pause.releaseTask);
        pause.releaseTask = 0;
    }
    // A server that is unavailable named its time, which the pauses before do not lengthen, and did not answer 429
    const std::chrono::seconds length =
        tooManyRequests ? std::min(std::max(asked, 2 * pause.length), kMaxPause) : asked;
    pause.length = tooManyRequests ? length : std::chrono::seconds{0};
    pause.untilUs = nowUs + tileServerPauseUs(length);
    if (!_schedule(service, pause, nowUs)) {
        _releaseAll(service);
        return;
    }
    qCWarning(TileServerPausesLog) << "the servers of" << service
                                   << (tooManyRequests ? "was asked too often:" : "is unavailable:")
                                   << "none of its tiles is asked for for" << length;
}

void TileServerPauses::answered(const QString& service)
{
    if (const auto pause = _pauses.find(service); pause != _pauses.end()) {
        pause->length = std::chrono::seconds{0};
    }
}

std::optional<std::chrono::seconds> TileServerPauses::retryAfter(const QHttpHeaders& headers, const QDateTime& utcNow)
{
    // Not intValue() first: Qt 6.12 reads it through a null pointer for headers that are empty
    if (!headers.contains(QHttpHeaders::WellKnownHeader::RetryAfter)) {
        return std::nullopt;
    }
    if (const std::optional<qint64> seconds = headers.intValue(QHttpHeaders::WellKnownHeader::RetryAfter)) {
        return (*seconds >= 0) ? std::optional(std::chrono::seconds{*seconds}) : std::nullopt;
    }
    if (const std::optional<QDateTime> date = headers.dateTimeValue(QHttpHeaders::WellKnownHeader::RetryAfter);
        date && date->isValid()) {
        return std::chrono::seconds{std::max<qint64>(0, utcNow.secsTo(*date))};
    }
    return std::nullopt;
}

bool TileServerPauses::_schedule(const QString& service, Pause& pause, quint64 nowUs)
{
    if (pause.task != 0) {
        _scheduler->cancel(pause.task);
    }
    pause.task = _scheduler->schedule(this, std::chrono::microseconds{pause.untilUs - nowUs},
                                      [this, service] { _end(service); });
    if (pause.task == 0) {
        pause.untilUs = 0;
        return false;
    }
    return true;
}

void TileServerPauses::_end(const QString& service)
{
    const auto pause = _pauses.find(service);
    if (pause == _pauses.end()) {
        return;
    }
    pause->untilUs = 0;
    pause->task = 0;
    qCDebug(TileServerPausesLog) << "the pause of the servers of" << service << "ended," << pause->held.size()
                                 << "requests wait";
    _release(service);
}

void TileServerPauses::_release(const QString& service)
{
    const auto pause = _pauses.find(service);
    if ((pause == _pauses.end()) || !_scheduler) {
        return;
    }
    pause->releaseTask = 0;
    if (pause->untilUs != 0) {
        return;
    }

    while (!pause->held.isEmpty()) {
        const Held next = pause->held.takeFirst();
        // Gone without being withdrawn: it takes no turn
        if (!next.request) {
            continue;
        }
        if (!pause->held.isEmpty()) {
            pause->releaseTask = _scheduler->schedule(this, kReleaseInterval, [this, service] { _release(service); });
        }
        const bool releaseRest = !pause->held.isEmpty() && (pause->releaseTask == 0);
        // The last use of pause: asking can hold or withdraw requests, which changes _pauses
        next.ask();
        if (releaseRest) {
            _releaseAll(service);
        }
        return;
    }
}

void TileServerPauses::_releaseAll(const QString& service)
{
    if (const auto pause = _pauses.find(service); pause != _pauses.end()) {
        qCWarning(TileServerPausesLog) << "the wait for the servers of" << service << "could not be scheduled: the"
                                       << pause->held.size() << "requests that wait ask now";
    }
    // Found again for each request: asking can hold or withdraw requests, or pause the server again
    while (true) {
        const auto pause = _pauses.find(service);
        if ((pause == _pauses.end()) || (pause->untilUs != 0) || pause->held.isEmpty()) {
            return;
        }
        const Held next = pause->held.takeFirst();
        if (next.request) {
            next.ask();
        }
    }
}
