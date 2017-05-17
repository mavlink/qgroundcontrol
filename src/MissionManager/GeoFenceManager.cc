/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoFenceManager.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "ParameterManager.h"
#include "QGCMapPolygon.h"
#include "QGCMapCircle.h"

QGC_LOGGING_CATEGORY(GeoFenceManagerLog, "GeoFenceManagerLog")

GeoFenceManager::GeoFenceManager(Vehicle* vehicle)
    : _vehicle                  (vehicle)
    , _planManager              (vehicle, MAV_MISSION_TYPE_FENCE)
    , _firstParamLoadComplete   (false)
    , _circleRadiusFact         (NULL)
{
    connect(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged, this, &GeoFenceManager::_parametersReady);

    connect(&_planManager, &PlanManager::inProgressChanged,         this, &GeoFenceManager::inProgressChanged);
    connect(&_planManager, &PlanManager::error,                     this, &GeoFenceManager::error);
    connect(&_planManager, &PlanManager::removeAllComplete,         this, &GeoFenceManager::removeAllComplete);
    connect(&_planManager, &PlanManager::sendComplete,              this, &GeoFenceManager::_sendComplete);
    connect(&_planManager, &PlanManager::newMissionItemsAvailable,  this, &GeoFenceManager::_planManagerLoadComplete);

    if (_vehicle->parameterManager()->parametersReady()) {
        _parametersReady();
    }
}

GeoFenceManager::~GeoFenceManager()
{

}

void GeoFenceManager::_parametersReady(void)
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

bool GeoFenceManager::inProgress(void) const
{
    return _planManager.inProgress();
}

void GeoFenceManager::loadFromVehicle(void)
{
    _planManager.loadFromVehicle();
}

void GeoFenceManager::sendToVehicle(const QGeoCoordinate&   breachReturn,
                                    QmlObjectListModel&     inclusionPolygons,
                                    QmlObjectListModel&     exclusionPolygons,
                                    QmlObjectListModel&     inclusionCircles,
                                    QmlObjectListModel&     exclusionCircles)
{
    Q_UNUSED(breachReturn);

    QList<MissionItem*> fenceItems;

    _sendInclusionPolygons.clear();
    _sendExclusionPolygons.clear();
    _sendInclusionCircles.clear();
    _sendExclusionCircles.clear();

    for (int i=0; i<inclusionPolygons.count(); i++) {
        _sendInclusionPolygons.append(inclusionPolygons.value<QGCMapPolygon*>(i)->coordinateList());
    }
    for (int i=0; i<exclusionPolygons.count(); i++) {
        _sendExclusionPolygons.append(exclusionPolygons.value<QGCMapPolygon*>(i)->coordinateList());
    }
    for (int i=0; i<inclusionCircles.count(); i++) {
        _sendInclusionCircles.append(*inclusionCircles.value<QGCMapCircle*>(i));
    }
    for (int i=0; i<exclusionCircles.count(); i++) {
        _sendExclusionCircles.append(*exclusionCircles.value<QGCMapCircle*>(i));
    }

    for (int i=0; i<_sendInclusionPolygons.count(); i++) {
        const QList<QGeoCoordinate>& polygon = _sendInclusionPolygons[i];

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
            fenceItems.append(item);
        }
    }

    for (int i=0; i<_sendExclusionPolygons.count(); i++) {
        const QList<QGeoCoordinate>& polygon = _sendExclusionPolygons[i];

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
            fenceItems.append(item);
        }
    }

    for (int i=0; i<_sendInclusionCircles.count(); i++) {
        const QGCMapCircle& circle = _sendInclusionCircles[i];
        MissionItem* item = new MissionItem(0,
                                            MAV_CMD_NAV_FENCE_CIRCLE_INCLUSION,
                                            MAV_FRAME_GLOBAL,
                                            circle.radius(),
                                            0, 0, 0,                    // param 2-4 unused
                                            circle.center().latitude(),
                                            circle.center().longitude(),
                                            0,                          // param 7 unused
                                            false,                      // autocontinue
                                            false,                      // isCurrentItem
                                            this);                      // parent
        fenceItems.append(item);
    }

    for (int i=0; i<_sendExclusionCircles.count(); i++) {
        const QGCMapCircle& circle = _sendExclusionCircles[i];
        MissionItem* item = new MissionItem(0,
                                            MAV_CMD_NAV_FENCE_CIRCLE_EXCLUSION,
                                            MAV_FRAME_GLOBAL,
                                            circle.radius(),
                                            0, 0, 0,                    // param 2-4 unused
                                            circle.center().latitude(),
                                            circle.center().longitude(),
                                            0,                          // param 7 unused
                                            false,                      // autocontinue
                                            false,                      // isCurrentItem
                                            this);                      // parent
        fenceItems.append(item);
    }

    _planManager.writeMissionItems(fenceItems);

    for (int i=0; i<fenceItems.count(); i++) {
        fenceItems[i]->deleteLater();
    }
}

void GeoFenceManager::removeAll(void)
{
    _planManager.removeAll();
}

void GeoFenceManager::_sendComplete(bool error)
{
    if (error) {
        _inclusionPolygons.clear();
        _exclusionPolygons.clear();
        _inclusionCircles.clear();
        _exclusionCircles.clear();
    } else {
        _inclusionPolygons = _sendInclusionPolygons;
        _exclusionPolygons = _sendExclusionPolygons;
        _inclusionCircles = _sendInclusionCircles;
        _exclusionCircles = _sendExclusionCircles;
    }
    _sendInclusionPolygons.clear();
    _sendExclusionPolygons.clear();
    _sendInclusionCircles.clear();
    _sendExclusionCircles.clear();
    emit sendComplete(error);
}

void GeoFenceManager::_planManagerLoadComplete(bool removeAllRequested)
{
    bool loadFailed = false;

    Q_UNUSED(removeAllRequested);

    _inclusionPolygons.clear();
    _exclusionPolygons.clear();
    _inclusionCircles.clear();
    _exclusionCircles.clear();

    MAV_CMD expectedCommand = (MAV_CMD)0;
    int expectedVertexCount = 0;
    QList<QGeoCoordinate> nextPolygon;
    const QList<MissionItem*>& fenceItems = _planManager.missionItems();

    for (int i=0; i<fenceItems.count(); i++) {
        MissionItem* item = fenceItems[i];

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
        } else if (command == MAV_CMD_NAV_FENCE_CIRCLE_INCLUSION || command == MAV_CMD_NAV_FENCE_CIRCLE_EXCLUSION) {
            if (nextPolygon.count() != 0) {
                // Incomplete polygon
                emit error(IncompletePolygonLoad, tr("GeoFence load: Incomplete polygon loaded"));
                break;
            }
            QGCMapCircle circle(QGeoCoordinate(item->param5(), item->param6()), item->param1());
            if (command == MAV_CMD_NAV_FENCE_CIRCLE_INCLUSION) {
                _inclusionCircles.append(circle);
            } else {
                _exclusionCircles.append(circle);
            }
        } else {
            emit error(UnsupportedCommand, tr("GeoFence load: Unsupported command %1").arg(item->command()));
            break;
        }
    }

    if (loadFailed) {
        _inclusionPolygons.clear();
        _exclusionPolygons.clear();
        _inclusionCircles.clear();
        _exclusionCircles.clear();
    }

    emit loadComplete();
}
