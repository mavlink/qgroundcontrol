#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <span>

#include "GPSCommandTransaction.h"
#include "GPSDeadline.h"
#include "GPSProtocolEvent.h"
#include "GPSTransportResult.h"

/// Blocking services the runtime performs for protocol code. Protocol code never calls these directly: it awaits
/// GPSCommandChannel operations, and the runtime executes the request and resumes it.
struct GPSRuntimeIO
{
    std::function<GPSReadResult(std::span<uint8_t>, GPSDeadline)> read{};
    std::function<GPSWriteResult(std::span<const uint8_t>, GPSDeadline)> write{};
    std::function<GPSBaudStatus(unsigned)> setBaudrate{};
    /// Defaults to MonotonicClock::nowUs(); tests inject a virtual clock.
    std::function<uint64_t()> nowUs{};
    /// @return false when cancelled. Defaults to sleeping in short slices until the duration elapses or
    /// isCancelled() reports a stop.
    std::function<bool(std::chrono::microseconds)> wait{};
    std::function<bool()> isCancelled{};
};

/// Observers of runtime output, invoked synchronously on the runtime thread.
struct GPSRuntimeObserver
{
    /// Borrowed for the callback; it may decode nested input through GPSProtocolRuntime::consume().
    std::function<void(const GPSEventBatch&)> decoded{};
    /// Every completed command, during configuration and streaming alike.
    std::function<void(const GPSCommandResult&)> commandFinished{};
};

/// One blocking operation a suspended coroutine waits for. It lives in the awaiting frame.
struct GPSIORequest
{
    enum class Kind : uint8_t
    {
        Read,
        Write,
        SetBaudrate,
        Wait,
    };

    Kind kind = Kind::Read;
    std::span<uint8_t> readBuffer{};
    std::span<const uint8_t> writeBytes{};
    GPSDeadline deadline{};
    unsigned baudrate = 0;
    std::chrono::microseconds duration{0};

    GPSReadResult readResult{};
    GPSWriteResult writeResult{};
    GPSBaudStatus baudStatus = GPSBaudStatus::Unsupported;
    bool waited = true;
};
