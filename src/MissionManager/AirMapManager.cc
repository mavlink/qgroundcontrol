/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "JsonHelper.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "AirMapSettings.h"
#include "QGCQGeoCoordinate.h"
#include "QGCApplication.h"

#include <airmap/authenticator.h>
#include <airmap/airspaces.h>
#include <airmap/flight_plans.h>
#include <airmap/flights.h>
#include <airmap/pilots.h>
#include <airmap/telemetry.h>

using namespace airmap;

QGC_LOGGING_CATEGORY(AirMapManagerLog, "AirMapManagerLog")


void AirMapSharedState::setSettings(const Settings& settings)
{
    logout();
    _settings = settings;
}

void AirMapSharedState::doRequestWithLogin(const Callback& callback)
{
    if (isLoggedIn()) {
        callback(_loginToken);
    } else {
        login();
        _pendingRequests.enqueue(callback);
    }
}

void AirMapSharedState::login()
{
    if (isLoggedIn() || _isLoginInProgress) {
        return;
    }
    _isLoginInProgress = true;

    if (_settings.userName == "") { //use anonymous login

        Authenticator::AuthenticateAnonymously::Params params;
        params.id = "";
        _client->authenticator().authenticate_anonymously(params,
                [this](const Authenticator::AuthenticateAnonymously::Result& result) {
            if (!_isLoginInProgress) { // was logout() called in the meanwhile?
                return;
            }
            if (result) {
                qCDebug(AirMapManagerLog)<<"Successfully authenticated with AirMap: id="<< result.value().id.c_str();
                _loginToken = QString::fromStdString(result.value().id);
                _processPendingRequests();
            } else {
                _pendingRequests.clear();
                try {
                    std::rethrow_exception(result.error());
                } catch (const std::exception& e) {
                    // TODO: emit signal
                    emit error("Failed to authenticate with AirMap", e.what(), "");
                }
            }
        });

    } else {

        Authenticator::AuthenticateWithPassword::Params params;
        params.oauth.username = _settings.userName.toStdString();
        params.oauth.password = _settings.password.toStdString();
        params.oauth.client_id = _settings.clientID.toStdString();
        params.oauth.device_id = "QGroundControl";
        _client->authenticator().authenticate_with_password(params,
                [this](const Authenticator::AuthenticateWithPassword::Result& result) {
            if (!_isLoginInProgress) { // was logout() called in the meanwhile?
                return;
            }
            if (result) {
                qCDebug(AirMapManagerLog)<<"Successfully authenticated with AirMap: id="<< result.value().id.c_str()<<", access="
                        <<result.value().access.c_str();
                _loginToken = QString::fromStdString(result.value().id);
                _processPendingRequests();
            } else {
                _pendingRequests.clear();
                try {
                    std::rethrow_exception(result.error());
                } catch (const std::exception& e) {
                    // TODO: proper error handling
                    emit error("Failed to authenticate with AirMap", e.what(), "");
                }
            }
        });
    }
}

void AirMapSharedState::_processPendingRequests()
{
    while (!_pendingRequests.isEmpty()) {
        _pendingRequests.dequeue()(_loginToken);
    }
}

void AirMapSharedState::logout()
{
    _isLoginInProgress = false; // cancel if we're currently trying to login

    if (!isLoggedIn()) {
        return;
    }
    _loginToken = "";
    _pendingRequests.clear();

}


AirMapRestrictionManager::AirMapRestrictionManager(AirMapSharedState& shared)
    : _shared(shared)
{
    //connect(&_networking, &AirMapNetworking::finished, this, &AirMapRestrictionManager::_parseAirspaceJson);
    //connect(&_networking, &AirMapNetworking::error, this, &AirMapRestrictionManager::_error);
}

void AirMapRestrictionManager::setROI(const QGeoCoordinate& center, double radiusMeters)
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
                        _circleList.append(new CircularAirspaceRestriction(QGeoCoordinate(point.latitude, point.longitude), 0.));
                        // TODO: radius???
                    }
                        break;
                    default:
                        qWarning() << "unsupported geometry type: "<<(int)geometry.type();
                        break;
                }

            }

        } else {
            try {
                std::rethrow_exception(result.error());
            } catch (const std::exception& e) {
                // TODO: proper error handling
                emit error("Failed to authenticate with AirMap", e.what(), "");
            }
        }
        emit requestDone(true);
        _state = State::Idle;
    });
}

