#pragma once

/// Lightweight header providing QGCMAVLink type aliases and constants
/// without pulling in the full MAVLink dialect definitions (~118k preprocessed lines).
///
/// Use this instead of QGCMAVLink.h in headers that only reference
/// FirmwareClass_t, VehicleClass_t, or maxRcChannels.

#include <cstdint>

struct QGCMAVLinkTypes {
    typedef int FirmwareClass_t;
    typedef int VehicleClass_t;

    static constexpr VehicleClass_t VehicleClassGeneric = 0; // Must match MAV_TYPE_GENERIC

    static constexpr uint8_t maxRcChannels = 18;
};
