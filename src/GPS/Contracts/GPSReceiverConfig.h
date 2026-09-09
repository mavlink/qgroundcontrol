#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    bool operator==(const GPSBaseStationConfig&) const = default;
    bool useFixedBase = false;
    double surveyInAccMeters = 0.0;
    int surveyInDurationSecs = 0;
    double fixedBaseLatitude = 0.0;
    double fixedBaseLongitude = 0.0;
    float fixedBaseAltitudeMeters = 0.0f;
    float fixedBaseAccuracyMeters = 0.0f;
};

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
