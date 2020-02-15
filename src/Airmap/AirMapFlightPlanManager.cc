/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapFlightPlanManager.h"
#include "AirMapManager.h"
#include "AirMapRulesetsManager.h"
#include "AirMapAdvisoryManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

#include "PlanMasterController.h"
#include "QGCMAVLink.h"

#include "airmap/date_time.h"
#include "airmap/flight_plans.h"
#include "airmap/flights.h"
#include "airmap/geometry.h"
#include "airmap/pilots.h"

using namespace airmap;

//-----------------------------------------------------------------------------
AirMapFlightAuthorization::AirMapFlightAuthorization(const Evaluation::Authorization auth, QObject *parent)
    : AirspaceFlightAuthorization(parent)
    , _auth(auth)
{
}

//-----------------------------------------------------------------------------
AirspaceFlightAuthorization::AuthorizationStatus
AirMapFlightAuthorization::status()
{
    switch(_auth.status) {
    case Evaluation::Authorization::Status::accepted:
        return AirspaceFlightAuthorization::Accepted;
    case Evaluation::Authorization::Status::rejected:
        return AirspaceFlightAuthorization::Rejected;
    case Evaluation::Authorization::Status::pending:
        return AirspaceFlightAuthorization::Pending;
    case Evaluation::Authorization::Status::accepted_upon_submission:
        return AirspaceFlightAuthorization::AcceptedOnSubmission;
    case Evaluation::Authorization::Status::rejected_upon_submission:
        return AirspaceFlightAuthorization::RejectedOnSubmission;
    }
    return AirspaceFlightAuthorization::Unknown;
}

//-----------------------------------------------------------------------------
AirMapFlightInfo::AirMapFlightInfo(const airmap::Flight& flight, QObject *parent)
    : AirspaceFlightInfo(parent)
    , _flight(flight)
{
    //-- Load bounding box geometry
    const Geometry& geometry = flight.geometry;
    if(geometry.type() == Geometry::Type::polygon) {
        const Geometry::Polygon& polygon = geometry.details_for_polygon();
        for (const auto& vertex :  polygon.outer_ring.coordinates) {
            QGeoCoordinate coord;
            if (vertex.altitude) {
                coord = QGeoCoordinate(vertex.latitude, vertex.longitude, vertex.altitude.get());
            } else {
                coord = QGeoCoordinate(vertex.latitude, vertex.longitude);
            }
            _boundingBox.append(QVariant::fromValue(coord));
        }
    }
}

//-----------------------------------------------------------------------------
QString
AirMapFlightInfo::createdTime()
{
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(airmap::milliseconds_since_epoch(_flight.created_at))).toString("yyyy MM dd - hh:mm:ss");
}

//-----------------------------------------------------------------------------
QString
AirMapFlightInfo::startTime()
{
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(airmap::milliseconds_since_epoch(_flight.start_time))).toString("yyyy MM dd - hh:mm:ss");
}

//-----------------------------------------------------------------------------
QDateTime
AirMapFlightInfo::qStartTime()
{
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(airmap::milliseconds_since_epoch(_flight.start_time)));
}

//-----------------------------------------------------------------------------
bool
AirMapFlightInfo::active()
{
    QDateTime end = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(airmap::milliseconds_since_epoch(_flight.end_time)));
    QDateTime now = QDateTime::currentDateTime();
    return end > now;
}

//-----------------------------------------------------------------------------
void
AirMapFlightInfo::setEndFlight(DateTime end)
{
    _flight.end_time = end;
    emit activeChanged();
}

//-----------------------------------------------------------------------------
QString
AirMapFlightInfo::endTime()
{
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(airmap::milliseconds_since_epoch(_flight.end_time))).toString("yyyy MM dd - hh:mm:ss");
}

//-----------------------------------------------------------------------------
AirMapFlightPlanManager::AirMapFlightPlanManager(AirMapSharedState& shared, QObject *parent)
    : AirspaceFlightPlanProvider(parent)
    , _shared(shared)
{
    connect(&_pollTimer, &QTimer::timeout, this, &AirMapFlightPlanManager::_pollBriefing);
    _flightStartTime = QDateTime::currentDateTime().addSecs(60);
}

