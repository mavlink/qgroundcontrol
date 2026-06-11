#pragma once

#include <cstdint>

/// Byte link the GPS driver reads and writes through (serial, TCP, ...).
/// Implemented by the owner of the physical connection and consumed by GPSDriver,
/// keeping the px4-gpsdrivers library decoupled from the concrete transport.
class GPSTransport
{
public:
    virtual ~GPSTransport() = default;

    /// Read up to length bytes into buffer, waiting up to timeoutMs.
    /// Returns bytes read, 0 on timeout, <0 on error.
    virtual int read(uint8_t *buffer, int length, int timeoutMs) = 0;

    /// Write length bytes. Returns bytes written, or -1 on error.
    virtual int write(const uint8_t *buffer, int length) = 0;

    /// Set the link baud rate. Returns true on success.
    virtual bool setBaudrate(unsigned baudrate) = 0;
};
