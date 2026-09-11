#include "QtRuntimeScheduler.h"

#include <QtCore/QChronoTimer>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QtRuntimeSchedulerLog, "Utilities.Timing.QtRuntimeScheduler")

QtRuntimeScheduler::QtRuntimeScheduler(QObject* parent)
    : RuntimeScheduler(parent)
{
    qCDebug(QtRuntimeSchedulerLog) << this;
}

QtRuntimeScheduler::~QtRuntimeScheduler()
{
    qCDebug(QtRuntimeSchedulerLog) << this;
}

quint64 QtRuntimeScheduler::nowUs() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

RuntimeScheduler::TaskId QtRuntimeScheduler::schedule(QObject* context, std::chrono::microseconds delay,
                                                      Callback callback)
{
    if (!context || !callback || context->thread() != thread()) {
        return 0;
    }
    const auto id = ++_nextTask;
    auto* timer = new QChronoTimer(this);
    timer->setSingleShot(true);
    timer->setTimerType(Qt::PreciseTimer);
    _tasks.insert(id, timer);
    const QPointer<QObject> guard(context);
    const QPointer<QtRuntimeScheduler> scheduler(this);
    connect(context, &QObject::destroyed, timer, [scheduler, id]() {
        if (scheduler) {
            scheduler->cancel(id);
        }
    });
    connect(timer, &QChronoTimer::timeout, this, [this, id, timer, guard, callback = std::move(callback)]() {
        _tasks.remove(id);
        timer->deleteLater();
        if (guard) {
            callback();
        }
    });
    timer->setInterval((std::max) (delay, std::chrono::microseconds::zero()));
    timer->start();
    return id;
}

void QtRuntimeScheduler::cancel(TaskId task)
{
    const auto timer = _tasks.take(task);
    if (timer) {
        timer->stop();
        timer->disconnect();
        timer->deleteLater();
    }
}
