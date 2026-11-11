#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

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
        double altMetersAMSL;   // Altitude above mean sea level in meters; NaN when the ground station's is not known
        double headingDegrees;  // Heading in degrees
        double vxMetersPerSec;  // X velocity in NED frame in meter / s
        double vyMetersPerSec;  // Y velocity in NED frame in meter / s
        double vzMetersPerSec;  // Z velocity in NED frame in meter / s
        double pos_std_dev[3];
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
