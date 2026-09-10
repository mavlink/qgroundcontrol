#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "GPSBaseStationConfig.h"

/// Receiver configuration, decoupled from QGC settings types.
struct GPSReceiverConfig
{
    enum class Role
    {
        RTKBase = 0,
        Position = 1
    };

    enum class OutputProtocol
    {
        Native,
        NMEA
    };

    Role role = Role::RTKBase;
    OutputProtocol outputProtocol = OutputProtocol::Native;
    GPSBaseStationConfig base;
    int constellationMask = 0;
    int dynamicModel = 0;
    int outputRateHz = 0;
    QString validationError() const;
    bool operator==(const GPSReceiverConfig&) const = default;

    float headingOffsetDeg = 5.0f;  // dual-antenna heading offset; consumed only by the Septentrio (SBF) driver
};

Q_DECLARE_METATYPE(GPSReceiverConfig)
