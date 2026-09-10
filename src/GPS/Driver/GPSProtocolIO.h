#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>

#include "GPSCommandTransaction.h"
#include "GPSDeadline.h"
#include "GPSDecodedBatch.h"
#include "GPSIOStatus.h"

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
    std::function<void(GPSDecodedBatch)> decoded;
    std::function<void(const GPSCommandResult&)> commandFinished;
    std::function<GPSProtocolReadResult(std::span<uint8_t>, GPSDeadline)> read;
    std::function<GPSProtocolWriteResult(std::span<const uint8_t>, GPSDeadline)> write;
    std::function<GPSBaudStatus(unsigned)> setBaudrate;
    std::function<uint64_t()> nowUs;
    std::function<bool(std::chrono::microseconds)> wait;
};
