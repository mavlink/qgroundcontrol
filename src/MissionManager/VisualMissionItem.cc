/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QStringList>
#include <QDebug>

#include "VisualMissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "JsonHelper.h"
#include "Terrain.h"

const char* VisualMissionItem::jsonTypeKey =                "type";
const char* VisualMissionItem::jsonTypeSimpleItemValue =    "SimpleItem";
const char* VisualMissionItem::jsonTypeComplexItemValue =   "ComplexItem";

VisualMissionItem::VisualMissionItem(Vehicle* vehicle, QObject* parent)
    : QObject                   (parent)
    , _vehicle                  (vehicle)
    , _isCurrentItem            (false)
    , _dirty                    (false)
    , _homePositionSpecialCase  (false)
    , _terrainAltitude          (qQNaN())
    , _altDifference            (0.0)
    , _altPercent               (0.0)
    , _terrainPercent           (qQNaN())
    , _azimuth                  (0.0)
    , _distance                 (0.0)
    , _missionGimbalYaw         (qQNaN())
    , _missionVehicleYaw        (qQNaN())
    , _lastLatTerrainQuery      (0)
    , _lastLonTerrainQuery      (0)
{
    _updateTerrainTimer.setInterval(500);
    _updateTerrainTimer.setSingleShot(true);
    connect(&_updateTerrainTimer, &QTimer::timeout, this, &VisualMissionItem::_reallyUpdateTerrainAltitude);

    connect(this, &VisualMissionItem::coordinateChanged, this, &VisualMissionItem::_updateTerrainAltitude);
}

VisualMissionItem::VisualMissionItem(const VisualMissionItem& other, QObject* parent)
    : QObject                   (parent)
    , _vehicle                  (NULL)
    , _isCurrentItem            (false)
    , _dirty                    (false)
    , _homePositionSpecialCase  (false)
    , _altDifference            (0.0)
    , _altPercent               (0.0)
    , _terrainPercent           (qQNaN())
    , _azimuth                  (0.0)
    , _distance                 (0.0)
{
    *this = other;
    connect(this, &VisualMissionItem::coordinateChanged, this, &VisualMissionItem::_updateTerrainAltitude);
}

const VisualMissionItem& VisualMissionItem::operator=(const VisualMissionItem& other)
{
    _vehicle = other._vehicle;

    setIsCurrentItem(other._isCurrentItem);
    setDirty(other._dirty);
    _homePositionSpecialCase = other._homePositionSpecialCase;
    _terrainAltitude = other._terrainAltitude;
    setAltDifference(other._altDifference);
    setAltPercent(other._altPercent);
    setTerrainPercent(other._terrainPercent);
    setAzimuth(other._azimuth);
    setDistance(other._distance);

    return *this;
}

VisualMissionItem::~VisualMissionItem()
{    
}

void VisualMissionItem::setIsCurrentItem(bool isCurrentItem)
{
    if (_isCurrentItem != isCurrentItem) {
        _isCurrentItem = isCurrentItem;
        emit isCurrentItemChanged(isCurrentItem);
    }
}

void VisualMissionItem::setDistance(double distance)
{
    if (!qFuzzyCompare(_distance, distance)) {
        _distance = distance;
        emit distanceChanged(_distance);
    }
}

void VisualMissionItem::setAltDifference(double altDifference)
{
    if (!qFuzzyCompare(_altDifference, altDifference)) {
        _altDifference = altDifference;
        emit altDifferenceChanged(_altDifference);
    }
}

void VisualMissionItem::setAltPercent(double altPercent)
{
    if (!qFuzzyCompare(_altPercent, altPercent)) {
        _altPercent = altPercent;
        emit altPercentChanged(_altPercent);
    }
}

void VisualMissionItem::setTerrainPercent(double terrainPercent)
{
    if (!qFuzzyCompare(_terrainPercent, terrainPercent)) {
        _terrainPercent = terrainPercent;
        emit terrainPercentChanged(terrainPercent);
    }
}

void VisualMissionItem::setAzimuth(double azimuth)
{
    if (!qFuzzyCompare(_azimuth, azimuth)) {
        _azimuth = azimuth;
        emit azimuthChanged(_azimuth);
    }
}

void VisualMissionItem::setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus)
{
    _missionFlightStatus = missionFlightStatus;
    if (qIsNaN(_missionFlightStatus.gimbalYaw) && qIsNaN(_missionGimbalYaw)) {
        return;
    }
    if (_missionFlightStatus.gimbalYaw != _missionGimbalYaw) {
        _missionGimbalYaw = _missionFlightStatus.gimbalYaw;
        emit missionGimbalYawChanged(_missionGimbalYaw);
    }
}

void VisualMissionItem::setMissionVehicleYaw(double vehicleYaw)
{
    if (!qFuzzyCompare(_missionVehicleYaw, vehicleYaw)) {
        _missionVehicleYaw = vehicleYaw;
        emit missionVehicleYawChanged(_missionVehicleYaw);
    }
}

void VisualMissionItem::_updateTerrainAltitude(void)
{
    if (coordinate().isValid()) {
        // We use a timer so that any additional requests before the timer fires result in only a single request
        _updateTerrainTimer.start();
    }
}

void VisualMissionItem::_reallyUpdateTerrainAltitude(void)
{
    QGeoCoordinate coord = coordinate();
    if (coord.isValid() && (qIsNaN(_terrainAltitude) || !qFuzzyCompare(_lastLatTerrainQuery, coord.latitude()) || qFuzzyCompare(_lastLonTerrainQuery, coord.longitude()))) {
        _lastLatTerrainQuery = coord.latitude();
        _lastLonTerrainQuery = coord.longitude();
        ElevationProvider* terrain = new ElevationProvider(this);
        connect(terrain, &ElevationProvider::terrainData, this, &VisualMissionItem::_terrainDataReceived);
        QList<QGeoCoordinate> rgCoord;
        rgCoord.append(coordinate());
        terrain->queryTerrainData(rgCoord);
    }
}

void VisualMissionItem::_terrainDataReceived(bool success, QList<float> altitudes)
{
    if (success) {
        _terrainAltitude = altitudes[0];
        emit terrainAltitudeChanged(_terrainAltitude);
        sender()->deleteLater();
    }
}
