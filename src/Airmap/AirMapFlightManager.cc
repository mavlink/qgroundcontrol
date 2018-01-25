/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirMapFlightManager.h"

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

                    QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                    emit error("Failed to create Flight Plan",
                            QString::fromStdString(result.error().message()), description);
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

        if (result && result.value().flights.size() > 0) {

            Q_ASSERT(_shared.loginToken() != ""); // at this point we know the user is logged in (we queried the pilot id)

            Flights::EndFlight::Parameters params;
            params.authorization = _shared.loginToken().toStdString();
            params.id = result.value().flights[0].id; // pick the first flight (TODO: match the vehicle id)
            _shared.client()->flights().end_flight(params, [this, isAlive](const Flights::EndFlight::Result& result) {
                if (!isAlive.lock()) return;
                if (_state != State::EndFirstFlight) return;

                if (!result) {
                    QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                    emit error("Failed to end first Flight",
                            QString::fromStdString(result.error().message()), description);
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
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                emit error("Flight Plan creation failed",
                        QString::fromStdString(result.error().message()), description);
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
            for (const auto& validation : result.value().evaluation.validations) {
                if (validation.status != Evaluation::Validation::Status::valid) {
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
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Brief Request failed",
                    QString::fromStdString(result.error().message()), description);
            _state = State::Idle;
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
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to submit Flight Plan",
                    QString::fromStdString(result.error().message()), description);
            _state = State::Idle;
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
            for (const auto& authorization : briefing.evaluation.authorizations) {
                switch (authorization.status) {
                case Evaluation::Authorization::Status::accepted:
                case Evaluation::Authorization::Status::accepted_upon_submission:
                    accepted = true;
                    break;
                case Evaluation::Authorization::Status::rejected:
                case Evaluation::Authorization::Status::rejected_upon_submission:
                    rejected = true;
                    break;
                case Evaluation::Authorization::Status::pending:
                    pending = true;
                    break;
                }
            }

            if (briefing.evaluation.authorizations.size() == 0) {
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
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Brief Request failed",
                    QString::fromStdString(result.error().message()), description);
            _state = State::Idle;
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
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to end Flight",
                    QString::fromStdString(result.error().message()), description);
        }
    });
}


