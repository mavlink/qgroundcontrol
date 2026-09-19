#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>

#include <QtCore/QStringView>

#include "GPSCommandTransaction.h"
#include "GPSDeadline.h"
#include "GPSDecodedBatch.h"
#include "GPSTransportResult.h"

enum class GPSProtocolLogLevel
{
    Debug,
    Warning,
    Error
};

/// Typed services used by protocol execution. Decoding never invokes device I/O.
struct GPSProtocolIO
{
    /// Borrowed only for the synchronous callback.
    std::function<void(GPSProtocolLogLevel, QStringView)> log;
    /// Borrowed only for the synchronous callback; decode() returns independently owned batches.
    std::function<void(const GPSDecodedBatch&)> decoded;
    std::function<void(const GPSCommandResult&)> commandFinished;
    std::function<GPSReadResult(std::span<uint8_t>, GPSDeadline)> read;
    std::function<GPSWriteResult(std::span<const uint8_t>, GPSDeadline)> write;
    std::function<GPSBaudStatus(unsigned)> setBaudrate;
    std::function<uint64_t()> nowUs;
    std::function<bool(std::chrono::microseconds)> wait;
};
