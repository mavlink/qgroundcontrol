/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "MultiVehicleManager.h"
#include "FollowMe.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"

#include <QSettings>

FollowMe::FollowMe(QGCApplication* app)
    : QGCTool(app)
    , _motionReport({})
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
}

FollowMe::~FollowMe()
{
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
