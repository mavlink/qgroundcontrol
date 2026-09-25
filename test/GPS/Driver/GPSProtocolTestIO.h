#pragma once

#include <QtCore/QStringList>

#include "GPSProtocol.h"
#include "GPSTestClock.h"

using SurveyInStatus = GPSDecodedSurvey;

template <typename Driver>
class GPSProtocolTestProbe : public Driver
{
public:
    using Driver::Driver;

    const GPSDecodedPosition& workingPosition() const { return this->_position; }

    const GPSDecodedSatellites& workingSatellites() const { return this->_satelliteStorage; }
};

inline GPSProtocolIO captureGPSReports(GPSProtocolIO io, GPSDecodedPosition& position,
                                       GPSDecodedSatellites* satellites = nullptr)
{
    io.decoded = [&position, satellites, sink = std::move(io.decoded)](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSDecodedPosition>(&event)) {
                position = *report;
            } else if (const auto* satellite = std::get_if<GPSDecodedSatellites>(&event); satellite && satellites) {
                *satellites = *satellite;
            }
        }
        if (sink) {
            sink(batch);
        }
    };
    return io;
}

/// Protocol services on @p clock: waits advance it instead of sleeping, and warnings are collected in
/// @p warnings, when given, instead of being logged.
inline GPSProtocolIO makeGPSProtocolTestIO(GPSTestClock& clock, QStringList* warnings = nullptr)
{
    GPSProtocolIO io;
    io.nowUs = [&clock] { return clock.nowUs(); };
    io.wait = [&clock](std::chrono::microseconds delay) {
        clock.advanceBy(delay.count());
        return true;
    };
    io.log = [warnings](const QLoggingCategory&, GPSProtocolLogLevel level, QStringView message) {
        if (warnings && level == GPSProtocolLogLevel::Warning) {
            warnings->push_back(message.toString());
        }
    };
    io.setBaudrate = [](unsigned) { return GPSBaudStatus::Configured; };
    return io;
}
