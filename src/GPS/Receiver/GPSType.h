#pragma once

#include <QtCore/QMetaType>

namespace GPSTypes {
Q_NAMESPACE

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
Q_ENUM_NS(GPSType)

}  // namespace GPSTypes

using GPSType = GPSTypes::GPSType;
