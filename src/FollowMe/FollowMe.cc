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
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "FollowMe.h"
#include "Vehicle.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"

FollowMe::FollowMe(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox), estimatation_capabilities(0)
{
    memset(&_motionReport, 0, sizeof(motionReport_s));
    runTime.start();
    _gcsMotionReportTimer.setSingleShot(false);
}

void FollowMe::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    connect(&_gcsMotionReportTimer, &QTimer::timeout, this, &FollowMe::_sendGCSMotionReport);
    connect(toolbox->settingsManager()->appSettings()->followTarget(), &Fact::rawValueChanged, this, &FollowMe::_settingsChanged);
    _currentMode = _toolbox->settingsManager()->appSettings()->followTarget()->rawValue().toUInt();
    if(_currentMode == MODE_ALWAYS) {
        _enable();
    }
}

void FollowMe::followMeHandleManager(const QString&)
{
    //-- If we are to change based on current flight mode
    if(_currentMode == MODE_FOLLOWME) {
        QmlObjectListModel & vehicles = *_toolbox->multiVehicleManager()->vehicles();
        for (int i=0; i< vehicles.count(); i++) {
            Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
            if (vehicle->px4Firmware() && vehicle->flightMode().compare(FirmwarePlugin::px4FollowMeFlightMode, Qt::CaseInsensitive) == 0) {
                _enable();
                return;
            }
        }
        _disable();
    }
}

void FollowMe::_settingsChanged()
{
    uint32_t mode = _toolbox->settingsManager()->appSettings()->followTarget()->rawValue().toUInt();
    if(_currentMode != mode) {
        _currentMode = mode;
        switch (mode) {
        case MODE_NEVER:
            if(_gcsMotionReportTimer.isActive()) {
                _disable();
            }
            break;
        case MODE_ALWAYS:
            if(!_gcsMotionReportTimer.isActive()) {
                _enable();
            }
            break;
        case MODE_FOLLOWME:
            if(!_gcsMotionReportTimer.isActive()) {
                followMeHandleManager(QString());
            }
            break;
        default:
            break;
        }
    }
}

void FollowMe::_enable()
{
    connect(_toolbox->qgcPositionManager(),
            &QGCPositionManager::positionInfoUpdated,
            this,
            &FollowMe::_setGPSLocation);
    _gcsMotionReportTimer.setInterval(_toolbox->qgcPositionManager()->updateInterval());
    _gcsMotionReportTimer.start();
}

void FollowMe::_disable()
{
    disconnect(_toolbox->qgcPositionManager(),
               &QGCPositionManager::positionInfoUpdated,
               this,
               &FollowMe::_setGPSLocation);
    _gcsMotionReportTimer.stop();
}

void FollowMe::_setGPSLocation(QGeoPositionInfo geoPositionInfo)
{
    if (!geoPositionInfo.isValid()) {
        //-- Invalidate cached coordinates
        memset(&_motionReport, 0, sizeof(motionReport_s));
    } else {
        // get the current location coordinates

        QGeoCoordinate geoCoordinate = geoPositionInfo.coordinate();

        _motionReport.lat_int = geoCoordinate.latitude()  * 1e7;
        _motionReport.lon_int = geoCoordinate.longitude() * 1e7;
        _motionReport.alt     = geoCoordinate.altitude();

        estimatation_capabilities |= (1 << POS);

        // get the current eph and epv

        if(geoPositionInfo.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) == true) {
            _motionReport.pos_std_dev[0] = _motionReport.pos_std_dev[1] = geoPositionInfo.attribute(QGeoPositionInfo::HorizontalAccuracy);
        }

        if(geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalAccuracy) == true) {
            _motionReport.pos_std_dev[2] = geoPositionInfo.attribute(QGeoPositionInfo::VerticalAccuracy);
        }                

        // calculate z velocity if it's available

        if(geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalSpeed)) {
            _motionReport.vz = geoPositionInfo.attribute(QGeoPositionInfo::VerticalSpeed);
        }

        // calculate x,y velocity if it's available

        if((geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction)   == true) &&
           (geoPositionInfo.hasAttribute(QGeoPositionInfo::GroundSpeed) == true)) {

            estimatation_capabilities |= (1 << VEL);

            qreal direction = _degreesToRadian(geoPositionInfo.attribute(QGeoPositionInfo::Direction));
            qreal velocity  = geoPositionInfo.attribute(QGeoPositionInfo::GroundSpeed);

            _motionReport.vx = cos(direction)*velocity;
            _motionReport.vy = sin(direction)*velocity;

        } else {
            _motionReport.vx = 0.0f;
            _motionReport.vy = 0.0f;
        }
    }
}

void FollowMe::_sendGCSMotionReport()
{

    //-- Do we have any real data?
    if(_motionReport.lat_int == 0 && _motionReport.lon_int == 0 && _motionReport.alt == 0) {
        return;
    }

    QmlObjectListModel & vehicles    = *_toolbox->multiVehicleManager()->vehicles();
    MAVLinkProtocol* mavlinkProtocol = _toolbox->mavlinkProtocol();
    mavlink_follow_target_t follow_target;
    memset(&follow_target, 0, sizeof(follow_target));

    follow_target.timestamp = runTime.nsecsElapsed() * 1e-6;
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
        if(_currentMode || vehicle->flightMode().compare(FirmwarePlugin::px4FollowMeFlightMode, Qt::CaseInsensitive) == 0) {
            mavlink_message_t message;
            mavlink_msg_follow_target_encode_chan(mavlinkProtocol->getSystemId(),
                                                  mavlinkProtocol->getComponentId(),
                                                  vehicle->priorityLink()->mavlinkChannel(),
                                                  &message,
                                                  &follow_target);
            vehicle->sendMessageOnLink(vehicle->priorityLink(), message);
        }
    }
}

double FollowMe::_degreesToRadian(double deg)
{
    return deg * M_PI / 180.0;
}
