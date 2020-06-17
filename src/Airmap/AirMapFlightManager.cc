/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapFlightManager.h"
#include "AirMapManager.h"
#include "AirMapRulesetsManager.h"
#include "QGCApplication.h"

#include "QGCMAVLink.h"

#include "airmap/pilots.h"
#include "airmap/flights.h"
#include "airmap/date_time.h"
#include "airmap/flight_plans.h"
#include "airmap/geometry.h"

using namespace airmap;

//-----------------------------------------------------------------------------
AirMapFlightManager::AirMapFlightManager(AirMapSharedState& shared)
    : _shared(shared)
{

}

//-----------------------------------------------------------------------------
void
AirMapFlightManager::findFlight(const QGCGeoBoundingCube& bc)
{
    _state = State::FetchFlights;
    _searchArea = bc;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.doRequestWithLogin([this, isAlive](const QString& login_token) {
        if (!isAlive.lock()) return;
        if (_state != State::FetchFlights) return;
        QList<QGeoCoordinate> coords = _searchArea.polygon2D();
        Geometry::LineString lineString;
        for (const auto& qcoord : coords) {
            Geometry::Coordinate coord;
            coord.latitude  = qcoord.latitude();
            coord.longitude = qcoord.longitude();
            lineString.coordinates.push_back(coord);
        }
        _flightID.clear();
        Flights::Search::Parameters params;
        params.authorization = login_token.toStdString();
        params.geometry      = Geometry(lineString);
        _shared.client()->flights().search(params, [this, isAlive](const Flights::Search::Result& result) {
            if (!isAlive.lock()) return;
            if (_state != State::FetchFlights) return;
            if (result && result.value().flights.size() > 0) {
                const Flights::Search::Response& response = result.value();
                qCDebug(AirMapManagerLog) << "Find flights response";
                for (const auto& flight : response.flights) {
                    QString fid = QString::fromStdString(flight.id);
                    qCDebug(AirMapManagerLog) << "Checking flight:" << fid;
                    if(flight.geometry.type() == Geometry::Type::line_string) {
                        const Geometry::LineString& lineString = flight.geometry.details_for_line_string();
                        QList<QGeoCoordinate> rcoords;
                        for (const auto& vertex : lineString.coordinates) {
                            rcoords.append(QGeoCoordinate(vertex.latitude, vertex.longitude));
                        }
                        if(_searchArea == rcoords) {
                            qCDebug(AirMapManagerLog) << "Found match:" << fid;
                            _flightID = fid;
                            _state = State::Idle;
                            emit flightIDChanged();
                            return;
                        }
                    }
                }
            }
            qCDebug(AirMapManagerLog) << "No flights found";
            emit flightIDChanged();
        });
        _state = State::Idle;
    });
}

//-----------------------------------------------------------------------------
void
AirMapFlightManager::endFlight(const QString& flightID)
{
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapFlightManager::endFlight: State not idle";
        return;
    }
    qCDebug(AirMapManagerLog) << "Ending flight" << flightID;
    _state = State::FlightEnd;
    Flights::EndFlight::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = flightID.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flights().end_flight(params, [this, isAlive](const Flights::EndFlight::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightEnd) return;
        _state = State::Idle;
        if (!result) {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to end Flight",
                    QString::fromStdString(result.error().message()), description);
        }
    });
}