void AirMapRestrictionManager::_addPolygonToList(const airmap::Geometry::Polygon& polygon, QList<PolygonAirspaceRestriction*>& list)
{
    QVariantList polygonArray;
    if (polygon.size() == 1) {
        for (const auto& vertex : polygon[0].coordinates) {
            QGeoCoordinate coord;
            if (vertex.altitude) {
                coord = QGeoCoordinate(vertex.latitude, vertex.longitude, vertex.altitude.get());
            } else {
                coord = QGeoCoordinate(vertex.latitude, vertex.longitude);
            }
            polygonArray.append(QVariant::fromValue(coord));
        }
        list.append(new PolygonAirspaceRestriction(polygonArray));

    } else {
        // no need to support those (they are rare, and in most cases, there's a more restrictive polygon filling the hole)
        qCDebug(AirMapManagerLog) << "Empty polygon, or Polygon with holes. Size: "<<polygon.size();
    }
}

AirMapFlightManager::AirMapFlightManager(AirMapSharedState& shared)
    : _shared(shared)
{
    connect(&_pollTimer, &QTimer::timeout, this, &AirMapFlightManager::_pollBriefing);
}

void AirMapFlightManager::createFlight(const QList<MissionItem*>& missionItems)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Will not create a flight";
        return;
    }

    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapFlightManager::createFlight: State not idle";
        return;
    }

    _flight.reset();

    // get the flight trajectory
    for(const auto &item : missionItems) {
        switch(item->command()) {
            case MAV_CMD_NAV_WAYPOINT:
            case MAV_CMD_NAV_LAND:
            case MAV_CMD_NAV_TAKEOFF:
                // TODO: others too?
            {
                // TODO: handle different coordinate frames?
                double lat = item->param5();
                double lon = item->param6();
                double alt = item->param7();
                _flight.coords.append(QGeoCoordinate(lat, lon, alt));
                if (alt > _flight.maxAltitude) {
                    _flight.maxAltitude = alt;
                }
                if (item->command() == MAV_CMD_NAV_TAKEOFF) {
                    _flight.takeoffCoord = _flight.coords.last();
                }
            }
                break;
            default:
                break;
        }
    }
    if (_flight.coords.empty()) {
        return;
    }

    _flight.maxAltitude += 5; // add a safety buffer


    if (_pilotID == "") {
        // need to get the pilot id before uploading the flight
        qCDebug(AirMapManagerLog) << "Getting pilot ID";
        _state = State::GetPilotID;
        std::weak_ptr<LifetimeChecker> isAlive(_instance);
        _shared.doRequestWithLogin([this, isAlive](const QString& login_token) {
            if (!isAlive.lock()) return;

            Pilots::Authenticated::Parameters params;
            params.authorization = login_token.toStdString();
            _shared.client()->pilots().authenticated(params, [this, isAlive](const Pilots::Authenticated::Result& result) {

                if (!isAlive.lock()) return;
                if (_state != State::GetPilotID) return;

                if (result) {
                    _pilotID = QString::fromStdString(result.value().id);
                    qCDebug(AirMapManagerLog) << "Got Pilot ID:"<<_pilotID;
                    _uploadFlight();
                } else {
                    _flightPermitStatus = AirspaceAuthorization::PermitUnknown;
                    emit flightPermitStatusChanged();
                    _state = State::Idle;

                    try {
                        std::rethrow_exception(result.error());
                    } catch (const std::exception& e) {
                        // TODO: proper error handling
                        emit error("Failed to create Flight Plan", e.what(), "");
                    }
                }
            });
        });
    } else {
        _uploadFlight();
    }

    _flightPermitStatus = AirspaceAuthorization::PermitPending;
    emit flightPermitStatusChanged();
}

