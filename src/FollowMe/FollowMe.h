/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

Q_DECLARE_LOGGING_CATEGORY(FollowMeLog)

class Vehicle;

class FollowMe : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("Vehicle.h")

public:
    explicit FollowMe(QObject *parent = nullptr);
    ~FollowMe();

    static FollowMe* instance();
    void init();

    struct GCSMotionReport {
        int lat_int;            // X Position in WGS84 frame in 1e7 * meters
        int lon_int;            // Y Position in WGS84 frame in 1e7 * meters
        double altMetersAMSL;   // Altitude in meters in AMSL altitude, not WGS84 if absolute or relative, above terrain if GLOBAL_TERRAIN_ALT_INT
        double headingDegrees;  // Heading in degrees
        double vxMetersPerSec;  // X velocity in NED frame in meter / s
        double vyMetersPerSec;  // Y velocity in NED frame in meter / s
        double vzMetersPerSec;  // Z velocity in NED frame in meter / s
        double pos_std_dev[3];  // -1 for unknown
    };

    /// Mavlink defined motion reporting capabilities
    enum MotionCapability {
        POS = 0,
        VEL = 1,
        ACCEL = 2,
        ATT_RATES = 3,
        HEADING = 4
    };

private slots:
    void _sendGCSMotionReport();
    void _settingsChanged(QVariant value);
    void _vehicleAdded(Vehicle *vehicle);
    void _vehicleRemoved(Vehicle *vehicle);
    void _enableIfVehicleInFollow();

private:
    enum FollowMode {
        MODE_NEVER,
        MODE_ALWAYS,
        MODE_FOLLOWME
    };

    void _disableFollowSend();
    void _enableFollowSend();
    bool _isFollowFlightMode(const Vehicle *vehicle, const QString &flightMode);

    QTimer *_gcsMotionReportTimer = nullptr;
    FollowMode _currentMode = MODE_NEVER;

    static constexpr int kMotionUpdateInterval = 250;
};