//-----------------------------------------------------------------------------
AirMapFlightPlanManager::~AirMapFlightPlanManager()
{
    _advisories.deleteListAndContents();
    _rulesets.deleteListAndContents();
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::setFlightStartTime(QDateTime start)
{
    if(start < QDateTime::currentDateTime()) {
        start = QDateTime::currentDateTime().addSecs(1);
        setDirty(true);
    }
    if(_flightStartTime != start) {
        _flightStartTime = start;
        setDirty(true);
        emit flightStartTimeChanged();
    }
    qCDebug(AirMapManagerLog) << "Set time start time" << _flightStartTime;
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::setFlightStartsNow(bool now)
{
    _flightStartsNow = now;
    setDirty(true);
    emit flightStartsNowChanged();
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::setFlightDuration(int seconds)
{
    if(_flightDuration != seconds || _flightDuration < 30) {
        _flightDuration = seconds;
        if(_flightDuration < 30) {
            _flightDuration = 30;
        }
        setDirty(true);
        emit flightDurationChanged();
        qCDebug(AirMapManagerLog) << "Set time duration" << _flightDuration;
    }
}

//-----------------------------------------------------------------------------
QDateTime
AirMapFlightPlanManager::flightStartTime() const
{
    return _flightStartTime;
}

//-----------------------------------------------------------------------------
int
AirMapFlightPlanManager::flightDuration() const
{
    return _flightDuration;
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::startFlightPlanning(PlanMasterController *planController)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Will not create a flight";
        return;
    }

    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapFlightPlanManager::startFlightPlanning: State not idle";
        return;
    }

    //-- TODO: Check if there is an ongoing flight plan and do something about it (Delete it?)

    /*
     * if(!flightPlanID().isEmpty()) {
     *     do something;
     * }
     */

    if(!_planController) {
        _planController = planController;
        //-- Get notified of mission changes
        connect(planController->missionController(), &MissionController::missionBoundingCubeChanged, this, &AirMapFlightPlanManager::_missionChanged);
    }
    //-- Set initial flight start time
    setFlightStartTime(QDateTime::currentDateTime().addSecs(5 * 60));
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::submitFlightPlan()
{
    if(flightPlanID().isEmpty()) {
        qCWarning(AirMapManagerLog) << "Submit flight with no flight plan.";
        return;
    }
    _flightId.clear();
    emit flightIDChanged(_flightId);
    _state = State::FlightSubmit;
    FlightPlans::Submit::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id            = flightPlanID().toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flight_plans().submit(params, [this, isAlive](const FlightPlans::Submit::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightSubmit) return;
        if (result) {
            _flightPlan = result.value();
            _flightId = QString::fromStdString(_flightPlan.flight_id.get());
            _state = State::Idle;
            _pollBriefing();
            emit flightIDChanged(_flightId);
        } else {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to submit Flight Plan",
                    QString::fromStdString(result.error().message()), description);
            _state = State::Idle;
            _flightPermitStatus = AirspaceFlightPlanProvider::PermitRejected;
            emit flightPermitStatusChanged();
        }
    });
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::updateFlightPlan()
{
    //-- Are we enabled?
    if(!qgcApp()->toolbox()->settingsManager()->airMapSettings()->enableAirMap()->rawValue().toBool()) {
        return;
    }
    //-- Do we have a license?
    if(!_shared.hasAPIKey()) {
        return;
    }
    _flightPermitStatus = AirspaceFlightPlanProvider::PermitPending;
    emit flightPermitStatusChanged();
    setDirty(false);
    _updateFlightPlan(true);
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::endFlight(QString flightID)
{
    qCDebug(AirMapManagerLog) << "End flight";
    _flightToEnd = flightID;
    if (_shared.pilotID().isEmpty()) {
        //-- Need to get the pilot id
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
                    QString pilotID = QString::fromStdString(result.value().id);
                    _shared.setPilotID(pilotID);
                    qCDebug(AirMapManagerLog) << "Got Pilot ID:" << pilotID;
                    _state = State::Idle;
                    _endFlight();
                } else {
                    _state = State::Idle;
                    QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                    emit error("Failed to get pilot ID", QString::fromStdString(result.error().message()), description);
                    return;
                }
            });
        });
    } else {
        _endFlight();
    }
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_endFlight()
{
    if(_flightToEnd.isEmpty()) {
        qCDebug(AirMapManagerLog) << "End non existing flight";
        return;
    }
    qCDebug(AirMapManagerLog) << "End Flight. State:" << static_cast<int>(_state);
    if(_state != State::Idle) {
        QTimer::singleShot(100, this, &AirMapFlightPlanManager::_endFlight);
        return;
    }
    qCDebug(AirMapManagerLog) << "Ending flight:" << _flightToEnd;
    _state = State::FlightEnd;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    Flights::EndFlight::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id = _flightToEnd.toStdString();
    //-- End flight
    _shared.client()->flights().end_flight(params, [this, isAlive](const Flights::EndFlight::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightEnd) return;
        if (result) {
            qCDebug(AirMapManagerLog) << "Flight Ended";
            int idx = _flightList.findFlightID(_flightToEnd);
            if(idx >= 0) {
                AirMapFlightInfo* pInfo = qobject_cast<AirMapFlightInfo*>(_flightList.get(idx));
                if(pInfo) {
                    pInfo->setEndFlight(result.value().end_time);
                }
            }
        } else {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("End flight failed", QString::fromStdString(result.error().message()), description);
        }
        _flightToEnd.clear();
        _state = State::Idle;
    });
}

