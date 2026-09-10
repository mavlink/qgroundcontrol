#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>

#include "GPSIOStatus.h"
#include "GPSRelativeReport.h"

struct GPSSurveyReport
{
    double latitude = 0;
    double longitude = 0;
    float altitude = 0;
    uint32_t mean_accuracy = 0;
    uint32_t duration = 0;
    uint8_t flags = 0;
};

struct GPSProtocolDeadline
{
    uint64_t untilUs = UINT64_MAX;

    int remainingMilliseconds(uint64_t nowUs) const
    {
        return nowUs >= untilUs ? 0
                                : static_cast<int>(std::min<uint64_t>(
                                      (untilUs - nowUs) / 1000 + ((untilUs - nowUs) % 1000 != 0), INT32_MAX));
    }
};

struct GPSProtocolReadResult
{
    GPSReadStatus status = GPSReadStatus::TimedOut;
    int bytesRead = 0;
};

struct GPSProtocolWriteResult
{
    GPSWriteStatus status = GPSWriteStatus::Unsupported;
    int acceptedBytes = 0;
    int writtenBytes = 0;
    int uncertainBytes = 0;
};

/// Typed services used by protocol execution. Decoding never invokes device I/O.
struct GPSProtocolIO
{
    std::function<GPSProtocolReadResult(std::span<uint8_t>, GPSProtocolDeadline)> read;
    std::function<GPSProtocolWriteResult(std::span<const uint8_t>, GPSProtocolDeadline)> write;
    std::function<GPSBaudStatus(unsigned)> setBaudrate;
    std::function<void(std::span<const uint8_t>)> rtcm;
    std::function<void(const GPSRelativeReport&)> relativePosition;
    std::function<void(const GPSSurveyReport&)> survey;
    std::function<uint64_t()> nowUs;
    std::function<bool(std::chrono::microseconds)> wait;
};
