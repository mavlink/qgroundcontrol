#pragma once

#include <QtCore/QMetaType>

namespace GPSTypes {
Q_NAMESPACE

enum class GPSType
{
    UBlox = 0,
    Trimble = 1,
    Septentrio = 2,
    Femto = 3,
    Unicore = 4,
    Quectel = 5,
    Passive = 6,
    /// No receiver's own type: a configured base whose family is detected on every connect. Stays last: every value
    /// before it needs a receiver descriptor (GPSReceiverDescriptor.cc checks this).
    Automatic = 7,
};
Q_ENUM_NS(GPSType)

}  // namespace GPSTypes

using GPSType = GPSTypes::GPSType;
