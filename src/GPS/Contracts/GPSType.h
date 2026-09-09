#pragma once

/// Receiver families QGC can drive via the px4-gpsdrivers library.
enum class GPSType
{
    u_blox = 0,
    trimble = 1,
    septentrio = 2,
    femto = 3,
};
