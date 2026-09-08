#pragma once

#include <atomic>
#include <cstdint>

/// Byte link the GPS driver reads and writes through (serial, TCP, ...).
/// Implemented by the owner of the physical connection and consumed by GPSDriver,
/// keeping the px4-gpsdrivers library decoupled from the concrete transport.
class GPSTransport
{
public:
    /// The caller owns requestStop and must keep it alive until this transport is destroyed.
    explicit GPSTransport(const std::atomic_bool& requestStop);
    virtual ~GPSTransport();

    virtual bool open() = 0;
    virtual bool fatalError() const = 0;

    bool isCancelled() const { return _requestStop.load(); }

    /// Nonzero when the link cannot follow baud-rate changes (for example, a serial bridge).
    virtual unsigned fixedBaudrate() const { return 0; }

    /// Read up to length bytes into buffer, waiting up to timeoutMs.
    /// Returns bytes read, 0 on timeout, <0 on error.
    virtual int read(uint8_t *buffer, int length, int timeoutMs) = 0;

    /// Write length bytes. Returns bytes written, or -1 on error.
    virtual int write(const uint8_t *buffer, int length) = 0;

    /// Set the link baud rate. Returns true on success.
    virtual bool setBaudrate(unsigned baudrate) = 0;

private:
    const std::atomic_bool& _requestStop;
};
