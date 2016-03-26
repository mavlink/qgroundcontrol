/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#pragma once

#include <QTimer>
#include <QObject>
#include <QThread>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QElapsedTimer>

#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "MAVLinkProtocol.h"

Q_DECLARE_LOGGING_CATEGORY(FollowMeLog)

class FollowMe : public QGCTool
{
    Q_OBJECT

public:
    FollowMe(QGCApplication* app);
    ~FollowMe();

public slots:
    void followMeHandleManager(const QString&);

private slots:
    void _setGPSLocation(QGeoPositionInfo geoPositionInfo);
    void _sendGCSMotionReport(void);

private:
    QGeoPositionInfoSource * _locationInfo;
    QElapsedTimer runTime;

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

    QString _followMeStr;

    QTimer _gcsMotionReportTimer;        ///< Timer to emit motion reports
    double _degreesToRadian(double deg);

    void disable();
    void enable();

    // Mavlink defined motion reporting capabilities

    enum {
        POS = 0,
        VEL = 1,
        ACCEL = 2,
        ATT_RATES = 3
    };

#ifdef QT_QML_DEBUG

    // items for simulating QGC movment in jMAVSIM

    struct simulated_motion_s {
        int lon;
        int lat;
    };

    static simulated_motion_s _simulated_motion[4];

    int _simulate_motion_timer;
    int _simulate_motion_index;

    bool _simulate_motion;

    void _createSimulatedMotion(mavlink_follow_target_t & follow_target);
#endif
};
