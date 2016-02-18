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

#include <cmath>

#include "MultiVehicleManager.h"
#include "MAVLinkProtocol.h"
#include "FollowMe.h"
#include "Vehicle.h"

FollowMe::FollowMe(QGCApplication* app)
    : QGCTool(app)
    , _followMeStr("Auto: Follow Me")
{
    // set up the QT position connection slot

    _locationInfo = QGeoPositionInfoSource::createDefaultSource(this);
    _locationInfo->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
    connect(_locationInfo, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(_setGPSLocation(QGeoPositionInfo)));

    // set up the motion mavlink report timer

    _gcsMotionReportTimer.setInterval(_gcsMotionReportRateMSecs);
    _gcsMotionReportTimer.setSingleShot(false);
    connect(&_gcsMotionReportTimer, &QTimer::timeout, this, &FollowMe::_sendGCSMotionReport);

    // change this to a setting?

    enable();
}

FollowMe::~FollowMe()
{
    disable();
}

void FollowMe::enable()
{
    _locationInfo->startUpdates();
    _gcsMotionReportTimer.start();
}

void FollowMe::disable()
{
    _locationInfo->stopUpdates();
    _gcsMotionReportTimer.stop();
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


            qWarning("vel = x%f, y%f\n", _motionReport.vx, _motionReport.vy);
        }

        qWarning("lat %d, lon %d alt %d\n", _motionReport.lat_int, _motionReport.lon_int, _motionReport.alt);
    }
}

void FollowMe::_sendGCSMotionReport(void)
{
    QmlObjectListModel & vehicles = *_toolbox->multiVehicleManager()->vehicles();
    MAVLinkProtocol* mavlinkProtocol = _toolbox->mavlinkProtocol();
    mavlink_follow_target_t follow_target = {};

    follow_target.alt = _motionReport.alt;
    follow_target.lat = _motionReport.lat_int;
    follow_target.lon = _motionReport.lon_int;
    follow_target.vel[0] = _motionReport.vx;
    follow_target.vel[1] = _motionReport.vy;
    follow_target.vel[2] = _motionReport.vz;

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
