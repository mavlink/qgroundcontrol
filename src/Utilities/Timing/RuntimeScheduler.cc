#include "RuntimeScheduler.h"

#include <QtCore/QThread>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(RuntimeSchedulerLog, "Utilities.Timing.RuntimeScheduler")

RuntimeScheduler* RuntimeScheduler::orDefault(RuntimeScheduler* scheduler, QObject* owner)
{
    return scheduler ? scheduler : new QtRuntimeScheduler(owner);
}

RuntimeScheduler::RuntimeScheduler(QObject* parent) : QObject(parent)
{
    qCDebug(RuntimeSchedulerLog) << this;
}

RuntimeScheduler::~RuntimeScheduler()
{
    qCDebug(RuntimeSchedulerLog) << this;
}

bool RuntimeScheduler::_isCurrentThread() const
{
    return QThread::currentThread() == thread();
}

bool RuntimeScheduler::_canSchedule(QObject* context, std::chrono::microseconds delay, const Callback& callback) const
{
    return _isCurrentThread() && context && callback && context->thread() == thread() && delay <= MAX_DELAY;
}
