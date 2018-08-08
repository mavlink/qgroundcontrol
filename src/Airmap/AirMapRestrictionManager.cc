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

#define RESTRICTION_UPDATE_DISTANCE    500     //-- 500m threshold for updates

using namespace airmap;

//-----------------------------------------------------------------------------
AirMapRestrictionManager::AirMapRestrictionManager(AirMapSharedState& shared)
    : _shared(shared)
{
}

//-----------------------------------------------------------------------------
void
AirMapRestrictionManager::setROI(const QGCGeoBoundingCube& roi, bool reset)
{
    //-- If first time or we've moved more than RESTRICTION_UPDATE_DISTANCE, ask for updates.
    if(reset || (!_lastROI.isValid() || _lastROI.pointNW.distanceTo(roi.pointNW) > RESTRICTION_UPDATE_DISTANCE || _lastROI.pointSE.distanceTo(roi.pointSE) > RESTRICTION_UPDATE_DISTANCE)) {
        //-- No more than 40000 km^2
        if(roi.area() < 40000.0) {
            _lastROI = roi;
            _requestRestrictions(roi);
        }
    }
}


//-----------------------------------------------------------------------------
QColor
AirMapRestrictionManager::_getColor(const Airspace::Type type)
{
    if(type == Airspace::Type::airport)
        return QColor(254,65,65,30);
    if(type == Airspace::Type::controlled_airspace)
        return QColor(254,158,65,60);
    if(type == Airspace::Type::special_use_airspace)
        return QColor(65,230,254,30);
    if(type == Airspace::Type::tfr)
        return QColor(95,230,254,30);
    if(type == Airspace::Type::wildfire)
        return QColor(254,120,0,30);
    if(type == Airspace::Type::park)
        return QColor(7,165,22,30);
    if(type == Airspace::Type::power_plant)
        return QColor(11,7,165,30);
    if(type == Airspace::Type::heliport)
        return QColor(233,57,57,30);
    if(type == Airspace::Type::prison)
        return QColor(100,100,100,30);
    if(type == Airspace::Type::school)
        return QColor(56,224,190,30);
    if(type == Airspace::Type::hospital)
        return QColor(56,159,224,30);
    if(type == Airspace::Type::fire)
        return QColor(223,83,10,30);
    if(type == Airspace::Type::emergency)
        return QColor(255,0,0,30);
    return QColor(255,0,255,30);
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
    qCDebug(AirMapManagerLog) << "Restrictions Request (ROI Changed)";
    _polygons.clear();
    _circles.clear();
    _state = State::RetrieveItems;
    Airspaces::Search::Parameters params;
    params.full = false;
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
                QColor color = _getColor(airspace.type());
                const Geometry& geometry = airspace.geometry();
                switch(geometry.type()) {
                    case Geometry::Type::polygon: {
                        const Geometry::Polygon& polygon = geometry.details_for_polygon();
                        _addPolygonToList(polygon, color);
                    }
                        break;
                    case Geometry::Type::multi_polygon: {
                        const Geometry::MultiPolygon& multiPolygon = geometry.details_for_multi_polygon();
                        for (const auto& polygon : multiPolygon) {
                            _addPolygonToList(polygon, color);
                        }
                    }
                        break;
                    case Geometry::Type::point: {
                        const Geometry::Point& point = geometry.details_for_point();
                        _circles.append(new AirspaceCircularRestriction(QGeoCoordinate(point.latitude, point.longitude), 0., color));
                        // TODO: radius???
                    }
                        break;
                    case Geometry::Type::invalid: {
                        qWarning() << "Invalid geometry type";
                    }
                        break;
                    default:
                        qWarning() << "Unsupported geometry type: " << static_cast<int>(geometry.type());
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
AirMapRestrictionManager::_addPolygonToList(const airmap::Geometry::Polygon& polygon, const QColor color)
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
    _polygons.append(new AirspacePolygonRestriction(polygonArray, color));
    if (polygon.inner_rings.size() > 0) {
        // no need to support those (they are rare, and in most cases, there's a more restrictive polygon filling the hole)
        qCDebug(AirMapManagerLog) << "Polygon with holes. Size: "<<polygon.inner_rings.size();
    }
}
