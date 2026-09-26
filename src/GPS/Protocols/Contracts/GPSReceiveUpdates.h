#pragma once

#include <cstdint>

#include <QtCore/QFlags>

/// What a decode or receive cycle produced. Position and satellite updates are navigation data; activity is protocol
/// traffic that proves the receiver is talking, such as acknowledgements, diagnostics and corrections.
enum class GPSReceiveUpdate : uint8_t
{
    Position = 0x1,
    Satellites = 0x2,
    Activity = 0x4,
};
Q_DECLARE_FLAGS(GPSReceiveUpdates, GPSReceiveUpdate)
Q_DECLARE_OPERATORS_FOR_FLAGS(GPSReceiveUpdates)
