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

#include <QElapsedTimer>
#include <cmath>

#include "MultiVehicleManager.h"
#include "PX4FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "FollowMe.h"
#include "Vehicle.h"

#ifdef QT_QML_DEBUG
FollowMe::simulated_motion_s FollowMe::_simulated_motion[4] = {{0,500},{500,0},{0, -500},{-500, 0}};
#endif

FollowMe::FollowMe(QGCApplication* app)
    : QGCTool(app),
      _followMeStr(PX4FirmwarePlugin::followMeFlightMode)
{

#ifdef QT_QML_DEBUG
    _simulate_motion_timer = 0;
    _simulate_motion_index = 0;
    _simulate_motion = false;
#endif

    memset(&_motionReport, 0, sizeof(motionReport_s));

    // set up the QT position connection slot

    _locationInfo = QGeoPositionInfoSource::createDefaultSource(this);

    if(_locationInfo != 0) {

        _locationInfo->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _locationInfo->setUpdateInterval(_locationInfo->minimumUpdateInterval());
        connect(_locationInfo, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(_setGPSLocation(QGeoPositionInfo)));

        // set up the mavlink motion report timer`

        _gcsMotionReportTimer.setInterval(_locationInfo->minimumUpdateInterval());
        _gcsMotionReportTimer.setSingleShot(false);
        connect(&_gcsMotionReportTimer, &QTimer::timeout, this, &FollowMe::_sendGCSMotionReport);

        runTime.start();
    }
}

FollowMe::~FollowMe()
{
    disable();
}

void FollowMe::followMeHandleManager(const QString&)
{
    QmlObjectListModel & vehicles = *_toolbox->multiVehicleManager()->vehicles();

    for (int i=0; i< vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
        if(vehicle->flightMode().compare(_followMeStr, Qt::CaseInsensitive) == 0) {
            enable();
            return;
        }
    }

    disable();
}

void FollowMe::enable()
{
    if(_locationInfo != 0) {
        _locationInfo->startUpdates();
        _gcsMotionReportTimer.start();
    }
}

void FollowMe::disable()
{
    if(_locationInfo != 0) {
        _locationInfo->stopUpdates();
        _gcsMotionReportTimer.stop();
    }
}

void FollowMe::_setGPSLocation(QGeoPositionInfo geoPositionInfo)
{
    if (geoPositionInfo.isValid())
    {
        // get the current location coordinates

        QGeoCoordinate geoCoordinate = geoPositionInfo.coordinate();

        _motionReport.lat_int = geoCoordinate.latitude()*1e7;
        _motionReport.lon_int = geoCoordinate.longitude()*1e7;
        _motionReport.alt = geoCoordinate.altitude();

#ifdef QT_QML_DEBUG
        if(_simulate_motion == true) {
            _motionReport.lat_int = 47.3977420*1e7;
            _motionReport.lon_int = 8.5455941*1e7;
            _motionReport.alt = 488.00;
        }
#endif
        // get the current eph and epv

        if(geoPositionInfo.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) == true) {
            _motionReport.pos_std_dev[0] = _motionReport.pos_std_dev[1] = geoPositionInfo.attribute(QGeoPositionInfo::HorizontalAccuracy);
        }

        if(geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalAccuracy) == true) {
            _motionReport.pos_std_dev[2] = geoPositionInfo.attribute(QGeoPositionInfo::VerticalAccuracy);
        }

        // calculate z velocity if it's availible

        if(geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalSpeed)) {
            _motionReport.vz = geoPositionInfo.attribute(QGeoPositionInfo::VerticalSpeed);
        }

        // calculate x,y velocity if it's availible

        if((geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction)   == true) &&
           (geoPositionInfo.hasAttribute(QGeoPositionInfo::GroundSpeed) == true)) {

            qreal direction = _degreesToRadian(geoPositionInfo.attribute(QGeoPositionInfo::Direction));
            qreal velocity = geoPositionInfo.attribute(QGeoPositionInfo::GroundSpeed);

            _motionReport.vx = cos(direction)*velocity;
            _motionReport.vy = sin(direction)*velocity;
        }
    }
}

void FollowMe::_sendGCSMotionReport(void)
{
    QmlObjectListModel & vehicles = *_toolbox->multiVehicleManager()->vehicles();
    MAVLinkProtocol* mavlinkProtocol = _toolbox->mavlinkProtocol();
    mavlink_follow_target_t follow_target;

    memset(&follow_target, 0, sizeof(mavlink_follow_target_t));

    follow_target.timestamp = runTime.nsecsElapsed()*1e-6;
    follow_target.est_capabilities = (1 << POS);
    follow_target.position_cov[0] = _motionReport.pos_std_dev[0];
    follow_target.position_cov[2] = _motionReport.pos_std_dev[2];
    follow_target.alt = _motionReport.alt;
    follow_target.lat = _motionReport.lat_int;
    follow_target.lon = _motionReport.lon_int;

#ifdef QT_QML_DEBUG
    if(_simulate_motion == true) {
        _createSimulatedMotion(follow_target);
    }
#endif

    for (int i=0; i< vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
        if(vehicle->flightMode().compare(_followMeStr, Qt::CaseInsensitive) == 0) {
            mavlink_message_t message;
            mavlink_msg_follow_target_encode(mavlinkProtocol->getSystemId(),
                                             mavlinkProtocol->getComponentId(),
                                             &message,
                                             &follow_target);
            vehicle->sendMessage(message);
        }
    }
}

double FollowMe::_degreesToRadian(double deg)
{
    return deg * M_PI / 180.0;
}

#ifdef QT_QML_DEBUG
void FollowMe::_createSimulatedMotion(mavlink_follow_target_t & follow_target)
{
    static int f_lon = 0;
    static int f_lat = 0;
    static float rot = 0;

    rot += (float) .1;

    if(!(_simulate_motion_timer++%50)) {
        _simulate_motion_index++;
        if(_simulate_motion_index > 3) {
            _simulate_motion_index = 0;
        }
    }

    f_lon = f_lon + _simulated_motion[_simulate_motion_index].lon*sin(rot);
    f_lat = f_lat + _simulated_motion[_simulate_motion_index].lat;

    follow_target.alt = _motionReport.alt;
    follow_target.lat = _motionReport.lat_int + f_lon;
    follow_target.lon = _motionReport.lon_int + f_lat;
}
#endif
