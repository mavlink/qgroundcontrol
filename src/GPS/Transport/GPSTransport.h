#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include <QtCore/QDeadlineTimer>

#include "GPSTransportResult.h"

/// Byte link the GPS driver reads and writes through (serial, TCP, ...).
/// Implemented by the owner of the physical connection and consumed by GPSDriver,
/// keeping the native protocol drivers decoupled from the concrete transport.
/// Construct, use, and destroy on one owner thread. Blocking socket waits dispatch
/// events: callbacks must not reenter, reopen, or destroy the transport during a call.
/// Other threads may request cancellation through requestStop; its lifetime must
/// include all in-flight operations and destruction of the transport.
class GPSTransport
{
public:
    /// The caller owns requestStop and must keep it alive until this transport is destroyed.
    explicit GPSTransport(const std::atomic_bool& requestStop);
    virtual ~GPSTransport();

    virtual GPSOpenResult open() = 0;
    virtual bool fatalError() const = 0;

    bool isCancelled() const { return _requestStop.load(); }

    /// Nonzero when the link cannot follow baud-rate changes (for example, a serial bridge).
    virtual unsigned fixedBaudrate() const { return 0; }

    /// A nonpositive timeout polls immediately available input. Failures never carry usable stream bytes.
    virtual GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) = 0;

    /// Configuration writes preserve the same progress evidence as correction writes.
    virtual GPSWriteResult write(const uint8_t* buffer, int length);
    virtual std::chrono::milliseconds configurationWriteTimeout() const;

    /// Counts describe transport progress, never receiver acknowledgement. Implementations must honor the deadline.
    /// A failed operation that accepted bytes retires the connection; open a new session before writing again.
    /// An unsupported implementation rejects without invoking an unbounded writer.
    virtual GPSWriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline);
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
