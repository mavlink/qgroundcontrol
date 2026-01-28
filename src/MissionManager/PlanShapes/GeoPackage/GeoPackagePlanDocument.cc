#include "GeoPackagePlanDocument.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "TransectStyleComplexItem.h"
#include "GeoPackageHelper.h"

GeoPackagePlanDocument::GeoPackagePlanDocument() = default;

void GeoPackagePlanDocument::_addFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
        if (uiInfo) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();

            if (uiInfo->isTakeoffCommand() && !vehicle->fixedWing()) {
                QGeoCoordinate coord = homeCoord;
                coord.setAltitude(item->param7() + altAdjustment);
                _flightPathCoords.append(coord);
            }

            if (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                coord.setAltitude(coord.altitude() + altAdjustment);
                _flightPathCoords.append(coord);
            }
        }
    }
}

void GeoPackagePlanDocument::_addWaypoints(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
        if (uiInfo && uiInfo->specifiesCoordinate()) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();
            QGeoCoordinate coord = item->coordinate();
            coord.setAltitude(coord.altitude() + altAdjustment);

            WaypointInfo wp;
            wp.coordinate = coord;
            wp.sequenceNumber = item->sequenceNumber();
            wp.command = static_cast<int>(item->command());
            wp.commandName = uiInfo->friendlyName();
            wp.altitudeAMSL = coord.altitude();
            wp.altitudeRelative = coord.altitude() - homeCoord.altitude();
            wp.isStandalone = uiInfo->isStandaloneCoordinate();

            _waypoints.append(wp);
        }
    }
}

void GeoPackagePlanDocument::_addComplexItems(QmlObjectListModel* visualItems)
{
    for (int i = 0; i < visualItems->count(); i++) {
        auto* transectItem = visualItems->value<TransectStyleComplexItem*>(i);
        if (transectItem) {
            QGCMapPolygon* polygon = transectItem->surveyAreaPolygon();
            if (polygon && polygon->count() >= 3) {
                QList<QGeoCoordinate> coords;
                for (int j = 0; j < polygon->count(); j++) {
                    coords.append(polygon->vertexCoordinate(j));
                }
                _areaPolygons.append(coords);
                _areaNames.append(transectItem->patternName());
            }
        }
    }
}

void GeoPackagePlanDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    _addFlightPath(vehicle, rgMissionItems);
    _addWaypoints(vehicle, rgMissionItems);
    _addComplexItems(visualItems);
}

bool GeoPackagePlanDocument::exportToFile(const QString& filename, QString& errorString) const
{
    // Create or open GeoPackage
    if (!GeoPackageHelper::createGeoPackage(filename, errorString)) {
        return false;
    }

    bool hasData = false;

    // Export waypoints as points
    if (!_waypoints.isEmpty()) {
        QList<QGeoCoordinate> points;
        for (const WaypointInfo& wp : _waypoints) {
            points.append(wp.coordinate);
        }
        if (GeoPackageHelper::savePoints(filename, points, QStringLiteral("waypoints"), errorString)) {
            hasData = true;
        }
    }

    // Export flight path as polyline
    if (_flightPathCoords.count() >= 2) {
        QList<QList<QGeoCoordinate>> polylines;
        polylines.append(_flightPathCoords);
        if (GeoPackageHelper::savePolylines(filename, polylines, QStringLiteral("flight_path"), errorString)) {
            hasData = true;
        }
    }

    // Export survey areas as polygons
    if (!_areaPolygons.isEmpty()) {
        if (GeoPackageHelper::savePolygons(filename, _areaPolygons, QStringLiteral("areas"), errorString)) {
            hasData = true;
        }
    }

    if (!hasData) {
        errorString = QObject::tr("No data to export");
        return false;
    }

    return true;
}
