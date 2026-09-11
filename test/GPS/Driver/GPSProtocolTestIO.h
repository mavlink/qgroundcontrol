#pragma once
#include <thread>

#include "GPSProtocol.h"
#ifdef QGC_GPS_TEST_CLOCK
inline uint64_t gps_test_time = 0;
inline std::vector<std::string> gps_test_warnings;
#else
#include <QtCore/QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(GPSDriversLog)
#endif
using SurveyInStatus = GPSSurveyReport;

inline GPSProtocolIO makeGPSProtocolTestIO()
{
    GPSProtocolIO io;
#ifdef QGC_GPS_TEST_CLOCK
    io.nowUs = [] { return gps_test_time; };
    io.wait = [](std::chrono::microseconds delay) {
        gps_test_time += delay.count();
        return true;
    };
    io.log = [](GPSProtocolLogLevel level, std::string_view message) {
        if (level == GPSProtocolLogLevel::Warning)
            gps_test_warnings.emplace_back(message);
    };
#else
    io.nowUs = [] {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    };
    io.wait = [](std::chrono::microseconds delay) {
        std::this_thread::sleep_for(delay);
        return true;
    };
    io.log = [](GPSProtocolLogLevel level, std::string_view message) {
        if (level == GPSProtocolLogLevel::Warning)
            qCWarning(GPSDriversLog, "%.*s", int(message.size()), message.data());
    };
#endif
    io.setBaudrate = [](unsigned) { return GPSBaudStatus::Configured; };
    return io;
}