void AirMapFlightManager::_endFirstFlight()
{
    // it could be that AirMap still has an open pending flight, but we don't know the flight ID.
    // As there can only be one, we query the flights that end in the future, and close it if there's one.

    _state = State::EndFirstFlight;

    Flights::Search::Parameters params;
    params.pilot_id = _pilotID.toStdString();
    params.end_after = Clock::universal_time() - Hours{1};

    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flights().search(params, [this, isAlive](const Flights::Search::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::EndFirstFlight) return;

        if (result && result.value().size() > 0) {

            Q_ASSERT(_shared.loginToken() != ""); // at this point we know the user is logged in (we queried the pilot id)

            Flights::EndFlight::Parameters params;
            params.authorization = _shared.loginToken().toStdString();
            params.id = result.value()[0].id; // pick the first flight (TODO: match the vehicle id)
            _shared.client()->flights().end_flight(params, [this, isAlive](const Flights::EndFlight::Result& result) {
                if (!isAlive.lock()) return;
                if (_state != State::EndFirstFlight) return;

                if (!result) {
                    try {
                        std::rethrow_exception(result.error());
                    } catch (const std::exception& e) {
                        // TODO: emit signal
                        emit error("Failed to end first Flight", e.what(), "");
                    }
                }
                _state = State::Idle;
                _uploadFlight();
            });

        } else {
            _state = State::Idle;
            _uploadFlight();
        }
    });
}

void AirMapFlightManager::_uploadFlight()
{
    if (_pendingFlightId != "") {
        // we need to end an existing flight first
        _endFlight(_pendingFlightId);
        return;
    }

    if (_noFlightCreatedYet) {
        _endFirstFlight();
        _noFlightCreatedYet = false;
        return;
    }

    qCDebug(AirMapManagerLog) << "uploading flight";
    _state = State::FlightUpload;

    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.doRequestWithLogin([this, isAlive](const QString& login_token) {

        if (!isAlive.lock()) return;
        if (_state != State::FlightUpload) return;

        FlightPlans::Create::Parameters params;
        params.max_altitude = _flight.maxAltitude;
        params.buffer = 2.f;
        params.latitude = _flight.takeoffCoord.latitude();
        params.longitude = _flight.takeoffCoord.longitude();
        params.pilot.id = _pilotID.toStdString();
        params.start_time = Clock::universal_time() + Minutes{5};
        params.end_time = Clock::universal_time() + Hours{2}; // TODO: user-configurable?
        params.rulesets = { // TODO: which ones to use?
                "che_drone_rules",
                "che_notam",
                "che_airmap_rules",
                "che_nature_preserve"
                };

        // geometry: LineString
        Geometry::LineString lineString;
        for (const auto& qcoord : _flight.coords) {
            Geometry::Coordinate coord;
            coord.latitude = qcoord.latitude();
            coord.longitude = qcoord.longitude();
            lineString.coordinates.push_back(coord);
        }

        params.geometry = Geometry(lineString);
        params.authorization = login_token.toStdString();
        _flight.coords.clear();

        _shared.client()->flight_plans().create_by_polygon(params, [this, isAlive](const FlightPlans::Create::Result& result) {
            if (!isAlive.lock()) return;
            if (_state != State::FlightUpload) return;

            if (result) {
                _pendingFlightPlan = QString::fromStdString(result.value().id);
                qCDebug(AirMapManagerLog) << "Flight Plan created:"<<_pendingFlightPlan;

                _checkForValidBriefing();

            } else {
                // TODO
                qCDebug(AirMapManagerLog) << "Flight Plan creation failed";
            }

        });

    });
}

void AirMapFlightManager::_checkForValidBriefing()
{
    _state = State::FlightBrief;
    FlightPlans::RenderBriefing::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = _pendingFlightPlan.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flight_plans().render_briefing(params, [this, isAlive](const FlightPlans::RenderBriefing::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightBrief) return;

        if (result) {
            bool allValid = true;
            for (const auto& validation : result.value().validations) {
                if (validation.status != FlightPlan::Briefing::Validation::Status::valid) {
                    emit error(QString("%1 registration identifier is invalid: %2").arg(
                        QString::fromStdString(validation.authority.name)).arg(QString::fromStdString(validation.message)), "", "");
                    allValid = false;
                }
            }
            if (allValid) {
                _submitPendingFlightPlan();
            } else {
                _flightPermitStatus = AirspaceAuthorization::PermitRejected;
                emit flightPermitStatusChanged();
                _state = State::Idle;
            }

        } else {
            // TODO
        }
    });
}

