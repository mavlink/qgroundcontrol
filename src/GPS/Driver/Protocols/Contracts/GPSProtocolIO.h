#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>

#include <QtCore/QStringView>

class QLoggingCategory;

#include "GPSCommandTransaction.h"
#include "GPSDeadline.h"
#include "GPSDecodedBatch.h"
#include "GPSTransportResult.h"

/// Sticky failure of the current receiver session; I/O and control stop until the next configure().
enum class GPSProtocolError : uint8_t
{
    None,
    Cancelled,        ///< A stop was requested; not a receiver or link fault.
    Transport,        ///< The link could not read, write, or change its baud rate.
    Protocol,         ///< The receiver violated or rejected the control protocol.
    InvalidArgument,  ///< The driver requested an invalid write.
};

enum class GPSProtocolLogLevel
{
    Debug,
    Warning,
    Error
};

/// Typed services used by protocol execution. Decoding never invokes device I/O.
struct GPSProtocolIO
{
    /// Borrowed only for the synchronous callback. The category names the emitting receiver family.
    std::function<void(const QLoggingCategory&, GPSProtocolLogLevel, QStringView)> log;
    /// Borrowed only for the synchronous callback; decode() returns independently owned batches.
    std::function<void(const GPSDecodedBatch&)> decoded;
    std::function<void(const GPSCommandResult&)> commandFinished;
    std::function<GPSReadResult(std::span<uint8_t>, GPSDeadline)> read;
    std::function<GPSWriteResult(std::span<const uint8_t>, GPSDeadline)> write;
    std::function<GPSBaudStatus(unsigned)> setBaudrate;
    std::function<uint64_t()> nowUs;
    std::function<bool(std::chrono::microseconds)> wait;
};
