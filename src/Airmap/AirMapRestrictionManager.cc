/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapRestrictionManager.h"
#include "AirMapManager.h"
#include "AirspaceRestriction.h"

#include "airmap/airspaces.h"

#define RESTRICTION_UPDATE_DISTANCE    500     //-- 500m threshold for updates

using namespace airmap;

//-----------------------------------------------------------------------------
AirMapRestrictionManager::AirMapRestrictionManager(AirMapSharedState& shared)
    : _shared(shared)
{
}

//-----------------------------------------------------------------------------
void
AirMapRestrictionManager::setROI(const QGCGeoBoundingCube& roi)
{
    //-- If first time or we've moved more than RESTRICTION_UPDATE_DISTANCE, ask for updates.
    if(!_lastROI.isValid() || _lastROI.pointNW.distanceTo(roi.pointNW) > RESTRICTION_UPDATE_DISTANCE || _lastROI.pointSE.distanceTo(roi.pointSE) > RESTRICTION_UPDATE_DISTANCE) {
        //-- No more than 40000 km^2
        if(roi.area() < 40000.0) {
            _lastROI = roi;
            _requestRestrictions(roi);
        }
    }
}

//-----------------------------------------------------------------------------
void
AirMapRestrictionManager::_requestRestrictions(const QGCGeoBoundingCube& roi)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Airspace";
        return;
    }
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapRestrictionManager::updateROI: state not idle";
        return;
    }
    qCDebug(AirMapManagerLog) << "Setting Restriction Manager ROI";
    _polygons.clear();
    _circles.clear();
    _state = State::RetrieveItems;
    Airspaces::Search::Parameters params;
    params.full = true;
    params.date_time = Clock::universal_time();
    //-- Geometry: Polygon
    Geometry::Polygon polygon;
    for (const auto& qcoord : roi.polygon2D()) {
        Geometry::Coordinate coord;
        coord.latitude  = qcoord.latitude();
        coord.longitude = qcoord.longitude();
        polygon.outer_ring.coordinates.push_back(coord);
    }
    params.geometry = Geometry(polygon);
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->airspaces().search(params,
            [this, isAlive](const Airspaces::Search::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::RetrieveItems) return;
        if (result) {
            const std::vector<Airspace>& airspaces = result.value();
            qCDebug(AirMapManagerLog)<<"Successful search. Items:" << airspaces.size();
            for (const auto& airspace : airspaces) {
                const Geometry& geometry = airspace.geometry();
                switch(geometry.type()) {
                    case Geometry::Type::polygon: {
                        const Geometry::Polygon& polygon = geometry.details_for_polygon();
                        _addPolygonToList(polygon);
                    }
                        break;
                    case Geometry::Type::multi_polygon: {
                        const Geometry::MultiPolygon& multiPolygon = geometry.details_for_multi_polygon();
                        for (const auto& polygon : multiPolygon) {
                            _addPolygonToList(polygon);
                        }
                    }
                        break;
                    case Geometry::Type::point: {
                        const Geometry::Point& point = geometry.details_for_point();
                        _circles.append(new AirspaceCircularRestriction(QGeoCoordinate(point.latitude, point.longitude), 0.));
                        // TODO: radius???
                    }
                        break;
                    default:
                        qWarning() << "unsupported geometry type: "<<(int)geometry.type();
                        break;
                }
            }
        } else {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to retrieve Geofences",
                    QString::fromStdString(result.error().message()), description);
        }
        _state = State::Idle;
    });
}

//-----------------------------------------------------------------------------
void
AirMapRestrictionManager::_addPolygonToList(const airmap::Geometry::Polygon& polygon)
{
    QVariantList polygonArray;
    for (const auto& vertex : polygon.outer_ring.coordinates) {
        QGeoCoordinate coord;
        if (vertex.altitude) {
            coord = QGeoCoordinate(vertex.latitude, vertex.longitude, vertex.altitude.get());
        } else {
            coord = QGeoCoordinate(vertex.latitude, vertex.longitude);
        }
        polygonArray.append(QVariant::fromValue(coord));
    }
    _polygons.append(new AirspacePolygonRestriction(polygonArray));
    if (polygon.inner_rings.size() > 0) {
        // no need to support those (they are rare, and in most cases, there's a more restrictive polygon filling the hole)
        qCDebug(AirMapManagerLog) << "Polygon with holes. Size: "<<polygon.inner_rings.size();
    }
}
