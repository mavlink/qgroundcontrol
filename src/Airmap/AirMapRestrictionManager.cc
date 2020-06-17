/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapRestrictionManager.h"
#include "AirMapManager.h"
#include "AirspaceRestriction.h"

#include "QGCApplication.h"
#include "SettingsManager.h"

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
    if(qgcApp()->toolbox()->settingsManager()->airMapSettings()->enableAirspace()->rawValue().toBool()) {
        //-- If first time or we've moved more than RESTRICTION_UPDATE_DISTANCE, ask for updates.
        if(reset ||
          (!_lastROI.isValid() || _lastROI.pointNW.distanceTo(roi.pointNW) > RESTRICTION_UPDATE_DISTANCE || _lastROI.pointSE.distanceTo(roi.pointSE) > RESTRICTION_UPDATE_DISTANCE) ||
           (_polygons.count() == 0 && _circles.count() == 0)) {
            //-- Limit area of interest
            qCDebug(AirMapManagerLog) << "ROI Area:" << roi.area() << "km^2";
            if(roi.area() < qgcApp()->toolbox()->airspaceManager()->maxAreaOfInterest()) {
                _lastROI = roi;
                _requestRestrictions(roi);
            } else {
                _polygons.clear();
                _circles.clear();
            }
        }
    }
}


//-----------------------------------------------------------------------------
void
AirMapRestrictionManager::_getColor(const Airspace& airspace, QColor& color, QColor& lineColor, float& lineWidth)
{
    if(airspace.type() == Airspace::Type::airport) {
        color       = QColor(246,165,23,50);
        lineColor   = QColor(246,165,23,255);
        lineWidth   = 2.0f;
    } else if(airspace.type() == Airspace::Type::controlled_airspace) {
        QString classification = QString::fromStdString(airspace.details_for_controlled_airspace().airspace_classification).toUpper();
        if(classification == "B") {
            color       = QColor(31,160,211,25);
            lineColor   = QColor(31,160,211,255);
            lineWidth   = 1.5f;
        } else if(classification == "C") {
            color       = QColor(155,108,157,25);
            lineColor   = QColor(155,108,157,255);
            lineWidth   = 1.5f;
        } else if(classification == "D") {
            color       = QColor(26,116,179,25);
            lineColor   = QColor(26,116,179,255);
            lineWidth   = 1.0f;
        } else if(classification == "E") {
            color       = QColor(155,108,157,25);
            lineColor   = QColor(155,108,157,255);
            lineWidth   = 1.0f;
        } else {
            //-- Don't know it
            qCWarning(AirMapManagerLog) << "Unknown airspace classification:" << QString::fromStdString(airspace.details_for_controlled_airspace().airspace_classification);
            color       = QColor(255,230,0,25);
            lineColor   = QColor(255,230,0,255);
            lineWidth   = 0.5f;
        }
    } else if(airspace.type() == Airspace::Type::special_use_airspace) {
        color       = QColor(27,90,207,38);
        lineColor   = QColor(27,90,207,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::tfr) {
        color       = QColor(244,67,54,38);
        lineColor   = QColor(244,67,54,255);
        lineWidth   = 2.0f;
    } else if(airspace.type() == Airspace::Type::wildfire) {
        color       = QColor(244,67,54,50);
        lineColor   = QColor(244,67,54,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::park) {
        color       = QColor(224,18,18,25);
        lineColor   = QColor(224,18,18,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::power_plant) {
        color       = QColor(246,165,23,25);
        lineColor   = QColor(246,165,23,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::heliport) {
        color       = QColor(246,165,23,20);
        lineColor   = QColor(246,165,23,100);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::prison) {
        color       = QColor(246,165,23,38);
        lineColor   = QColor(246,165,23,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::school) {
        color       = QColor(246,165,23,38);
        lineColor   = QColor(246,165,23,255);
        lineWidth   = 1.0f;
     } else if(airspace.type() == Airspace::Type::hospital) {
        color       = QColor(246,165,23,38);
        lineColor   = QColor(246,165,23,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::fire) {
        color       = QColor(244,67,54,50);
        lineColor   = QColor(244,67,54,255);
        lineWidth   = 1.0f;
    } else if(airspace.type() == Airspace::Type::emergency) {
        color       = QColor(246,113,23,77);
        lineColor   = QColor(246,113,23,255);
        lineWidth   = 1.0f;
    } else {
        //-- Don't know it
        qCWarning(AirMapManagerLog) << "Unknown airspace type:" << static_cast<int>(airspace.type());
        color       = QColor(255,0,230,25);
        lineColor   = QColor(255,0,230,255);
        lineWidth   = 0.5f;
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
                QColor color;
                QColor lineColor;
                float  lineWidth;
                _getColor(airspace, color, lineColor, lineWidth);
                const Geometry& geometry = airspace.geometry();
                switch(geometry.type()) {
                    case Geometry::Type::polygon: {
                        const Geometry::Polygon& polygon = geometry.details_for_polygon();
                        _addPolygonToList(polygon, QString::fromStdString(airspace.id()), color, lineColor, lineWidth);
                    }
                        break;
                    case Geometry::Type::multi_polygon: {
                        const Geometry::MultiPolygon& multiPolygon = geometry.details_for_multi_polygon();
                        for (const auto& polygon : multiPolygon) {
                            _addPolygonToList(polygon, QString::fromStdString(airspace.id()), color, lineColor, lineWidth);
                        }
                    }
                        break;
                    case Geometry::Type::point: {
                        const Geometry::Point& point = geometry.details_for_point();
                        _circles.append(new AirspaceCircularRestriction(QGeoCoordinate(point.latitude, point.longitude), 0., QString::fromStdString(airspace.id()), color, lineColor, lineWidth));
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
AirMapRestrictionManager::_addPolygonToList(const airmap::Geometry::Polygon& polygon, const QString advisoryID, const QColor color, const QColor lineColor, float lineWidth)
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
    _polygons.append(new AirspacePolygonRestriction(polygonArray, advisoryID, color, lineColor, lineWidth));
    if (polygon.inner_rings.size() > 0) {
        // no need to support those (they are rare, and in most cases, there's a more restrictive polygon filling the hole)
        qCDebug(AirMapManagerLog) << "Polygon with holes. Size: "<<polygon.inner_rings.size();
    }
}
