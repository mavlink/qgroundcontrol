#pragma once

/// Receiver families QGC can drive using the native protocol drivers.
enum class GPSType
{
    u_blox = 0,
    trimble = 1,
    septentrio = 2,
    femto = 3,
};
