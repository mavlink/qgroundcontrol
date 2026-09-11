#include "RuntimeScheduler.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RuntimeSchedulerLog, "Utilities.Timing.RuntimeScheduler")

RuntimeScheduler::RuntimeScheduler(QObject* parent)
    : QObject(parent)
{
    qCDebug(RuntimeSchedulerLog) << this;
}

RuntimeScheduler::~RuntimeScheduler()
{
    qCDebug(RuntimeSchedulerLog) << this;
}