void AirMapFlightManager::_submitPendingFlightPlan()
{
    _state = State::FlightSubmit;
    FlightPlans::Submit::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = _pendingFlightPlan.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flight_plans().submit(params, [this, isAlive](const FlightPlans::Submit::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightSubmit) return;

        if (result) {
            _pendingFlightId = QString::fromStdString(result.value().flight_id.get());
            _state = State::FlightPolling;
            _pollBriefing();

        } else {
            // TODO
        }
    });
}

void AirMapFlightManager::_pollBriefing()
{
    if (_state != State::FlightPolling) {
        qCWarning(AirMapManagerLog) << "AirMapFlightManager::_pollBriefing: not in polling state";
        return;
    }

    FlightPlans::RenderBriefing::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = _pendingFlightPlan.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flight_plans().render_briefing(params, [this, isAlive](const FlightPlans::RenderBriefing::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightPolling) return;

        if (result) {
            const FlightPlan::Briefing& briefing = result.value();
            qCDebug(AirMapManagerLog) << "flight polling/briefing response";
            bool rejected = false;
            bool accepted = false;
            bool pending = false;
            for (const auto& authorization : briefing.authorizations) {
                switch (authorization.status) {
                case FlightPlan::Briefing::Authorization::Status::accepted:
                case FlightPlan::Briefing::Authorization::Status::accepted_upon_submission:
                    accepted = true;
                    break;
                case FlightPlan::Briefing::Authorization::Status::rejected:
                case FlightPlan::Briefing::Authorization::Status::rejected_upon_submission:
                    rejected = true;
                    break;
                case FlightPlan::Briefing::Authorization::Status::pending:
                    pending = true;
                    break;
                }
            }

            if (briefing.authorizations.size() == 0) {
                // if we don't get any authorizations, we assume it's accepted
                accepted = true;
            }

            qCDebug(AirMapManagerLog) << "flight approval: accepted=" << accepted << "rejected" << rejected << "pending" << pending;

            if ((rejected || accepted) && !pending) {
                if (rejected) { // rejected has priority
                    _flightPermitStatus = AirspaceAuthorization::PermitRejected;
                } else {
                    _flightPermitStatus = AirspaceAuthorization::PermitAccepted;
                }
                _currentFlightId = _pendingFlightId;
                _pendingFlightPlan = "";
                emit flightPermitStatusChanged();
                _state = State::Idle;
            } else {
                // wait until we send the next polling request
                _pollTimer.setSingleShot(true);
                _pollTimer.start(2000);
            }
        } else {
            // TODO: error handling
        }
    });
}

void AirMapFlightManager::endFlight()
{
    if (_currentFlightId.length() == 0) {
        return;
    }
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapFlightManager::endFlight: State not idle";
        return;
    }
    _endFlight(_currentFlightId);

    _flightPermitStatus = AirspaceAuthorization::PermitUnknown;
    emit flightPermitStatusChanged();
}

void AirMapFlightManager::_endFlight(const QString& flightID)
{
    qCDebug(AirMapManagerLog) << "ending flight" << flightID;

    _state = State::FlightEnd;

    Q_ASSERT(_shared.loginToken() != ""); // Since we have a flight ID, we need to be logged in

    Flights::EndFlight::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = flightID.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flights().end_flight(params, [this, isAlive](const Flights::EndFlight::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightEnd) return;

        _state = State::Idle;
        _pendingFlightId = "";
        _pendingFlightPlan = "";
        _currentFlightId = "";
        if (result) {
            if (!_flight.coords.empty()) {
                _uploadFlight();
            }
        } else {
            try {
                std::rethrow_exception(result.error());
            } catch (const std::exception& e) {
                // TODO: emit signal
                emit error("Failed to end Flight", e.what(), "");
            }
        }
    });
}


AirMapTelemetry::AirMapTelemetry(AirMapSharedState& shared)
: _shared(shared)
{
}

void AirMapTelemetry::vehicleMavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _handleGlobalPositionInt(message);
        break;
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _handleGPSRawInt(message);
        break;
    }

}

