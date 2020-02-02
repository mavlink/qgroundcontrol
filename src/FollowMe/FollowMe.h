/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QTimer>
#include <QObject>
#include <QThread>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QElapsedTimer>

#include "QGCToolbox.h"
#include "MAVLinkProtocol.h"

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(FollowMeLog)

class FollowMe : public QGCTool
{
    Q_OBJECT

public:
    FollowMe(QGCApplication* app, QGCToolbox* toolbox);

    struct GCSMotionReport {
        int     lat_int;            // X Position in WGS84 frame in 1e7 * meters
        int     lon_int;            // Y Position in WGS84 frame in 1e7 * meters
        double  altMetersAMSL;      //	Altitude in meters in AMSL altitude, not WGS84 if absolute or relative, above terrain if GLOBAL_TERRAIN_ALT_INT
        double  headingDegrees;      // Heading in degrees
        double  vxMetersPerSec;     //	X velocity in NED frame in meter / s
        double  vyMetersPerSec;     //	Y velocity in NED frame in meter / s
        double  vzMetersPerSec;     //	Z velocity in NED frame in meter / s
        double  pos_std_dev[3];     // -1 for unknown
    };

    // Mavlink defined motion reporting capabilities
    enum {
        POS = 0,
        VEL = 1,
        ACCEL = 2,
        ATT_RATES = 3,
        HEADING = 4
    };

    void setToolbox(QGCToolbox* toolbox) override;

private slots:
    void _sendGCSMotionReport       (void);
    void _settingsChanged           (void);
    void _vehicleAdded              (Vehicle* vehicle);
    void _vehicleRemoved            (Vehicle* vehicle);
    void _enableIfVehicleInFollow   (void);

private:
    enum {
        MODE_NEVER,
        MODE_ALWAYS,
        MODE_FOLLOWME
    };

    void    _disableFollowSend  (void);
    void    _enableFollowSend   (void);
    double  _degreesToRadian    (double deg);
    bool    _isFollowFlightMode (Vehicle* vehicle, const QString& flightMode);

    QTimer      _gcsMotionReportTimer;
    uint32_t    _currentMode;
};
