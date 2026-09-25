#pragma once

#include <chrono>
#include <cstdint>
#include <stop_token>

#include <QtCore/QDeadlineTimer>

#include "GPSTransportResult.h"

/// Byte link the GPS driver reads and writes through.
/// Implemented by the owner of the physical connection and consumed by GPSDriver,
/// keeping the native protocol drivers decoupled from the concrete transport.
/// Construct, use, and destroy on one owner thread. Blocking socket waits dispatch
/// events: callbacks must not reenter, reopen, or destroy the transport during a call.
/// Other threads cancel through the std::stop_source that issued the construction token.
class GPSTransport
{
public:
    explicit GPSTransport(std::stop_token stopToken);
    virtual ~GPSTransport();

    virtual GPSOpenResult open() = 0;
    virtual bool fatalError() const = 0;

    bool isCancelled() const { return _stopToken.stop_requested(); }

    const std::stop_token& stopToken() const { return _stopToken; }

    /// Nonzero when the link cannot follow baud-rate changes (for example, a serial bridge).
    virtual unsigned fixedBaudrate() const { return 0; }

    /// Line rate that TCP and UDP links report. Drivers still program the receiver's serial port with it (UBX CFG-PRT
    /// sets UART1 to it), so the serial side of a network bridge and its receiver must run at this rate.
    static constexpr unsigned BRIDGE_BAUDRATE = 115200;

    /// A nonpositive timeout polls immediately available input. Failures never carry usable stream bytes.
    virtual GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) = 0;

    virtual std::chrono::milliseconds configurationWriteTimeout() const;

    /// Receiver configuration write under the command deadline, capped by configurationWriteTimeout().
    /// Counts describe transport progress, never receiver acknowledgement.
    /// A failed operation that accepted bytes retires the connection; open a new session before writing again.
    GPSWriteResult write(const uint8_t* buffer, int length, QDeadlineTimer deadline);
    /// Set the link baud rate. Returns true on success.
    virtual bool setBaudrate(unsigned baudrate) = 0;

protected:
    /// Receives a non-empty valid buffer and an unexpired, capped deadline that implementations must honor.
    virtual GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) = 0;

private:
    std::stop_token _stopToken;
};
