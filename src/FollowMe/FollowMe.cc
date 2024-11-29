/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FollowMe.h"
#include "MultiVehicleManager.h"
#include "FirmwarePlugin.h"
#include "Vehicle.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCLoggingCategory.h"

#include <QtPositioning/QGeoPositionInfo>

QGC_LOGGING_CATEGORY(FollowMeLog, "qgc.followme")

Q_APPLICATION_STATIC(FollowMe, _followMeInstance);

FollowMe::FollowMe(QObject *parent)
    : QObject(parent)
    , _gcsMotionReportTimer(new QTimer(this))
{
    // qCDebug(FollowMeLog) << Q_FUNC_INFO << this;

    _gcsMotionReportTimer->setSingleShot(false);
}

FollowMe::~FollowMe()
{
    // qCDebug(FollowMeLog) << Q_FUNC_INFO << this;
}

FollowMe *FollowMe::instance()
{
    return _followMeInstance();
}

void FollowMe::init()
{
    static bool once = false;
    if (!once) {
        (void) connect(_gcsMotionReportTimer, &QTimer::timeout, this, &FollowMe::_sendGCSMotionReport);
        (void) connect(SettingsManager::instance()->appSettings()->followTarget(), &Fact::rawValueChanged, this, &FollowMe::_settingsChanged);

        _settingsChanged(SettingsManager::instance()->appSettings()->followTarget()->rawValue());
    }
    once = true;
}

void FollowMe::_settingsChanged(QVariant value)
{
    _currentMode = static_cast<FollowMode>(value.toUInt());

    switch (_currentMode) {
    case MODE_NEVER:
        (void) disconnect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &FollowMe::_vehicleAdded);
        (void) disconnect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, &FollowMe::_vehicleRemoved);
        _disableFollowSend();
        break;
    case MODE_ALWAYS:
        (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &FollowMe::_vehicleAdded);
        (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, &FollowMe::_vehicleRemoved);
        _enableFollowSend();
        break;
    case MODE_FOLLOWME:
        (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &FollowMe::_vehicleAdded);
        (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, &FollowMe::_vehicleRemoved);
        _enableIfVehicleInFollow();
        break;
    }
}

void FollowMe::_enableFollowSend()
{
    if (!_gcsMotionReportTimer->isActive()) {
        _gcsMotionReportTimer->setInterval(qMin(QGCPositionManager::instance()->updateInterval(), kMotionUpdateInterval));
        _gcsMotionReportTimer->start();
    }
}

void FollowMe::_disableFollowSend()
{
    if (_gcsMotionReportTimer->isActive()) {
        _gcsMotionReportTimer->stop();
    }
}

void FollowMe::_sendGCSMotionReport()
{
    const QGeoPositionInfo geoPositionInfo = QGCPositionManager::instance()->geoPositionInfo();
    const QGeoCoordinate gcsCoordinate = geoPositionInfo.coordinate();

    if (!geoPositionInfo.isValid()) {
        return;
    }

    // First check to see if any vehicles need follow me updates
    bool needFollowMe = false;
    if (_currentMode == MODE_ALWAYS) {
        needFollowMe = true;
    } else if (_currentMode == MODE_FOLLOWME) {
        QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();
        for (int i = 0; i < vehicles->count(); i++) {
            const Vehicle* const vehicle = vehicles->value<const Vehicle*>(i);
            if (_isFollowFlightMode(vehicle, vehicle->flightMode())) {
                needFollowMe = true;
            }
        }
    }
    if (!needFollowMe) {
        return;
    }

    GCSMotionReport motionReport{0};
    uint8_t estimationCapabilities = 0;

    // Get the current location coordinates
    // Important note: QGC only supports sending the constant GCS home position altitude for follow me.
    motionReport.lat_int = static_cast<int>(gcsCoordinate.latitude() * 1e7);
    motionReport.lon_int = static_cast<int>(gcsCoordinate.longitude() * 1e7);
    motionReport.altMetersAMSL = gcsCoordinate.altitude();
    estimationCapabilities |= (1 << POS);

    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction)) {
        estimationCapabilities |= (1 << HEADING);
        motionReport.headingDegrees = geoPositionInfo.attribute(QGeoPositionInfo::Direction);
    }

    // get the current eph
    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        motionReport.pos_std_dev[0] = motionReport.pos_std_dev[1] = geoPositionInfo.attribute(QGeoPositionInfo::HorizontalAccuracy);
    }

    // get the current epv
    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        motionReport.pos_std_dev[2] = geoPositionInfo.attribute(QGeoPositionInfo::VerticalAccuracy);
    }

    // calculate z velocity if it's available
    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::VerticalSpeed)) {
        motionReport.vzMetersPerSec = geoPositionInfo.attribute(QGeoPositionInfo::VerticalSpeed);
    }

    // calculate x,y velocity if it's available
    if (geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction) && geoPositionInfo.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
        estimationCapabilities |= (1 << VEL);

        const qreal direction = qDegreesToRadians(geoPositionInfo.attribute(QGeoPositionInfo::Direction));
        const qreal velocity = geoPositionInfo.attribute(QGeoPositionInfo::GroundSpeed);

        motionReport.vxMetersPerSec = cos(direction) * velocity;
        motionReport.vyMetersPerSec = sin(direction) * velocity;
    } else {
        motionReport.vxMetersPerSec = 0;
        motionReport.vyMetersPerSec = 0;
    }

    QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();

    for (int i = 0; i < vehicles->count(); i++) {
        Vehicle* const vehicle = vehicles->value<Vehicle*>(i);
        if ((_currentMode == MODE_ALWAYS) || (_isFollowFlightMode(vehicle, vehicle->flightMode()))) {
            qCDebug(FollowMeLog) << "sendGCSMotionReport latInt:lonInt:altMetersAMSL" << motionReport.lat_int << motionReport.lon_int << motionReport.altMetersAMSL;
            vehicle->firmwarePlugin()->sendGCSMotionReport(vehicle, motionReport, estimationCapabilities);
        }
    }
}

void FollowMe::_vehicleAdded(Vehicle *vehicle)
{
    (void) connect(vehicle, &Vehicle::flightModeChanged, this, &FollowMe::_enableIfVehicleInFollow);
    _enableIfVehicleInFollow();
}

void FollowMe::_vehicleRemoved(Vehicle *vehicle)
{
    (void) disconnect(vehicle, &Vehicle::flightModeChanged, this, &FollowMe::_enableIfVehicleInFollow);
    _enableIfVehicleInFollow();
}

void FollowMe::_enableIfVehicleInFollow()
{
    if (_currentMode == MODE_ALWAYS) {
        return;
    }

    // Any vehicle in follow mode will enable the system
    QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();

    for (int i = 0; i < vehicles->count(); i++) {
        const Vehicle* const vehicle = vehicles->value<const Vehicle*>(i);
        if (_isFollowFlightMode(vehicle, vehicle->flightMode())) {
            _enableFollowSend();
            return;
        }
    }

    _disableFollowSend();
}

bool FollowMe::_isFollowFlightMode(const Vehicle *vehicle, const QString &flightMode)
{
    return (flightMode.compare(vehicle->followFlightMode()) == 0);
}
