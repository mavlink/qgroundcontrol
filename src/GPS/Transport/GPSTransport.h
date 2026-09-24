#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include <QtCore/QDeadlineTimer>

#include "GPSTransportResult.h"

/// Byte link the GPS driver reads and writes through.
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

    virtual std::chrono::milliseconds configurationWriteTimeout() const;

    /// Receiver configuration write under the command deadline, capped by configurationWriteTimeout().
    /// Counts describe transport progress, never receiver acknowledgement.
    /// A failed operation that accepted bytes retires the connection; open a new session before writing again.
    GPSWriteResult write(const uint8_t* buffer, int length, QDeadlineTimer deadline);
    /// Set the link baud rate. Returns true on success.
    virtual bool setBaudrate(unsigned baudrate) = 0;

protected:
    static constexpr int kCancellationPollMs = 50;

    /// Receives a non-empty valid buffer and an unexpired, capped deadline that implementations must honor.
    virtual GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) = 0;

private:
    const std::atomic_bool& _requestStop;
};
