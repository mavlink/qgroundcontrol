#pragma once
#include <thread>

#include <QtCore/QStringList>

#include "GPSProtocol.h"
#ifdef QGC_GPS_TEST_CLOCK
inline uint64_t gps_test_time = 0;
inline QStringList gps_test_warnings;
#else
#include <QtCore/QLoggingCategory>
#endif
using SurveyInStatus = GPSNativeSurveyReport;

template <typename Driver>
class GPSProtocolTestProbe : public Driver
{
public:
    using Driver::Driver;

    const GPSNativePositionReport& workingPosition() const { return this->_position; }

    const GPSNativeSatelliteReport& workingSatellites() const { return this->_satelliteStorage; }
};

inline GPSProtocolIO captureGPSReports(GPSProtocolIO io, GPSNativePositionReport& position,
                                       GPSNativeSatelliteReport* satellites = nullptr)
{
    io.decoded = [&position, satellites, sink = std::move(io.decoded)](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSNativePositionReport>(&event)) {
                position = *report;
            } else if (const auto* satellite = std::get_if<GPSNativeSatelliteReport>(&event); satellite && satellites) {
                *satellites = *satellite;
            }
        }
        if (sink) {
            sink(batch);
        }
    };
    return io;
}

inline GPSProtocolIO makeGPSProtocolTestIO()
{
    GPSProtocolIO io;
#ifdef QGC_GPS_TEST_CLOCK
    io.nowUs = [] { return gps_test_time; };
    io.wait = [](std::chrono::microseconds delay) {
        gps_test_time += delay.count();
        return true;
    };
    io.log = [](const QLoggingCategory&, GPSProtocolLogLevel level, QStringView message) {
        if (level == GPSProtocolLogLevel::Warning) {
            gps_test_warnings.push_back(message.toString());
        }
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
    io.log = [](const QLoggingCategory& category, GPSProtocolLogLevel level, QStringView message) {
        if (level == GPSProtocolLogLevel::Warning) {
            QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).warning(category) << message;
        }
    };
#endif
    io.setBaudrate = [](unsigned) { return GPSBaudStatus::Configured; };
    return io;
}
