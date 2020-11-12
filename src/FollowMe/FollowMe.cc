/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

QGC_LOGGING_CATEGORY(FollowMeLog, "FollowMeLog")

FollowMe::FollowMe(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _gcsMotionReportTimer.setSingleShot(false);
}

void FollowMe::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    connect(&_gcsMotionReportTimer,                                     &QTimer::timeout,       this, &FollowMe::_sendGCSMotionReport);
    connect(toolbox->settingsManager()->appSettings()->followTarget(),  &Fact::rawValueChanged, this, &FollowMe::_settingsChanged);

    _settingsChanged();
}

void FollowMe::_settingsChanged()
{
    _currentMode = _toolbox->settingsManager()->appSettings()->followTarget()->rawValue().toUInt();

    switch (_currentMode) {
    case MODE_NEVER:
        disconnect(_toolbox->multiVehicleManager(), &MultiVehicleManager::vehicleAdded,     this, &FollowMe::_vehicleAdded);
        disconnect(_toolbox->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved,   this, &FollowMe::_vehicleRemoved);
        _disableFollowSend();
        break;
    case MODE_ALWAYS:
        connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::vehicleAdded,    this, &FollowMe::_vehicleAdded);
        connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved,  this, &FollowMe::_vehicleRemoved);
        _enableFollowSend();
        break;
    case MODE_FOLLOWME:
        connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::vehicleAdded,    this, &FollowMe::_vehicleAdded);
        connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved,  this, &FollowMe::_vehicleRemoved);
        _enableIfVehicleInFollow();
        break;
    }
}

void FollowMe::_enableFollowSend()
{
    if (!_gcsMotionReportTimer.isActive()) {
        _gcsMotionReportTimer.setInterval(qMin(_toolbox->qgcPositionManager()->updateInterval(), 250));
        _gcsMotionReportTimer.start();
    }
}

void FollowMe::_disableFollowSend()
{
    if (_gcsMotionReportTimer.isActive()) {
        _gcsMotionReportTimer.stop();
    }
}

void FollowMe::_sendGCSMotionReport()
{
    QGeoPositionInfo    geoPositionInfo =   _toolbox->qgcPositionManager()->geoPositionInfo();
    QGeoCoordinate      gcsCoordinate =     geoPositionInfo.coordinate();

    if (!geoPositionInfo.isValid()) {
        return;
    }

    // First check to see if any vehicles need follow me updates
    bool needFollowMe = false;
    if (_currentMode == MODE_ALWAYS) {
        needFollowMe = true;
    } else if (_currentMode == MODE_FOLLOWME) {
        QmlObjectListModel* vehicles = _toolbox->multiVehicleManager()->vehicles();
        for (int i=0; i<vehicles->count(); i++) {
            Vehicle* vehicle = vehicles->value<Vehicle*>(i);
            if (_isFollowFlightMode(vehicle, vehicle->flightMode())) {
                needFollowMe = true;
            }
        }
    }
    if (!needFollowMe) {
        return;
    }

    GCSMotionReport motionReport = {};
    uint8_t         estimatation_capabilities = 0;

    // get the current location coordinates

    motionReport.lat_int =          static_cast<int>(gcsCoordinate.latitude()  * 1e7);
    motionReport.lon_int =          static_cast<int>(gcsCoordinate.longitude() * 1e7);
    motionReport.altMetersAMSL =    gcsCoordinate.altitude();
    estimatation_capabilities |=    (1 << POS);

    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction) == true) {
        estimatation_capabilities |= (1 << HEADING);
        motionReport.headingDegrees = geoPositionInfo.attribute(QGeoPositionInfo::Direction);
    }

    // get the current eph and epv

    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        motionReport.pos_std_dev[0] = motionReport.pos_std_dev[1] = geoPositionInfo.attribute(QGeoPositionInfo::HorizontalAccuracy);
    }

    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        motionReport.pos_std_dev[2] = geoPositionInfo.attribute(QGeoPositionInfo::VerticalAccuracy);
    }

    // calculate z velocity if it's available

    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalSpeed)) {
        motionReport.vzMetersPerSec = geoPositionInfo.attribute(QGeoPositionInfo::VerticalSpeed);
    }

    // calculate x,y velocity if it's available

    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction) && geoPositionInfo.hasAttribute(QGeoPositionInfo::GroundSpeed) == true) {
        estimatation_capabilities |= (1 << VEL);

        qreal direction = _degreesToRadian(geoPositionInfo.attribute(QGeoPositionInfo::Direction));
        qreal velocity  = geoPositionInfo.attribute(QGeoPositionInfo::GroundSpeed);

        motionReport.vxMetersPerSec = cos(direction)*velocity;
        motionReport.vyMetersPerSec = sin(direction)*velocity;
    } else {
        motionReport.vxMetersPerSec = 0;
        motionReport.vyMetersPerSec = 0;
    }

    QmlObjectListModel* vehicles = _toolbox->multiVehicleManager()->vehicles();

    for (int i=0; i<vehicles->count(); i++) {
        Vehicle* vehicle = vehicles->value<Vehicle*>(i);
        if (_currentMode == MODE_ALWAYS || _isFollowFlightMode(vehicle, vehicle->flightMode())) {
            qCDebug(FollowMeLog) << "sendGCSMotionReport latInt:lonInt:altMetersAMSL" << motionReport.lat_int << motionReport.lon_int << motionReport.altMetersAMSL;
            vehicle->firmwarePlugin()->sendGCSMotionReport(vehicle, motionReport, estimatation_capabilities);
        }
    }
}

double FollowMe::_degreesToRadian(double deg)
{
    return deg * M_PI / 180.0;
}

void FollowMe::_vehicleAdded(Vehicle* vehicle)
{
    connect(vehicle, &Vehicle::flightModeChanged, this, &FollowMe::_enableIfVehicleInFollow);
    _enableIfVehicleInFollow();
}

void FollowMe::_vehicleRemoved(Vehicle* vehicle)
{
    disconnect(vehicle, &Vehicle::flightModeChanged, this, &FollowMe::_enableIfVehicleInFollow);
    _enableIfVehicleInFollow();
}

void FollowMe::_enableIfVehicleInFollow(void)
{
    if (_currentMode == MODE_ALWAYS) {
        // System always enabled
        return;
    }

    // Any vehicle in follow mode will enable the system
    QmlObjectListModel* vehicles = _toolbox->multiVehicleManager()->vehicles();

    for (int i=0; i< vehicles->count(); i++) {
        Vehicle* vehicle = vehicles->value<Vehicle*>(i);
        if (_isFollowFlightMode(vehicle, vehicle->flightMode())) {
            _enableFollowSend();
            return;
        }
    }

    _disableFollowSend();
}

bool FollowMe::_isFollowFlightMode (Vehicle* vehicle, const QString& flightMode)
{
    return flightMode.compare(vehicle->followFlightMode()) == 0;
}