bool AirMapTelemetry::isTelemetryStreaming() const
{
    return _state == State::Streaming;
}

void AirMapTelemetry::_handleGPSRawInt(const mavlink_message_t& message)
{
    if (!isTelemetryStreaming()) {
        return;
    }

    mavlink_gps_raw_int_t gps_raw;
    mavlink_msg_gps_raw_int_decode(&message, &gps_raw);

    if (gps_raw.eph == UINT16_MAX) {
        _lastHdop = 1.f;
    } else {
        _lastHdop = gps_raw.eph / 100.f;
    }
}

void AirMapTelemetry::_handleGlobalPositionInt(const mavlink_message_t& message)
{
    if (!isTelemetryStreaming()) {
        return;
    }

    mavlink_global_position_int_t globalPosition;
    mavlink_msg_global_position_int_decode(&message, &globalPosition);

    Telemetry::Position position{
        milliseconds_since_epoch(Clock::universal_time()),
        (double) globalPosition.lat / 1e7,
        (double) globalPosition.lon / 1e7,
        (float) globalPosition.alt / 1000.f,
        (float) globalPosition.relative_alt / 1000.f,
        _lastHdop
    };
    Telemetry::Speed speed{
        milliseconds_since_epoch(Clock::universal_time()),
        globalPosition.vx / 100.f,
        globalPosition.vy / 100.f,
        globalPosition.vz / 100.f
    };

    Flight flight;
    flight.id = _flightID.toStdString();
    _shared.client()->telemetry().submit_updates(flight, _key,
        {Telemetry::Update{position}, Telemetry::Update{speed}});

}

void AirMapTelemetry::startTelemetryStream(const QString& flightID)
{
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "Not starting telemetry: not in idle state:" << (int)_state;
        return;
    }

    qCInfo(AirMapManagerLog) << "Starting Telemetry stream with flightID" << flightID;

    _state = State::StartCommunication;
    _flightID = flightID;

    Flights::StartFlightCommunications::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = _flightID.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flights().start_flight_communications(params, [this, isAlive](const Flights::StartFlightCommunications::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::StartCommunication) return;

        if (result) {
            _key = result.value().key;
            _state = State::Streaming;
        } else {
            _state = State::Idle;
            try {
                std::rethrow_exception(result.error());
            } catch (const std::exception& e) {
                // TODO: emit signal
                emit error("Failed to start telemetry streaming", e.what(), "");
            }
        }
    });
}

void AirMapTelemetry::stopTelemetryStream()
{
    if (_state == State::Idle) {
        return;
    }
    qCInfo(AirMapManagerLog) << "Stopping Telemetry stream with flightID" << _flightID;

    _state = State::EndCommunication;
    Flights::EndFlightCommunications::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = _flightID.toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flights().end_flight_communications(params, [this, isAlive](const Flights::EndFlightCommunications::Result& result) {
        Q_UNUSED(result);
        if (!isAlive.lock()) return;
        if (_state != State::EndCommunication) return;

        _key = "";
        _state = State::Idle;
    });
}

AirMapTrafficMonitor::~AirMapTrafficMonitor()
{
    stop();
}

void AirMapTrafficMonitor::startConnection(const QString& flightID)
{
    _flightID = flightID;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    auto handler = [this, isAlive](const Traffic::Monitor::Result& result) {
        if (!isAlive.lock()) return;
        if (result) {
            _monitor = result.value();
            _subscriber = std::make_shared<Traffic::Monitor::FunctionalSubscriber>(
                    std::bind(&AirMapTrafficMonitor::_update, this, std::placeholders::_1,  std::placeholders::_2));
            _monitor->subscribe(_subscriber);
        } else {
            try {
                std::rethrow_exception(result.error());
            } catch (const std::exception& e) {
                // TODO: error
            }
        }
    };

    Traffic::Monitor::Params params{flightID.toStdString(), _shared.loginToken().toStdString()};
    _shared.client()->traffic().monitor(params, handler);
}

