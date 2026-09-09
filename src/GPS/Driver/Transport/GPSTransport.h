#pragma once

#include <QtCore/QDeadlineTimer>

#include <atomic>
#include <chrono>
#include <cstdint>

#include "GPSTransportResult.h"

/// Byte link the GPS driver reads and writes through (serial, TCP, ...).
/// Implemented by the owner of the physical connection and consumed by GPSDriver,
/// keeping the px4-gpsdrivers library decoupled from the concrete transport.
class GPSTransport
{
public:
    /// The caller owns requestStop and must keep it alive until this transport is destroyed.
    explicit GPSTransport(const std::atomic_bool& requestStop);
    virtual ~GPSTransport();

    using OpenStatus = GPSOpenStatus;
    using OpenResult = GPSOpenResult;
    using ReadStatus = GPSReadStatus;
    using ReadResult = GPSReadResult;
    using WriteStatus = GPSWriteStatus;
    using WriteResult = GPSWriteResult;

    virtual OpenResult open() = 0;
    virtual bool fatalError() const = 0;

    bool isCancelled() const { return _requestStop.load(); }

    /// Nonzero when the link cannot follow baud-rate changes (for example, a serial bridge).
    virtual unsigned fixedBaudrate() const { return 0; }

    /// A nonpositive timeout polls immediately available input. Failures never carry usable stream bytes.
    virtual ReadResult read(uint8_t* buffer, int length, int timeoutMs) = 0;
    /// Configuration writes preserve the same progress evidence as correction writes.
    virtual WriteResult write(const uint8_t* buffer, int length);
    virtual std::chrono::milliseconds configurationWriteTimeout() const;

    /// Counts describe transport progress, never receiver acknowledgement. Implementations must honor the deadline.
    /// An unsupported implementation rejects without invoking an unbounded writer.
    virtual WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline);
    /// Runtime correction allowance; serial links account for the current wire speed.
    virtual std::chrono::milliseconds correctionWriteTimeout(int length) const;
    static std::chrono::milliseconds serialCorrectionWriteTimeout(int length, qint64 baud);

    /// Set the link baud rate. Returns true on success.
    virtual bool setBaudrate(unsigned baudrate) = 0;

protected:
    static constexpr int kCancellationPollMs = 50;

private:
    const std::atomic_bool& _requestStop;
};
