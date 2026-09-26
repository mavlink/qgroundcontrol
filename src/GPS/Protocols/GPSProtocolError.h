#pragma once

#include <cstdint>

/// Sticky failure of the current receiver session; I/O and control stop until the next configure().
enum class GPSProtocolError : uint8_t
{
    None,
    Cancelled,        ///< A stop was requested; not a receiver or link fault.
    Transport,        ///< The link could not read, write, or change its baud rate.
    Protocol,         ///< The receiver violated or rejected the control protocol.
    InvalidArgument,  ///< The driver requested an invalid write.
    ConsentRequired,  ///< Configuration needs a persistent receiver change the caller did not allow.
};
