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
    , _followMeStr("follow me")
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

        // calculate velocity if it's availible

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

    for (int i=0; i< vehicles.count(); i++) {

        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);

        if(vehicle->flightMode().compare(_followMeStr, Qt::CaseInsensitive) == 0) {
            mavlink_message_t message;

            vehicle->sendMessage(message);
        }
    }
}

double FollowMe::_degreesToRadian(double deg)
{
    return deg * M_PI / 180.0;
}
