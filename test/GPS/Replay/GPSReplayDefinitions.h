#pragma once

// Replay the production message ABI with an injected monotonic clock.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

using gps_abstime = uint64_t;
inline gps_abstime gps_test_time = 0;

inline gps_abstime gps_absolute_time()
{
    return gps_test_time;
}

inline void gps_usleep(unsigned long usecs)
{
    gps_test_time += usecs;
}

#define GPS_INFO(...) \
    do {              \
    } while (0)
inline std::vector<std::string> gps_test_warnings;

inline void gps_test_warn(const char* format, ...)
{
    char message[1024]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    gps_test_warnings.emplace_back(message);
}

#define GPS_WARN(...) gps_test_warn(__VA_ARGS__)
#define GPS_ERR(...) \
    do {             \
    } while (0)
#define M_DEG_TO_RAD_F 0.01745329251994329577f
#define M_RAD_TO_DEG 57.2957795130823208768

#include "satellite_info.h"
#include "sensor_gnss_relative.h"
#include "sensor_gps.h"
