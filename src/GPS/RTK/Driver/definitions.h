#pragma once

#include <chrono>
#include <cstdint>
#include <time.h> // IWYU pragma: keep — part of the GPS_DEFINITIONS_HEADER contract for px4 sources

#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtCore/QtMath>

#include "sensor_gps.h"          // IWYU pragma: keep — px4 sources resolve sensor_gps_s through this header
#include "sensor_gnss_relative.h" // IWYU pragma: keep — px4 sources resolve sensor_gnss_relative_s through this header
#include "satellite_info.h"      // IWYU pragma: keep — px4 sources resolve satellite_info_s through this header

// Symbols below satisfy the px4-gpsdrivers GPS_DEFINITIONS_HEADER contract.

Q_DECLARE_LOGGING_CATEGORY(GPSDriversLog)

#define GPS_INFO(...) qCInfo(GPSDriversLog, __VA_ARGS__)
#define GPS_WARN(...) qCWarning(GPSDriversLog, __VA_ARGS__)
#define GPS_ERR(...)  qCCritical(GPSDriversLog, __VA_ARGS__)

#define M_DEG_TO_RAD_F static_cast<float>(M_PI / 180.0)
#define M_RAD_TO_DEG   (180.0 / M_PI)

typedef uint64_t gps_abstime;

static inline gps_abstime gps_absolute_time()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static inline void gps_usleep(unsigned long usecs)
{
    QThread::usleep(usecs);
}
