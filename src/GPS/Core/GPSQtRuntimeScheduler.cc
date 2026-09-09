#include "GPSQtRuntimeScheduler.h"

#include <QtCore/QChronoTimer>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSQtRuntimeSchedulerLog, "GPS.Core.GPSQtRuntimeScheduler")

GPSQtRuntimeScheduler::GPSQtRuntimeScheduler(QObject* parent) : GPSRuntimeScheduler(parent)
{
    qCDebug(GPSQtRuntimeSchedulerLog) << this;
}

GPSQtRuntimeScheduler::~GPSQtRuntimeScheduler()
{
    qCDebug(GPSQtRuntimeSchedulerLog) << this;
}

quint64 GPSQtRuntimeScheduler::nowUs() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

GPSRuntimeScheduler::TaskId GPSQtRuntimeScheduler::schedule(QObject* context, std::chrono::microseconds delay,
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
    const QPointer<GPSQtRuntimeScheduler> scheduler(this);
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

void GPSQtRuntimeScheduler::cancel(TaskId task)
{
    const auto timer = _tasks.take(task);
    if (timer) {
        timer->stop();
        timer->disconnect();
        timer->deleteLater();
    }
}
