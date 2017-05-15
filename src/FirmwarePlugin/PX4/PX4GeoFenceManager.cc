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
#include "QGCMapPolygon.h"

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

void PX4GeoFenceManager::sendToVehicle(const QGeoCoordinate& breachReturn, QmlObjectListModel& inclusionPolygons, QmlObjectListModel& exclusionPolygons)
{
    Q_UNUSED(breachReturn);

    QList<MissionItem*> polygonItems;

    _sendInclusions.clear();
    _sendExclusions.clear();
    for (int i=0; i<inclusionPolygons.count(); i++) {
        _sendInclusions.append(inclusionPolygons.value<QGCMapPolygon*>(i)->coordinateList());
    }
    for (int i=0; i<exclusionPolygons.count(); i++) {
        _sendExclusions.append(exclusionPolygons.value<QGCMapPolygon*>(i)->coordinateList());
    }

    for (int i=0; i<_sendInclusions.count(); i++) {
        const QList<QGeoCoordinate>& polygon = _sendInclusions[i];

        for (int j=0; j<polygon.count(); j++) {
            const QGeoCoordinate& vertex = polygon[j];

            MissionItem* item = new MissionItem(0,
                                                MAV_CMD_NAV_FENCE_POLYGON_VERTEX_INCLUSION,
                                                MAV_FRAME_GLOBAL,
                                                polygon.count(),    // vertex count
                                                0, 0, 0,            // param 2-4 unused
                                                vertex.latitude(),
                                                vertex.longitude(),
                                                0,                  // param 7 unused
                                                false,              // autocontinue
                                                false,              // isCurrentItem
                                                this);              // parent
            polygonItems.append(item);
        }
    }
    for (int i=0; i<_sendExclusions.count(); i++) {
        const QList<QGeoCoordinate>& polygon = _sendExclusions[i];

        for (int j=0; j<polygon.count(); j++) {
            const QGeoCoordinate& vertex = polygon[j];

            MissionItem* item = new MissionItem(0,
                                                MAV_CMD_NAV_FENCE_POLYGON_VERTEX_EXCLUSION,
                                                MAV_FRAME_GLOBAL,
                                                polygon.count(),    // vertex count
                                                0, 0, 0,            // param 2-4 unused
                                                vertex.latitude(),
                                                vertex.longitude(),
                                                0,                  // param 7 unused
                                                false,              // autocontinue
                                                false,              // isCurrentItem
                                                this);              // parent
            polygonItems.append(item);
        }
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
        _inclusionPolygons.clear();
        _exclusionPolygons.clear();
    } else {
        _inclusionPolygons = _sendInclusions;
        _exclusionPolygons = _sendExclusions;
    }
    _sendInclusions.clear();
    _sendExclusions.clear();
    emit sendComplete(error);
}

void PX4GeoFenceManager::_planManagerLoadComplete(bool removeAllRequested)
{
    Q_UNUSED(removeAllRequested);

    _inclusionPolygons.clear();
    _exclusionPolygons.clear();

    MAV_CMD expectedCommand = (MAV_CMD)0;
    int expectedVertexCount = 0;
    QList<QGeoCoordinate> nextPolygon;
    const QList<MissionItem*>& polygonItems = _planManager.missionItems();
    for (int i=0; i<polygonItems.count(); i++) {
        MissionItem* item = polygonItems[i];

        MAV_CMD command = item->command();
        if (command == MAV_CMD_NAV_FENCE_POLYGON_VERTEX_INCLUSION || command == MAV_CMD_NAV_FENCE_POLYGON_VERTEX_EXCLUSION) {
            if (nextPolygon.count() == 0) {
                // Starting a new polygon
                expectedVertexCount = item->param1();
                expectedCommand = command;
            } else if (expectedVertexCount != item->param1()){
                // In the middle of a polygon, but count suddenly changed
                emit error(BadPolygonItemFormat, tr("GeoFence load: Vertex count change mid-polygon - actual:expected").arg(item->param1()).arg(expectedVertexCount));
                break;
            } if (expectedCommand != command) {
                // Command changed before last polygon was completely loaded
                emit error(BadPolygonItemFormat, tr("GeoFence load: Polygon type changed before last load complete - actual:expected").arg(command).arg(expectedCommand));
                break;
            }
            nextPolygon.append(QGeoCoordinate(item->param5(), item->param6()));
            if (nextPolygon.count() == expectedVertexCount) {
                // Polygon is complete
                if (command == MAV_CMD_NAV_FENCE_POLYGON_VERTEX_INCLUSION) {
                    _inclusionPolygons.append(nextPolygon);
                } else {
                    _exclusionPolygons.append(nextPolygon);
                }
                nextPolygon.clear();
            }
        } else {
            emit error(UnsupportedCommand, tr("GeoFence load: Unsupported command %1").arg(item->command()));
            break;
        }
    }

    emit loadComplete();
}
