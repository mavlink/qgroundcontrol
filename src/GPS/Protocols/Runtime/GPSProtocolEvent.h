#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>

#include "GPSDecodedReports.h"
#include "GPSReceiveUpdates.h"

/// One RTCM3 frame the receiver produced, including its header and CRC.
struct GPSRTCMFrame
{
    QByteArray bytes{};
};

using GPSProtocolEvent = std::variant<GPSDecodedPosition, GPSIntegrityReport, GPSDecodedSatellites,
                                      GPSDecodedSatelliteUsage, GPSDecodedSurvey, GPSRTCMFrame>;

/// Events decoded from one bounded chunk, in publication order.
struct GPSEventBatch
{
    std::vector<GPSProtocolEvent> events{};
    GPSReceiveUpdates updates{};
};
