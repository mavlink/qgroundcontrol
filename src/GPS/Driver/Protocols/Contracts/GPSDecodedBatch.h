#pragma once

#include <array>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "GPSDriverReports.h"
#include "GPSNativePositionReport.h"
#include "GPSNativeSatelliteReport.h"
#include "GPSNativeSurveyReport.h"

struct GPSRTCMReport
{
    std::array<uint8_t, 1029> bytes{};
    size_t size = 0;
};

using GPSDecodedEvent = std::variant<GPSNativePositionReport, GPSIntegrityReport, GPSNativeSatelliteReport,
                                     GPSSatelliteUsageReport, GPSNativeSurveyReport, GPSRTCMReport>;

/// A bounded, owned sequence. The caller decides whether position epochs may be coalesced.
struct GPSDecodedBatch
{
    static constexpr size_t MAX_EVENTS = 8;
    static constexpr int POSITION_UPDATE = 1;
    static constexpr int SATELLITES_UPDATE = 2;
    static constexpr int PROTOCOL_ACTIVITY = 4;
    std::vector<GPSDecodedEvent> events;
    int updates = 0;
};

struct GPSDecodeResult
{
    size_t bytesConsumed = 0;
    GPSDecodedBatch batch;
};
