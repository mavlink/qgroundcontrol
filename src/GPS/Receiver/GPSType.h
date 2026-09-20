#pragma once

/// Values are persisted; do not renumber.
enum class GPSType
{
    ublox = 0,
    trimble = 1,
    septentrio = 2,
    femto = 3,
    unicore = 4,
    quectel = 5,
    passive = 6,
};
