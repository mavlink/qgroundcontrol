/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

Q_DECLARE_LOGGING_CATEGORY(FollowMeLog)

class FollowMe : public QGCTool
{
    Q_OBJECT

public:
    FollowMe(QGCApplication* app, QGCToolbox* toolbox);

    void setToolbox(QGCToolbox* toolbox) override;

public slots:
    void followMeHandleManager  (const QString&);

private slots:
    void _setGPSLocation        (QGeoPositionInfo geoPositionInfo);
    void _sendGCSMotionReport   ();
    void _settingsChanged       ();

private:
    QElapsedTimer runTime;    
    QTimer _gcsMotionReportTimer;   // Timer to emit motion reports

    struct motionReport_s {
        uint32_t timestamp;     // time since boot
        int32_t lat_int;        // X Position in WGS84 frame in 1e7 * meters
        int32_t lon_int;        // Y Position in WGS84 frame in 1e7 * meters
        float alt;              //	Altitude in meters in AMSL altitude, not WGS84 if absolute or relative, above terrain if GLOBAL_TERRAIN_ALT_INT
        float vx;               //	X velocity in NED frame in meter / s
        float vy;               //	Y velocity in NED frame in meter / s
        float vz;               //	Z velocity in NED frame in meter / s
        float afx;              //	X acceleration in NED frame in meter / s^2 or N
        float afy;              //	Y acceleration in NED frame in meter / s^2 or N
        float afz;              //	Z acceleration in NED frame in meter / s^2 or N
        float pos_std_dev[3];   // -1 for unknown
    } _motionReport;

    // Mavlink defined motion reporting capabilities

    enum {
        POS = 0,
        VEL = 1,
        ACCEL = 2,
        ATT_RATES = 3
    };

    uint8_t estimatation_capabilities;

    void    _disable    ();
    void    _enable     ();

    double  _degreesToRadian(double deg);

    enum {
        MODE_NEVER,
        MODE_ALWAYS,
        MODE_FOLLOWME
    };

    uint32_t _currentMode;
};
