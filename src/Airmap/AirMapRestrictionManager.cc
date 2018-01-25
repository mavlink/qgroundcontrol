#include "AirMapRestrictionManager.h"

AirMapRestrictionManager::AirMapRestrictionManager(AirMapSharedState& shared)
    : _shared(shared)
{
}

void
AirMapRestrictionManager::setROI(const QGeoCoordinate& center, double radiusMeters)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Airspace";
        return;
    }
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapRestrictionManager::updateROI: state not idle";
        return;
    }
    qCDebug(AirMapManagerLog) << "setting ROI";
    _polygonList.clear();
    _circleList.clear();
    _state = State::RetrieveItems;
    Airspaces::Search::Parameters params;
    params.geometry = Geometry::point(center.latitude(), center.longitude());
    params.buffer = radiusMeters;
    params.full = true;
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
                        _addPolygonToList(polygon, _polygonList);
                    }
                        break;
                    case Geometry::Type::multi_polygon: {
                        const Geometry::MultiPolygon& multiPolygon = geometry.details_for_multi_polygon();
                        for (const auto& polygon : multiPolygon) {
                            _addPolygonToList(polygon, _polygonList);
                        }
                    }
                        break;
                    case Geometry::Type::point: {
                        const Geometry::Point& point = geometry.details_for_point();
                        _circleList.append(new AirspaceCircularRestriction(QGeoCoordinate(point.latitude, point.longitude), 0.));
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
        emit requestDone(true);
        _state = State::Idle;
    });
}

void
AirMapRestrictionManager::_addPolygonToList(const airmap::Geometry::Polygon& polygon, QList<AirspacePolygonRestriction*>& list)
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
    list.append(new AirspacePolygonRestriction(polygonArray));
    if (polygon.inner_rings.size() > 0) {
        // no need to support those (they are rare, and in most cases, there's a more restrictive polygon filling the hole)
        qCDebug(AirMapManagerLog) << "Polygon with holes. Size: "<<polygon.inner_rings.size();
    }
}