void AirMapTrafficMonitor::_update(Traffic::Update::Type type, const std::vector<Traffic::Update>& update)
{
    qCDebug(AirMapManagerLog) << "Traffic update with" << update.size() << "elements";

    if (type != Traffic::Update::Type::situational_awareness)
        return; // currently we're only interested in situational awareness

    for (const auto& traffic : update) {
        QString traffic_id = QString::fromStdString(traffic.id);
        QString vehicle_id = QString::fromStdString(traffic.aircraft_id);
        emit trafficUpdate(traffic_id, vehicle_id, QGeoCoordinate(traffic.latitude, traffic.longitude, traffic.altitude),
                traffic.heading);
    }
}

void AirMapTrafficMonitor::stop()
{
    if (_monitor) {
        _monitor->unsubscribe(_subscriber);
        _subscriber.reset();
        _monitor.reset();
    }
}

AirMapManagerPerVehicle::AirMapManagerPerVehicle(AirMapSharedState& shared, const Vehicle& vehicle,
        QGCToolbox& toolbox)
    : AirspaceManagerPerVehicle(vehicle), _shared(shared),
    _flightManager(shared), _telemetry(shared), _trafficMonitor(shared),
    _toolbox(toolbox)
{
    connect(&_flightManager, &AirMapFlightManager::flightPermitStatusChanged,
            this, &AirMapManagerPerVehicle::flightPermitStatusChanged);
    connect(&_flightManager, &AirMapFlightManager::flightPermitStatusChanged,
            this, &AirMapManagerPerVehicle::_flightPermitStatusChanged);
    //connect(&_flightManager, &AirMapFlightManager::networkError, this, &AirMapManagerPerVehicle::networkError);
    //connect(&_telemetry, &AirMapTelemetry::networkError, this, &AirMapManagerPerVehicle::networkError);
    connect(&_trafficMonitor, &AirMapTrafficMonitor::trafficUpdate, this, &AirspaceManagerPerVehicle::trafficUpdate);
}

void AirMapManagerPerVehicle::createFlight(const QList<MissionItem*>& missionItems)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Will not create a flight";
        return;
    }
    _flightManager.createFlight(missionItems);
}

AirspaceAuthorization::PermitStatus AirMapManagerPerVehicle::flightPermitStatus() const
{
    return _flightManager.flightPermitStatus();
}

void AirMapManagerPerVehicle::startTelemetryStream()
{
    if (_flightManager.flightID() != "") {
        _telemetry.startTelemetryStream(_flightManager.flightID());
    }
}

void AirMapManagerPerVehicle::stopTelemetryStream()
{
    _telemetry.stopTelemetryStream();
}

bool AirMapManagerPerVehicle::isTelemetryStreaming() const
{
    return _telemetry.isTelemetryStreaming();
}

void AirMapManagerPerVehicle::endFlight()
{
    _flightManager.endFlight();
    _trafficMonitor.stop();
}

void AirMapManagerPerVehicle::vehicleMavlinkMessageReceived(const mavlink_message_t& message)
{
    if (isTelemetryStreaming()) {
        _telemetry.vehicleMavlinkMessageReceived(message);
    }
}

void AirMapManagerPerVehicle::_flightPermitStatusChanged()
{
    // activate traffic alerts
    if (_flightManager.flightPermitStatus() == AirspaceAuthorization::PermitAccepted) {
        qCDebug(AirMapManagerLog) << "Subscribing to Traffic Alerts";
        // since we already created the flight, we know that we have a valid login token
        _trafficMonitor.startConnection(_flightManager.flightID());
    }
}

AirMapManager::AirMapManager(QGCApplication* app, QGCToolbox* toolbox)
    : AirspaceManager(app, toolbox)
{
    _logger = std::make_shared<qt::Logger>();
    qt::register_types(); // TODO: still needed?
    _logger->logging_category().setEnabled(QtDebugMsg, true);
    _logger->logging_category().setEnabled(QtInfoMsg, true);
    _logger->logging_category().setEnabled(QtWarningMsg, true);
    _dispatchingLogger = std::make_shared<qt::DispatchingLogger>(_logger);

    connect(&_shared, &AirMapSharedState::error, this, &AirMapManager::_error);
}

AirMapManager::~AirMapManager()
{
    if (_shared.client()) {
        delete _shared.client();
    }
}

