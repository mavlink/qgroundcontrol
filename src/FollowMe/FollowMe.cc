/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QElapsedTimer>
#include <cmath>

#include "MultiVehicleManager.h"
#include "PX4FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "FollowMe.h"
#include "Vehicle.h"
#include "PositionManager.h"

FollowMe::FollowMe(QGCApplication* app)
    : QGCTool(app), estimatation_capabilities(0)
{
    memset(&_motionReport, 0, sizeof(motionReport_s));
    runTime.start();

    _gcsMotionReportTimer.setSingleShot(false);
    connect(&_gcsMotionReportTimer, &QTimer::timeout, this, &FollowMe::_sendGCSMotionReport);
}

FollowMe::~FollowMe()
{
    _disable();
}

void FollowMe::followMeHandleManager(const QString&)
{
    QmlObjectListModel & vehicles = *_toolbox->multiVehicleManager()->vehicles();

    for (int i=0; i< vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
        if(vehicle->flightMode().compare(PX4FirmwarePlugin::followMeFlightMode, Qt::CaseInsensitive) == 0) {
            _enable();
            return;
        }
    }

    _disable();
}

void FollowMe::_enable()
{
    connect(_toolbox->qgcPositionManager(),
            SIGNAL(positionInfoUpdated(QGeoPositionInfo)),
            this,
            SLOT(_setGPSLocation(QGeoPositionInfo)));

    _gcsMotionReportTimer.setInterval(_toolbox->qgcPositionManager()->updateInterval());
    _gcsMotionReportTimer.start();
}

void FollowMe::_disable()
{
    disconnect(_toolbox->qgcPositionManager(),
               SIGNAL(positionInfoUpdated(QGeoPositionInfo)),
               this,
               SLOT(_setGPSLocation(QGeoPositionInfo)));

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

        estimatation_capabilities |= (1 << POS);

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

            estimatation_capabilities |= (1 << VEL);

            qreal direction = _degreesToRadian(geoPositionInfo.attribute(QGeoPositionInfo::Direction));
            qreal velocity = geoPositionInfo.attribute(QGeoPositionInfo::GroundSpeed);

            _motionReport.vx = cos(direction)*velocity;
            _motionReport.vy = sin(direction)*velocity;

        } else {
            _motionReport.vx = 0.0f;
            _motionReport.vy = 0.0f;
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
    follow_target.est_capabilities = estimatation_capabilities;
    follow_target.position_cov[0] = _motionReport.pos_std_dev[0];
    follow_target.position_cov[2] = _motionReport.pos_std_dev[2];
    follow_target.alt = _motionReport.alt;
    follow_target.lat = _motionReport.lat_int;
    follow_target.lon = _motionReport.lon_int;
    follow_target.vel[0] = _motionReport.vx;
    follow_target.vel[1] = _motionReport.vy;

    for (int i=0; i< vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
        if(vehicle->flightMode().compare(PX4FirmwarePlugin::followMeFlightMode, Qt::CaseInsensitive) == 0) {
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
