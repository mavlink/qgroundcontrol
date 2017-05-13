/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4GeoFenceManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "PlanManager.h"
#include "QGCQGeoCoordinate.h"

PX4GeoFenceManager::PX4GeoFenceManager(Vehicle* vehicle)
    : GeoFenceManager(vehicle)
    , _planManager(vehicle, MAV_MISSION_TYPE_FENCE)
    , _firstParamLoadComplete(false)
    , _circleRadiusFact(NULL)
{
    connect(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged, this, &PX4GeoFenceManager::_parametersReady);

    connect(&_planManager, &PlanManager::inProgressChanged,         this, &PX4GeoFenceManager::inProgressChanged);
    connect(&_planManager, &PlanManager::error,                     this, &PX4GeoFenceManager::error);
    connect(&_planManager, &PlanManager::removeAllComplete,         this, &PX4GeoFenceManager::removeAllComplete);
    connect(&_planManager, &PlanManager::sendComplete,              this, &PX4GeoFenceManager::_sendComplete);
    connect(&_planManager, &PlanManager::newMissionItemsAvailable,  this, &PX4GeoFenceManager::_planManagerLoadComplete);

    if (_vehicle->parameterManager()->parametersReady()) {
        _parametersReady();
    }

    void loadComplete                   (const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon);
}

PX4GeoFenceManager::~PX4GeoFenceManager()
{

}

void PX4GeoFenceManager::_parametersReady(void)
{
    if (!_firstParamLoadComplete) {
        _firstParamLoadComplete = true;

        _circleRadiusFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("GF_MAX_HOR_DIST"));
        emit circleRadiusFactChanged(_circleRadiusFact);

        QStringList paramNames;
        QStringList paramLabels;

        paramNames << QStringLiteral("GF_ACTION") << QStringLiteral("GF_MAX_HOR_DIST") << QStringLiteral("GF_MAX_VER_DIST");
        paramLabels << QStringLiteral("Breach Action:") << QStringLiteral("Radius:") << QStringLiteral("Max Altitude:");

        _params.clear();
        _paramLabels.clear();
        for (int i=0; i<paramNames.count(); i++) {
            QString paramName = paramNames[i];
            if (_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, paramName)) {
                Fact* paramFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName);
                _params << QVariant::fromValue(paramFact);
                _paramLabels << paramLabels[i];
            }
        }

        emit paramsChanged(_params);
        emit paramLabelsChanged(_paramLabels);
    }
}

bool PX4GeoFenceManager::inProgress(void) const
{
    return _planManager.inProgress();
}

void PX4GeoFenceManager::loadFromVehicle(void)
{
    _planManager.loadFromVehicle();
}

void PX4GeoFenceManager::sendToVehicle(const QGeoCoordinate& breachReturn, QmlObjectListModel& polygon)
{
    Q_UNUSED(breachReturn);

    qDebug() << polygon.count();

    QList<MissionItem*> polygonItems;

    _sendPolygon.clear();
    for (int i=0; i<polygon.count(); i++) {
        QGeoCoordinate vertex = polygon.value<QGCQGeoCoordinate*>(i)->coordinate();

        _sendPolygon.append(vertex);
        MissionItem* item = new MissionItem(0,
                                            MAV_CMD_NAV_FENCE_POLYGON_VERTEX_INCLUSION,
                                            MAV_FRAME_GLOBAL,
                                            0, 0, 0, 0,         // param 1-4 unused
                                            vertex.latitude(),
                                            vertex.longitude(),
                                            0,                  // param 7 unused
                                            false,              // autocontinue
                                            false,              // isCurrentItem
                                            this);              // parent
        polygonItems.append(item);
    }
    _planManager.writeMissionItems(polygonItems);

    for (int i=0; i<polygonItems.count(); i++) {
        polygonItems[i]->deleteLater();
    }
}

void PX4GeoFenceManager::removeAll(void)
{
    _planManager.removeAll();
}

void PX4GeoFenceManager::_sendComplete(bool error)
{
    if (error) {
        _polygon.clear();
    } else {
        _polygon = _sendPolygon;
    }
    _sendPolygon.clear();
    emit sendComplete(error);
}

void PX4GeoFenceManager::_planManagerLoadComplete(bool removeAllRequested)
{
    Q_UNUSED(removeAllRequested);
}
