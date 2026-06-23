#pragma once

/// Receiver families QGC can drive via the px4-gpsdrivers library.
enum class GPSType
{
    u_blox,
    trimble,
    septentrio,
    femto
};