//-----------------------------------------------------------------------------
bool
AirMapFlightPlanManager::_collectFlightDtata()
{
    if(!_planController || !_planController->missionController()) {
        return false;
    }
    //-- Get flight bounding cube and prepare (box) polygon
    QGCGeoBoundingCube bc = *_planController->missionController()->travelBoundingCube();
    if(!bc.isValid() || (fabs(bc.area()) < 0.0001)) {
        //-- TODO: If single point, we need to set a point and a radius instead
        qCDebug(AirMapManagerLog) << "Not enough points for a flight plan.";
        return false;
    }
    _flight.maxAltitude   = static_cast<float>(fmax(bc.pointNW.altitude(), bc.pointSE.altitude()));
    _flight.takeoffCoord  = _planController->missionController()->takeoffCoordinate();
    _flight.coords        = bc.polygon2D();
    _flight.bc            = bc;
    emit missionAreaChanged();
    return true;
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_createFlightPlan()
{
    _flight.reset();

    //-- Get flight data
    if(!_collectFlightDtata()) {
        return;
    }

    qCDebug(AirMapManagerLog) << "About to create flight plan";
    qCDebug(AirMapManagerLog) << "Takeoff:     " << _flight.takeoffCoord;
    qCDebug(AirMapManagerLog) << "Bounding box:" << _flight.bc.pointNW << _flight.bc.pointSE;
    qCDebug(AirMapManagerLog) << "Flight Start:" << flightStartTime().toString();
    qCDebug(AirMapManagerLog) << "Flight Duration:  " << flightDuration();

    if (_shared.pilotID().isEmpty() && !_shared.settings().userName.isEmpty() && !_shared.settings().password.isEmpty()) {
        //-- Need to get the pilot id before uploading the flight plan
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
                    QString pilotID = QString::fromStdString(result.value().id);
                    _shared.setPilotID(pilotID);
                    qCDebug(AirMapManagerLog) << "Got Pilot ID:" << pilotID;
                    _state = State::Idle;
                    _uploadFlightPlan();
                } else {
                    _flightPermitStatus = AirspaceFlightPlanProvider::PermitNone;
                    emit flightPermitStatusChanged();
                    _state = State::Idle;
                    QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                    emit error("Failed to create Flight Plan", QString::fromStdString(result.error().message()), description);
                    return;
                }
            });
        });
    } else {
        _uploadFlightPlan();
    }

    _flightPermitStatus = AirspaceFlightPlanProvider::PermitPending;
    emit flightPermitStatusChanged();
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_updateRulesAndFeatures(std::vector<RuleSet::Id>& rulesets, std::unordered_map<std::string, RuleSet::Feature::Value>& features, bool updateFeatures)
{
    auto* pRulesMgr = qobject_cast<AirMapRulesetsManager*>(qgcApp()->toolbox()->airspaceManager()->ruleSets());
    if(pRulesMgr) {
        for(int rs = 0; rs < pRulesMgr->ruleSets()->count(); rs++) {
            AirMapRuleSet* ruleSet = qobject_cast<AirMapRuleSet*>(pRulesMgr->ruleSets()->get(rs));
            //-- If this ruleset is selected
            if(ruleSet && ruleSet->selected()) {
                rulesets.push_back(ruleSet->id().toStdString());
                //-- Features within each rule (only when updating)
                if(updateFeatures) {
                    for(int r = 0; r < ruleSet->rules()->count(); r++) {
                        AirMapRule* rule = qobject_cast<AirMapRule*>(ruleSet->rules()->get(r));
                        if(rule) {
                            for(int f = 0; f < rule->features()->count(); f++) {
                                AirMapRuleFeature* feature = qobject_cast<AirMapRuleFeature*>(rule->features()->get(f));
                                if(features.find(feature->name().toStdString()) != features.end()) {
                                    qCDebug(AirMapManagerLog) << "Removing duplicate:" << feature->name();
                                    continue;
                                }
                                if(feature && feature->value().isValid()) {
                                    switch(feature->type()) {
                                    case AirspaceRuleFeature::Boolean:
                                        if(feature->value().toInt() == 0 || feature->value().toInt() == 1) {
                                            features[feature->name().toStdString()] = RuleSet::Feature::Value(feature->value().toBool());
                                        } else {
                                            //-- If not set, default to false
                                            features[feature->name().toStdString()] = RuleSet::Feature::Value(false);
                                        }
                                        break;
                                    case AirspaceRuleFeature::Float:
                                        //-- Sanity check for floats
                                        if(std::isfinite(feature->value().toFloat())) {
                                            features[feature->name().toStdString()] = RuleSet::Feature::Value(feature->value().toDouble());
                                        }
                                        break;
                                    case AirspaceRuleFeature::String:
                                        //-- Skip empty responses
                                        if(!feature->value().toString().isEmpty()) {
                                            features[feature->name().toStdString()] = RuleSet::Feature::Value(feature->value().toString().toStdString());
                                        }
                                        break;
                                    default:
                                        qCWarning(AirMapManagerLog) << "Unknown type for feature" << feature->name();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_updateFlightStartEndTime(DateTime& start_time, DateTime& end_time)
{
    if(_flightStartsNow || _flightStartTime < QDateTime::currentDateTime()) {
        setFlightStartTime(QDateTime::currentDateTime().addSecs(1));
    }
    quint64 startt = static_cast<quint64>(_flightStartTime.toUTC().toMSecsSinceEpoch());
    start_time = airmap::from_milliseconds_since_epoch(airmap::milliseconds(static_cast<qint64>(startt)));
    quint64 endt = startt + (static_cast<uint64_t>(_flightDuration) * 1000);
    end_time = airmap::from_milliseconds_since_epoch(airmap::milliseconds(static_cast<qint64>(endt)));
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_uploadFlightPlan()
{
    qCDebug(AirMapManagerLog) << "Uploading flight plan. State:" << static_cast<int>(_state);
    if(_state != State::Idle) {
        QTimer::singleShot(100, this, &AirMapFlightPlanManager::_uploadFlightPlan);
        return;
    }
    //-- Reset "relevant" features
    _importantFeatures.clear();
    _state = State::FlightUpload;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.doRequestWithLogin([this, isAlive](const QString& login_token) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightUpload) return;
        FlightPlans::Create::Parameters params;
        params.max_altitude = _flight.maxAltitude;
        params.min_altitude = 0.0;
        params.buffer       = 10.f;
        params.latitude     = static_cast<float>(_flight.takeoffCoord.latitude());
        params.longitude    = static_cast<float>(_flight.takeoffCoord.longitude());
        params.pilot.id     = _shared.pilotID().toStdString();
        //-- Handle flight start/end
        _updateFlightStartEndTime(params.start_time, params.end_time);
        //-- Rules & Features
        _updateRulesAndFeatures(params.rulesets, params.features);
        //-- Geometry: polygon
        Geometry::Polygon polygon;
        for (const auto& qcoord : _flight.coords) {
            Geometry::Coordinate coord;
            coord.latitude  = qcoord.latitude();
            coord.longitude = qcoord.longitude();
            polygon.outer_ring.coordinates.push_back(coord);
        }
        params.geometry = Geometry(polygon);
        params.authorization = login_token.toStdString();
        //-- Create flight plan
        _shared.client()->flight_plans().create_by_polygon(params, [this, isAlive](const FlightPlans::Create::Result& result) {
            if (!isAlive.lock()) return;
            if (_state != State::FlightUpload) return;
            _state = State::Idle;
            if (result) {
                _flightPlan = result.value();
                qCDebug(AirMapManagerLog) << "Flight plan created:" << flightPlanID();
                _pollBriefing();
            } else {
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                emit error("Flight Plan creation failed", QString::fromStdString(result.error().message()), description);
            }
        });
    });
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_updateFlightPlanOnTimer()
{
    _updateFlightPlan(false);
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_updateFlightPlan(bool interactive)
{
    qCDebug(AirMapManagerLog) << "Updating flight plan. State:" << static_cast<int>(_state);

    if(_state != State::Idle) {
        QTimer::singleShot(250, this, &AirMapFlightPlanManager::_updateFlightPlanOnTimer);
        return;
    }
    //-- Get flight data
    if(!_collectFlightDtata()) {
        return;
    }

    //-- Update local instance of the flight plan
    _flightPlan.altitude_agl.max  = _flight.maxAltitude;
    _flightPlan.altitude_agl.min  = 0.0f;
    _flightPlan.buffer            = 2.f;
    _flightPlan.takeoff.latitude  = static_cast<float>(_flight.takeoffCoord.latitude());
    _flightPlan.takeoff.longitude = static_cast<float>(_flight.takeoffCoord.longitude());
    //-- Rules & Features
    _flightPlan.rulesets.clear();
    _flightPlan.features.clear();
    //-- If interactive, we collect features otherwise we don't
    _updateRulesAndFeatures(_flightPlan.rulesets, _flightPlan.features, interactive);
    //-- Handle flight start/end
    _updateFlightStartEndTime(_flightPlan.start_time, _flightPlan.end_time);
    //-- Geometry: polygon
    Geometry::Polygon polygon;
    for (const auto& qcoord : _flight.coords) {
        Geometry::Coordinate coord;
        coord.latitude  = qcoord.latitude();
        coord.longitude = qcoord.longitude();
        polygon.outer_ring.coordinates.push_back(coord);
    }
    _flightPlan.geometry = Geometry(polygon);

    qCDebug(AirMapManagerLog) << "Takeoff:        " << _flight.takeoffCoord;
    qCDebug(AirMapManagerLog) << "Bounding box:   " << _flight.bc.pointNW << _flight.bc.pointSE;
    qCDebug(AirMapManagerLog) << "Flight Start:   " << flightStartTime().toString();
    qCDebug(AirMapManagerLog) << "Flight Duration:" << flightDuration();

    _state = State::FlightUpdate;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.doRequestWithLogin([this, isAlive](const QString& login_token) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightUpdate) return;
        FlightPlans::Update::Parameters params = {};
        params.authorization                 = login_token.toStdString();
        params.flight_plan                   = _flightPlan;
        //-- Update flight plan
        _shared.client()->flight_plans().update(params, [this, isAlive](const FlightPlans::Update::Result& result) {
            if (!isAlive.lock()) return;
            if (_state != State::FlightUpdate) return;
            _state = State::Idle;
            if (result) {
                qCDebug(AirMapManagerLog) << "Flight plan updated:" << flightPlanID();
                _pollBriefing();
            } else {
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                emit error("Flight Plan update failed", QString::fromStdString(result.error().message()), description);
            }
        });
    });
}

//-----------------------------------------------------------------------------
static bool
adv_sort(QObject* a, QObject* b)
{
    AirMapAdvisory* aa = qobject_cast<AirMapAdvisory*>(a);
    AirMapAdvisory* bb = qobject_cast<AirMapAdvisory*>(b);
    if(!aa || !bb) return false;
    return static_cast<int>(aa->color()) > static_cast<int>(bb->color());
}

//-----------------------------------------------------------------------------
static bool
rules_sort(QObject* a, QObject* b)
{
    AirMapRule* aa = qobject_cast<AirMapRule*>(a);
    AirMapRule* bb = qobject_cast<AirMapRule*>(b);
    if(!aa || !bb) return false;
    return static_cast<int>(aa->status()) > static_cast<int>(bb->status());
}

//-----------------------------------------------------------------------------
bool
AirMapFlightPlanManager::_findBriefFeature(const QString& name)
{
    for(int i = 0; i < _briefFeatures.count(); i++ ) {
        AirMapRuleFeature* feature = qobject_cast<AirMapRuleFeature*>(_briefFeatures.get(i));
        if (feature && feature->name() == name) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_pollBriefing()
{
    qCDebug(AirMapManagerLog) << "Poll Briefing. State:" << static_cast<int>(_state);
    if(_state != State::Idle) {
        QTimer::singleShot(100, this, &AirMapFlightPlanManager::_pollBriefing);
        return;
    }
    _state = State::FlightPolling;
    FlightPlans::RenderBriefing::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    params.id            = flightPlanID().toStdString();
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->flight_plans().render_briefing(params, [this, isAlive](const FlightPlans::RenderBriefing::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::FlightPolling) return;
        if (result) {
            const FlightPlan::Briefing& briefing = result.value();
            qCDebug(AirMapManagerLog) << "Flight polling/briefing response";
            //-- Collect advisories
            _valid = false;
            _advisories.clearAndDeleteContents();
            const std::vector<Status::Advisory> advisories = briefing.airspace.advisories;
            _airspaceColor = static_cast<AirspaceAdvisoryProvider::AdvisoryColor>(briefing.airspace.color);
            for (const auto& advisory : advisories) {
                AirMapAdvisory* pAdvisory = new AirMapAdvisory(this);
                pAdvisory->_id          = QString::fromStdString(advisory.airspace.id());
                pAdvisory->_name        = QString::fromStdString(advisory.airspace.name());
                pAdvisory->_type        = static_cast<AirspaceAdvisory::AdvisoryType>(advisory.airspace.type());
                pAdvisory->_color       = static_cast<AirspaceAdvisoryProvider::AdvisoryColor>(advisory.color);
                _advisories.append(pAdvisory);
                qCDebug(AirMapManagerLog) << "Adding briefing advisory" << pAdvisory->name();
            }
            //-- Sort in order of color (priority)
            _advisories.beginReset();
            std::sort(_advisories.objectList()->begin(), _advisories.objectList()->end(), adv_sort);
            _advisories.endReset();
            _valid = true;
            //-- Collect Rulesets
            _authorizations.clearAndDeleteContents();
            _rulesViolation.clearAndDeleteContents();
            _rulesInfo.clearAndDeleteContents();
            _rulesReview.clearAndDeleteContents();
            _rulesFollowing.clearAndDeleteContents();
            _briefFeatures.clear();
            for(const auto& ruleset : briefing.evaluation.rulesets) {
                AirMapRuleSet* pRuleSet = new AirMapRuleSet(this);
                pRuleSet->_id = QString::fromStdString(ruleset.id);
                //-- Iterate Rules
                for (const auto& rule : ruleset.rules) {
                    AirMapRule* pRule = new AirMapRule(rule, this);
                    //-- Iterate Rule Features
                    for (const auto& feature : rule.features) {
                        AirMapRuleFeature* pFeature = new AirMapRuleFeature(feature, this);
                        pRule->_features.append(pFeature);
                        if(rule.status == RuleSet::Rule::Status::missing_info) {
                            if(!_findBriefFeature(pFeature->name())) {
                                _briefFeatures.append(pFeature);
                                _importantFeatures.append(pFeature);
                                qCDebug(AirMapManagerLog) << "Adding briefing feature" << pFeature->name() << pFeature->description() << pFeature->type();
                            } else {
                                qCDebug(AirMapManagerLog) << "Skipping briefing feature duplicate" << pFeature->name() << pFeature->description() << pFeature->type();
                            }
                        }
                    }
                    //-- When a flight is first created, we send no features. That means that all "missing_info" are "relevant" features.
                    //   We keep a list of them so they will be always shown to the user even when they are no longer "missing_info"
                    for(const auto& feature : _importantFeatures) {
                        if(!_findBriefFeature(feature->name())) {
                            _briefFeatures.append(feature);
                        }
                    }
                    pRuleSet->_rules.append(pRule);
                    //-- Rules separated by status for presentation
                    switch(rule.status) {
                    case RuleSet::Rule::Status::conflicting:
                        _rulesViolation.append(new AirMapRule(rule, this));
                        break;
                    case RuleSet::Rule::Status::not_conflicting:
                        _rulesFollowing.append(new AirMapRule(rule, this));
                        break;
                    case RuleSet::Rule::Status::missing_info:
                        _rulesInfo.append(new AirMapRule(rule, this));
                        break;
                    case RuleSet::Rule::Status::informational:
                        _rulesReview.append(new AirMapRule(rule, this));
                        break;
                    default:
                        break;
                    }
                }
                //-- Sort rules by relevance order
                pRuleSet->_rules.beginReset();
                std::sort(pRuleSet->_rules.objectList()->begin(), pRuleSet->_rules.objectList()->end(), rules_sort);
                pRuleSet->_rules.endReset();
                _rulesets.append(pRuleSet);
                qCDebug(AirMapManagerLog) << "Adding briefing ruleset" << pRuleSet->id();
            }
            //-- Evaluate briefing status
            if (briefing.evaluation.authorizations.size() == 0) {
                _flightPermitStatus = AirspaceFlightPlanProvider::PermitNotRequired;
                emit flightPermitStatusChanged();
            } else {
                bool rejected = false;
                bool accepted = false;
                bool pending  = false;
                for (const auto& authorization : briefing.evaluation.authorizations) {
                    AirMapFlightAuthorization* pAuth = new AirMapFlightAuthorization(authorization, this);
                    _authorizations.append(pAuth);
                    qCDebug(AirMapManagerLog) << "Autorization:" << pAuth->name() << " (" << pAuth->message() << ")" << static_cast<int>(pAuth->status());
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
                qCDebug(AirMapManagerLog) << "Flight approval: accepted=" << accepted << "rejected" << rejected << "pending" << pending;
                if ((rejected || accepted) && !pending) {
                    if (rejected) { // rejected has priority
                        _flightPermitStatus = AirspaceFlightPlanProvider::PermitRejected;
                    } else {
                        _flightPermitStatus = AirspaceFlightPlanProvider::PermitAccepted;
                    }
                    emit flightPermitStatusChanged();
                } else {
                    //-- Pending. Try again.
                    _pollTimer.setSingleShot(true);
                    _pollTimer.start(1000);
                }
            }
            emit advisoryChanged();
            emit rulesChanged();
            _state = State::Idle;
        } else {
            _state = State::Idle;
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Brief Request failed",
                    QString::fromStdString(result.error().message()), description);
        }
    });
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_missionChanged()
{
    //-- Are we enabled?
    if(!qgcApp()->toolbox()->settingsManager()->airMapSettings()->enableAirMap()->rawValue().toBool()) {
        return;
    }
    //-- Do we have a license?
    if(!_shared.hasAPIKey()) {
        return;
    }
    //-- Creating a new flight plan?
    if(_state == State::Idle) {
        if(flightPlanID().isEmpty()) {
            _createFlightPlan();
        } else {
            //-- Plan is being modified
            _updateFlightPlan();
        }
    }
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::loadFlightList(QDateTime startTime, QDateTime endTime)
{
    //-- TODO: This is not checking if the state is Idle. Again, these need to
    //   queued up and handled by a worker thread.
    qCDebug(AirMapManagerLog) << "Preparing load flight list";
    _loadingFlightList = true;
    emit loadingFlightListChanged();
    _rangeStart = startTime;
    _rangeEnd   = endTime;
    qCDebug(AirMapManagerLog) << "List flights from:" << _rangeStart.toString("yyyy MM dd - hh:mm:ss") << "to" << _rangeEnd.toString("yyyy MM dd - hh:mm:ss");
    if (_shared.pilotID().isEmpty()) {
        //-- Need to get the pilot id
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
                    QString pilotID = QString::fromStdString(result.value().id);
                    _shared.setPilotID(pilotID);
                    qCDebug(AirMapManagerLog) << "Got Pilot ID:" << pilotID;
                    _state = State::Idle;
                    _loadFlightList();
                } else {
                    _state = State::Idle;
                    QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                    emit error("Failed to get pilot ID", QString::fromStdString(result.error().message()), description);
                    _loadingFlightList = false;
                    emit loadingFlightListChanged();
                    return;
                }
            });
        });
    } else {
        _loadFlightList();
    }
}

//-----------------------------------------------------------------------------
void
AirMapFlightPlanManager::_loadFlightList()
{
    qCDebug(AirMapManagerLog) << "Load flight list. State:" << static_cast<int>(_state);
    if(_state != State::Idle) {
        QTimer::singleShot(100, this, &AirMapFlightPlanManager::_loadFlightList);
        return;
    }
    _flightList.clear();
    emit flightListChanged();
    _state = State::LoadFlightList;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.doRequestWithLogin([this, isAlive](const QString& login_token) {
        if (!isAlive.lock()) return;
        if (_state != State::LoadFlightList) return;
        Flights::Search::Parameters params;
        params.authorization = login_token.toStdString();
        quint64 start   = static_cast<quint64>(_rangeStart.toUTC().toMSecsSinceEpoch());
        quint64 end     = static_cast<quint64>(_rangeEnd.toUTC().toMSecsSinceEpoch());
        params.start_after  = airmap::from_milliseconds_since_epoch(airmap::milliseconds(static_cast<long long>(start)));
        params.start_before = airmap::from_milliseconds_since_epoch(airmap::milliseconds(static_cast<long long>(end)));
        params.limit    = 250;
        params.pilot_id = _shared.pilotID().toStdString();
        _shared.client()->flights().search(params, [this, isAlive](const Flights::Search::Result& result) {
            if (!isAlive.lock()) return;
            if (_state != State::LoadFlightList) return;
            if (result && result.value().flights.size() > 0) {
                const Flights::Search::Response& response = result.value();
                for (const auto& flight : response.flights) {
                    AirMapFlightInfo* pFlight = new AirMapFlightInfo(flight, this);
                    _flightList.append(pFlight);
                    qCDebug(AirMapManagerLog) << "Found:" << pFlight->flightID() << pFlight->flightPlanID() << pFlight->endTime();
                }
                _flightList.sortStartFlight();
                emit flightListChanged();
            } else {
                if(!result) {
                    QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                    emit error("Flight search failed", QString::fromStdString(result.error().message()), description);
                }
            }
            _state = State::Idle;
            _loadingFlightList = false;
            emit loadingFlightListChanged();
        });
    });
}