void AirMapManager::setToolbox(QGCToolbox* toolbox)
{
    AirspaceManager::setToolbox(toolbox);
    AirMapSettings* ap = toolbox->settingsManager()->airMapSettings();

    connect(ap->apiKey(),   &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->clientID(),   &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->userName(),   &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->password(),   &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);

    _settingsChanged();

}

void AirMapManager::_error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails)
{
    //TODO: console message & UI message
    //qgcApp()->showMessage(QString("AirMap error: %1%2").arg(errorString).arg(errorDetails));
    qCDebug(AirMapManagerLog) << "Caught error: "<<what<<", "<<airmapdMessage<<", "<<airmapdDetails;
}

void AirMapManager::requestWeatherUpdate(const QGeoCoordinate& coordinate)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Weather information";
        emit weatherUpdate(false, QGeoCoordinate{}, WeatherInformation{});
        return;
    }

    Status::GetStatus::Parameters params;
    params.longitude = coordinate.longitude();
    params.latitude = coordinate.latitude();
    params.weather = true;

    _shared.client()->status().get_status_by_point(params, [this, coordinate](const Status::GetStatus::Result& result) {

        if (result) {

            const Status::Weather& weather = result.value().weather;
            WeatherInformation weatherUpdateInfo;

            weatherUpdateInfo.condition = QString::fromStdString(weather.condition);
            weatherUpdateInfo.icon = QString::fromStdString(weather.icon);
            weatherUpdateInfo.windHeading = weather.wind.heading;
            weatherUpdateInfo.windSpeed = weather.wind.speed;
            weatherUpdateInfo.windGusting = weather.wind.gusting;
            weatherUpdateInfo.temperature = weather.temperature;
            weatherUpdateInfo.humidity = weather.humidity;
            weatherUpdateInfo.visibility = weather.visibility;
            weatherUpdateInfo.precipitation = weather.precipitation;
            emit weatherUpdate(true, coordinate, weatherUpdateInfo);

        } else {
            // TODO: error handling
            emit weatherUpdate(false, coordinate, WeatherInformation{});
        }
    });

}

void AirMapManager::_settingsChanged()
{
    qCDebug(AirMapManagerLog) << "AirMap settings changed";

    AirMapSettings* ap = _toolbox->settingsManager()->airMapSettings();

    AirMapSharedState::Settings settings;
    settings.apiKey = ap->apiKey()->rawValueString();
    bool apiKeyChanged = settings.apiKey != _shared.settings().apiKey;
    settings.clientID = ap->clientID()->rawValueString();
    settings.userName = ap->userName()->rawValueString();
    settings.password = ap->password()->rawValueString();
    _shared.setSettings(settings);

    // need to re-create the client if the API key changed
    if (_shared.client() && apiKeyChanged) {
        delete _shared.client();
        _shared.setClient(nullptr);
    }


    if (!_shared.client() && settings.apiKey != "") {
        qCDebug(AirMapManagerLog) << "Creating AirMap client";

        auto credentials    = Credentials{};
        credentials.api_key = _shared.settings().apiKey.toStdString();
        auto configuration  = Client::default_staging_configuration(credentials);
        qt::Client::create(configuration, _dispatchingLogger, this, [this, ap](const qt::Client::CreateResult& result) {
            if (result) {
                qCDebug(AirMapManagerLog) << "Successfully created airmap::qt::Client instance";
                _shared.setClient(result.value());

            } else {
                qWarning("Failed to create airmap::qt::Client instance");
                // TODO: user error message
            }
        });
    }
}

AirspaceManagerPerVehicle* AirMapManager::instantiateVehicle(const Vehicle& vehicle)
{
    AirMapManagerPerVehicle* manager = new AirMapManagerPerVehicle(_shared, vehicle, *_toolbox);
    //connect(manager, &AirMapManagerPerVehicle::networkError, this, &AirMapManager::_networkError);
    return manager;
}

AirspaceRestrictionProvider* AirMapManager::instantiateRestrictionProvider()
{
    AirMapRestrictionManager* restrictionManager = new AirMapRestrictionManager(_shared);
    //connect(restrictionManager, &AirMapRestrictionManager::networkError, this, &AirMapManager::_networkError);
    connect(restrictionManager, &AirMapRestrictionManager::error, this, &AirMapManager::_error);
    return restrictionManager;
}

