#pragma once

#include <QtCore/QtGlobal>
#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>

#include "sensor_gps.h"
#include "sensor_gnss_relative.h"
#include "satellite_info.h"

Q_DECLARE_LOGGING_CATEGORY(GPSDriversLog)

#define GPS_INFO(...) qCInfo(GPSDriversLog, __VA_ARGS__)
#define GPS_WARN(...) qCWarning(GPSDriversLog, __VA_ARGS__)
#define GPS_ERR(...) qCCritical(GPSDriversLog, __VA_ARGS__)

#define M_DEG_TO_RAD (M_PI / 180.0)
#define M_RAD_TO_DEG (180.0 / M_PI)
#define M_DEG_TO_RAD_F 0.0174532925f
#define M_RAD_TO_DEG_F 57.2957795f

#define M_PI_2_F 0.63661977f

#ifdef _WIN32
#if (_MSC_VER < 1900)
struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};
#else
#include <time.h>
#endif
#endif

static inline void gps_usleep(unsigned long usecs)
{
    QThread::usleep(usecs);
}

typedef uint64_t gps_abstime;
static inline gps_abstime gps_absolute_time()
{
    return (QDateTime::currentMSecsSinceEpoch() * 1000);
}
