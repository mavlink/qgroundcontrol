#include "GPSRuntimeScheduler.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRuntimeSchedulerLog, "GPS.Core.GPSRuntimeScheduler")

GPSRuntimeScheduler::GPSRuntimeScheduler(QObject* parent) : QObject(parent)
{
    qCDebug(GPSRuntimeSchedulerLog) << this;
}

GPSRuntimeScheduler::~GPSRuntimeScheduler()
{
    qCDebug(GPSRuntimeSchedulerLog) << this;
}
