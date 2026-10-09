#include "QtRuntimeScheduler.h"

#include <algorithm>
#include <utility>

#include <QtCore/QTimerEvent>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QtRuntimeSchedulerLog, "Utilities.Timing.QtRuntimeScheduler")

QtRuntimeScheduler::QtRuntimeScheduler(QObject* parent) : RuntimeScheduler(parent)
{
    qCDebug(QtRuntimeSchedulerLog) << this;
}

QtRuntimeScheduler::~QtRuntimeScheduler()
{
    qCDebug(QtRuntimeSchedulerLog) << this;
}

quint64 QtRuntimeScheduler::nowUs() const
{
    return MonotonicClock::nowUs();
}

RuntimeScheduler::TaskId QtRuntimeScheduler::schedule(QObject* context, std::chrono::microseconds delay,
                                                      Callback callback)
{
    if (!_canSchedule(context, delay, callback)) {
        return 0;
    }
    const int timer = startTimer((std::max) (delay, std::chrono::microseconds::zero()), Qt::PreciseTimer);
    if (timer == 0) {
        return 0;
    }
    const auto id = ++_nextTask;
    const auto contextDestroyed = connect(context, &QObject::destroyed, this, [this, id]() { cancel(id); });
    _tasks.insert(timer, {id, context, std::move(callback), contextDestroyed});
    _timers.insert(id, timer);
    return id;
}

void QtRuntimeScheduler::cancel(TaskId task)
{
    if (!_isCurrentThread()) {
        return;
    }
    const auto timer = _timers.find(task);
    if (timer == _timers.end()) {
        return;
    }
    killTimer(*timer);
    disconnect(_tasks.take(*timer).contextDestroyed);
    _timers.erase(timer);
}

void QtRuntimeScheduler::timerEvent(QTimerEvent* event)
{
    const auto found = _tasks.find(event->timerId());
    if (found == _tasks.end()) {
        RuntimeScheduler::timerEvent(event);
        return;
    }
    killTimer(found.key());
    Task task = std::move(*found);
    _tasks.erase(found);
    _timers.remove(task.id);
    disconnect(task.contextDestroyed);
    if (task.context) {
        task.callback();
    }
}
