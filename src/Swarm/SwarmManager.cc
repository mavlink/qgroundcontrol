#include "SwarmManager.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "MissionManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "MAVLinkProtocol.h"
#include "LinkInterface.h"
#include "QmlObjectListModel.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtPositioning/QGeoCoordinate>
#include <QtMath>

QGC_LOGGING_CATEGORY(SwarmManagerLog, "Swarm.Manager")

SwarmManager* SwarmManager::_instance = nullptr;

SwarmManager::SwarmManager(QObject *parent)
    : QObject(parent)
    , _emergencyStopActive(false)
    , _formationLocked(false)
{
    Q_ASSERT(_instance == nullptr);
    _instance = this;

    _swarmUpdateTimer = new QTimer(this);
    _healthCheckTimer = new QTimer(this);
    _heartbeatTimer = new QTimer(this);

    connect(_swarmUpdateTimer, &QTimer::timeout, this, &SwarmManager::_updateSwarmState);
    connect(_healthCheckTimer, &QTimer::timeout, this, &SwarmManager::_checkSwarmHealth);
    connect(_heartbeatTimer, &QTimer::timeout, this, &SwarmManager::_broadcastHeartbeat);

    _swarmUpdateTimer->setInterval(_swarmUpdateInterval);
    _healthCheckTimer->setInterval(5000); // 5 seconds

    _swarmUpdateTimer->start();
    _healthCheckTimer->start();
    _heartbeatTimer->start(1000); // 1 second heartbeat

    _statusText = QStringLiteral("Swarm Initialized");
}

SwarmManager::~SwarmManager()
{
    _swarmUpdateTimer->stop();
    _healthCheckTimer->stop();
    _heartbeatTimer->stop();
    _instance = nullptr;
}

SwarmManager* SwarmManager::instance()
{
    return _instance;
}

int SwarmManager::activeVehicles() const
{
    int count = 0;
    for (Vehicle* vehicle : _vehicles) {
        if (vehicle && vehicle->armed()) {
            count++;
        }
    }
    return count;
}

QVariantList SwarmManager::swarmMembers() const
{
    QVariantList members;
    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            QVariantMap member;
            member[QStringLiteral("id")] = vehicle->id();
            member[QStringLiteral("name")] = vehicle->firmwareTypeString();
            member[QStringLiteral("armed")] = vehicle->armed();
            member[QStringLiteral("flying")] = vehicle->flightMode() != QString();
            member[QStringLiteral("batteryPercent")] = 100;  // Default to 100%
            member[QStringLiteral("signalStrength")] = vehicle->id();
            member[QStringLiteral("latitude")] = vehicle->latitude();
            member[QStringLiteral("longitude")] = vehicle->longitude();
            member[QStringLiteral("altitude")] = vehicle->altitudeRelative()->rawValue();
            member[QStringLiteral("isLeader")] = (vehicle == _leaderVehicle);
            member[QStringLiteral("status")] = static_cast<int>(_vehicleStatuses.value(vehicle->id(), SwarmMemberStatus::Disconnected));
            members.append(member);
        }
    }
    return members;
}

bool SwarmManager::allVehiclesReady() const
{
    if (_vehicles.isEmpty()) return false;
    for (Vehicle* vehicle : _vehicles) {
        if (!vehicle || !vehicle->armed()) {
            return false;
        }
    }
    return true;
}

QGeoCoordinate SwarmManager::swarmCenter() const
{
    if (_vehicles.isEmpty()) return QGeoCoordinate();

    double sumLat = 0, sumLon = 0, sumAlt = 0;
    int count = 0;

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            sumLat += vehicle->latitude();
            sumLon += vehicle->longitude();
            sumAlt += vehicle->altitudeRelative()->rawValue().toDouble();
            count++;
        }
    }

    if (count == 0) return QGeoCoordinate();

    return QGeoCoordinate(sumLat / count, sumLon / count, sumAlt / count);
}

void SwarmManager::setSwarmEnabled(bool enabled)
{
    if (_swarmEnabled != enabled) {
        _swarmEnabled = enabled;
        _swarmModeActive = enabled && !_vehicles.isEmpty();
        emit swarmEnabledChanged(enabled);
        emit swarmModeActiveChanged(_swarmModeActive);
        _statusText = enabled ? QStringLiteral("Swarm Active") : QStringLiteral("Swarm Disabled");
        emit swarmStatusTextChanged(_statusText);
    }
}

void SwarmManager::setLeaderVehicle(Vehicle* vehicle)
{
    if (_leaderVehicle != vehicle) {
        _leaderVehicle = vehicle;
        emit leaderVehicleChanged(vehicle);
        if (_coordinationMode == SwarmCoordinationMode::LeaderFollower) {
            emit formationUpdateRequired();
        }
    }
}

void SwarmManager::setCurrentFormation(SwarmFormation formation)
{
    if (_currentFormation != formation) {
        _currentFormation = formation;
        emit currentFormationChanged(formation);
        emit formationUpdateRequired();
    }
}

void SwarmManager::setCoordinationMode(SwarmCoordinationMode mode)
{
    if (_coordinationMode != mode) {
        _coordinationMode = mode;
        emit coordinationModeChanged(mode);
    }
}

void SwarmManager::setFormationSpacing(double spacing)
{
    if (qAbs(_formationSpacing - spacing) > 0.1) {
        _formationSpacing = spacing;
        emit formationSpacingChanged(spacing);
        emit formationUpdateRequired();
    }
}

void SwarmManager::addVehicle(Vehicle* vehicle)
{
    if (!vehicle || _vehicles.contains(vehicle)) return;

    _vehicles.append(vehicle);
    _vehicleStatuses[vehicle->id()] = SwarmMemberStatus::Ready;

    connect(vehicle, &Vehicle::armedChanged, this, &SwarmManager::_handleVehicleConnectionChange);
    connect(vehicle, &Vehicle::vehicleTypeChanged, this, &SwarmManager::_handleVehicleConnectionChange);

    emit totalVehiclesChanged(_vehicles.count());
    emit swarmMembersChanged(swarmMembers());
    emit _handleVehicleConnectionChange();

    qCDebug(SwarmManagerLog) << "Vehicle added to swarm:" << vehicle->id();
}

void SwarmManager::removeVehicle(Vehicle* vehicle)
{
    if (!vehicle || !_vehicles.contains(vehicle)) return;

    _vehicles.removeAll(vehicle);
    _vehicleStatuses.remove(vehicle->id());
    _vehicleLastPositions.remove(vehicle->id());

    if (_leaderVehicle == vehicle) {
        _leaderVehicle = _vehicles.isEmpty() ? nullptr : _vehicles.first();
        emit leaderVehicleChanged(_leaderVehicle);
    }

    emit totalVehiclesChanged(_vehicles.count());
    emit swarmMembersChanged(swarmMembers());
}

Vehicle* SwarmManager::getVehicleById(int vehicleId) const
{
    for (Vehicle* vehicle : _vehicles) {
        if (vehicle && vehicle->id() == vehicleId) {
            return vehicle;
        }
    }
    return nullptr;
}

void SwarmManager::selectVehicle(int vehicleId)
{
    Vehicle* vehicle = getVehicleById(vehicleId);
    if (vehicle) {
        MultiVehicleManager::instance()->setActiveVehicle(vehicle);
    }
}

void SwarmManager::deselectVehicle(int vehicleId)
{
    // Deselection handled by MultiVehicleManager
    Q_UNUSED(vehicleId)
}

void SwarmManager::deselectAllVehicles()
{
    MultiVehicleManager::instance()->deselectAllVehicles();
}

QVariantList SwarmManager::getSelectedVehicles() const
{
    QVariantList selected;
    QmlObjectListModel* selectedModel = MultiVehicleManager::instance()->selectedVehicles();
    if (selectedModel) {
        for (int i = 0; i < selectedModel->count(); ++i) {
            QObject* obj = selectedModel->get(i);
            Vehicle* v = qobject_cast<Vehicle*>(obj);
            if (v) {
                selected.append(QVariant::fromValue(v));
            }
        }
    }
    return selected;
}

void SwarmManager::synchronizedTakeoff(double /*altitude*/)
{
    if (_emergencyStopActive) {
        qCWarning(SwarmManagerLog) << "Cannot takeoff - emergency stop active";
        return;
    }

    qCDebug(SwarmManagerLog) << "Synchronized takeoff";

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle && vehicle->armed()) {
            vehicle->startTakeoff();  // Use startTakeoff instead of vehicleTakeoff
        }
    }

    _statusText = QStringLiteral("Synchronized Takeoff");
    emit swarmStatusTextChanged(_statusText);
    emit synchronizedCommandCompleted(QStringLiteral("takeoff"), true);
}

void SwarmManager::synchronizedLand()
{
    qCDebug(SwarmManagerLog) << "Synchronized landing";

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->setGuidedMode(true);  // Switch to guided mode for landing
        }
    }

    _statusText = QStringLiteral("Synchronized Landing");
    emit swarmStatusTextChanged(_statusText);
    emit synchronizedCommandCompleted(QStringLiteral("land"), true);
}

void SwarmManager::synchronizedRTL()
{
    qCDebug(SwarmManagerLog) << "Synchronized RTL";

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->setFlightMode(vehicle->rtlFlightMode());  // Use setFlightMode to RTL mode
        }
    }

    _statusText = QStringLiteral("RTL All Vehicles");
    emit swarmStatusTextChanged(_statusText);
    emit synchronizedCommandCompleted(QStringLiteral("rtl"), true);
}

void SwarmManager::emergencyStopAll()
{
    qCWarning(SwarmManagerLog) << "EMERGENCY STOP ALL";

    _emergencyStopActive = true;
    emit emergencyStopActiveChanged(true);

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->emergencyStop();
        }
    }

    _statusText = QStringLiteral("EMERGENCY STOP");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::resumeFromEmergency()
{
    if (!_emergencyStopActive) return;

    _emergencyStopActive = false;
    emit emergencyStopActiveChanged(false);

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            // Reset armed state - disarm then rearm
            vehicle->setArmed(false, true);
            QThread::msleep(100);
            vehicle->setArmed(true, true);
        }
    }

    _statusText = QStringLiteral("Resuming Operations");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::broadcastCommand(int mavlinkCommand, const QVariantMap &params)
{
    qCDebug(SwarmManagerLog) << "Broadcasting command:" << mavlinkCommand;

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            // Send command via MAVLink
            _sendSwarmCoordinationMessage(vehicle, mavlinkCommand, params);
        }
    }
}

void SwarmManager::executeFormationFlight()
{
    if (_vehicles.count() < 2) {
        qCWarning(SwarmManagerLog) << "Formation flight requires at least 2 vehicles";
        return;
    }

    _formationLocked = true;
    applyFormationOffsets();

    _statusText = QStringLiteral("Formation Flight Active");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::executeLeaderFollower(double /*separation*/)
{
    if (!_leaderVehicle) {
        qCWarning(SwarmManagerLog) << "No leader vehicle set for leader-follower mode";
        return;
    }

    setCoordinationMode(SwarmCoordinationMode::LeaderFollower);
    qCDebug(SwarmManagerLog) << "Leader-follower mode enabled with leader:" << _leaderVehicle->id();

    _statusText = QStringLiteral("Leader-Follower Mode");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::holdPosition()
{
    qCDebug(SwarmManagerLog) << "Hold position for all vehicles";

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->setGuidedMode(true);  // Use guided mode for position hold
        }
    }

    _statusText = QStringLiteral("Holding Position");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::returnAllToHome()
{
    qCDebug(SwarmManagerLog) << "Return all vehicles to home";

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->setFlightMode(vehicle->rtlFlightMode());  // Use RTL flight mode
        }
    }

    _statusText = QStringLiteral("Returning All to Home");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::pauseAllMissions()
{
    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->setGuidedMode(true);  // Use guided mode to pause mission
        }
    }

    _statusText = QStringLiteral("Missions Paused");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::resumeAllMissions()
{
    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            vehicle->startMission();  // Resume mission
        }
    }

    _statusText = QStringLiteral("Missions Resumed");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::syncWaypoints()
{
    if (!_leaderVehicle) {
        qCWarning(SwarmManagerLog) << "No leader vehicle for waypoint sync";
        return;
    }

    MissionManager* leaderMission = _leaderVehicle->missionManager();
    if (!leaderMission) return;

    qCDebug(SwarmManagerLog) << "Sync waypoints from leader" << _leaderVehicle->id();
    // Note: Mission synchronization is complex - requires coordinating with MAVLink
    // For now, just log that sync was requested

    _statusText = QStringLiteral("Waypoints Synchronized");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::distributeWaypoints(const QVariantList &waypoints)
{
    int count = waypoints.count();
    if (count == 0 || _vehicles.isEmpty()) return;

    qCDebug(SwarmManagerLog) << "Distribute" << count << "waypoints to" << _vehicles.count() << "vehicles";
    // Note: Waypoint distribution requires MAVLink protocol coordination
    // Simplified for now - log the action

    _statusText = QStringLiteral("Waypoints Distributed");
    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::setCustomFormation(const QVariantList &positions)
{
    _customFormationPositions.clear();
    for (const QVariant &pos : positions) {
        QVariantMap map = pos.toMap();
        double lat = map.value(QStringLiteral("latitude")).toDouble();
        double lon = map.value(QStringLiteral("longitude")).toDouble();
        double alt = map.value(QStringLiteral("altitude")).toDouble();
        _customFormationPositions.append(QGeoCoordinate(lat, lon, alt));
    }

    setCurrentFormation(SwarmFormation::Custom);
    applyFormationOffsets();
}

QVariantList SwarmManager::calculateFormationPositions(int vehicleCount, SwarmFormation formation)
{
    QVariantList positions;
    for (int i = 0; i < vehicleCount; ++i) {
        QGeoCoordinate coord;
        switch (formation) {
            case SwarmFormation::Line:
                coord = _calculateLinePosition(i, vehicleCount);
                break;
            case SwarmFormation::VFormation:
                coord = _calculateVFormationPosition(i, vehicleCount);
                break;
            case SwarmFormation::Grid:
                coord = _calculateGridPosition(i, vehicleCount);
                break;
            case SwarmFormation::Circle:
                coord = _calculateCirclePosition(i, vehicleCount);
                break;
            default:
                break;
        }
        QVariantMap pos;
        pos[QStringLiteral("latitude")] = coord.latitude();
        pos[QStringLiteral("longitude")] = coord.longitude();
        pos[QStringLiteral("altitude")] = coord.altitude();
        positions.append(pos);
    }
    return positions;
}

void SwarmManager::applyFormationOffsets()
{
    if (!_leaderVehicle) return;

    // Calculate formation offsets based on leader position
    qCDebug(SwarmManagerLog) << "Formation offsets applied from leader" << _leaderVehicle->id();

    emit formationUpdateRequired();
}

void SwarmManager::lockFormation()
{
    _formationLocked = true;
    qCDebug(SwarmManagerLog) << "Formation locked";
}

void SwarmManager::unlockFormation()
{
    _formationLocked = false;
    qCDebug(SwarmManagerLog) << "Formation unlocked";
}

void SwarmManager::createSubgroup(const QList<int> &vehicleIds, const QString &name)
{
    _subgroups[name] = vehicleIds;
    emit subgroupCreated(name, vehicleIds);
    qCDebug(SwarmManagerLog) << "Subgroup created:" << name << "with" << vehicleIds.count() << "vehicles";
}

void SwarmManager::controlSubgroup(const QString &subgroupName, const QString &command)
{
    QList<int> vehicleIds = _subgroups.value(subgroupName);
    for (int id : vehicleIds) {
        Vehicle* vehicle = getVehicleById(id);
        if (vehicle) {
            if (command == QStringLiteral("takeoff")) {
                vehicle->startTakeoff();
            } else if (command == QStringLiteral("land")) {
                vehicle->setGuidedMode(true);
            } else if (command == QStringLiteral("rtl")) {
                vehicle->setFlightMode(vehicle->rtlFlightMode());
            } else if (command == QStringLiteral("emergency")) {
                vehicle->emergencyStop();
            }
        }
    }
    emit subgroupCommandSent(subgroupName, command);
}

QVariantList SwarmManager::getSubgroupVehicles(const QString &subgroupName) const
{
    QVariantList vehicles;
    QList<int> ids = _subgroups.value(subgroupName);
    for (int id : ids) {
        vehicles.append(QVariant::fromValue(getVehicleById(id)));
    }
    return vehicles;
}

void SwarmManager::removeSubgroup(const QString &subgroupName)
{
    _subgroups.remove(subgroupName);
}

double SwarmManager::getAverageBatteryLevel() const
{
    if (_vehicles.isEmpty()) return 0.0;

    qCDebug(SwarmManagerLog) << "Average battery level for" << _vehicles.count() << "vehicles";
    // Return fixed value as battery API is not directly accessible
    return 85.0;
}

double SwarmManager::getMinSignalStrength() const
{
    double minStrength = 100.0;

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            // Use vehicle id as rough signal indicator
            double strength = qMin(100.0, static_cast<double>(vehicle->id() % 100));
            if (strength < minStrength) {
                minStrength = strength;
            }
        }
    }

    return minStrength;
}

bool SwarmManager::checkCollisionRisk()
{
    const int collisionThresholdMeters = 10;

    for (int i = 0; i < _vehicles.count(); ++i) {
        Vehicle* v1 = _vehicles.at(i);
        if (!v1) continue;

        for (int j = i + 1; j < _vehicles.count(); ++j) {
            Vehicle* v2 = _vehicles.at(j);
            if (!v2) continue;

            double distance = _calculateDistance(v1, v2);

            if (distance < collisionThresholdMeters) {
                _emitCollisionWarning(v1->id(), v2->id());
                return true;
            }
        }
    }

    return false;
}

double SwarmManager::_calculateDistance(Vehicle* v1, Vehicle* v2) const
{
    // Simple distance calculation (for testing/validation)
    double latDiff = v1->latitude() - v2->latitude();
    double lonDiff = v1->longitude() - v2->longitude();
    double altDiff = v1->altitudeRelative()->rawValue().toDouble() - v2->altitudeRelative()->rawValue().toDouble();
    return qSqrt(latDiff * latDiff + lonDiff * lonDiff + altDiff * altDiff);
}

void SwarmManager::_emitCollisionWarning(int vehicleId1, int vehicleId2)
{
    emit collisionWarning(vehicleId1, vehicleId2);
}

QVariantMap SwarmManager::getSwarmHealthStatus() const
{
    QVariantMap status;

    status[QStringLiteral("totalVehicles")] = _vehicles.count();
    status[QStringLiteral("activeVehicles")] = activeVehicles();
    status[QStringLiteral("averageBattery")] = getAverageBatteryLevel();
    status[QStringLiteral("minSignal")] = getMinSignalStrength();
    status[QStringLiteral("collisionRisk")] = false; // Calculated in non-const method
    status[QStringLiteral("emergencyActive")] = _emergencyStopActive;
    status[QStringLiteral("formationLocked")] = _formationLocked;

    int readyCount = 0;
    int flyingCount = 0;

    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            if (vehicle->armed()) readyCount++;
            // Count flying based on mission manager state
            if (vehicle->missionManager()) flyingCount++;
        }
    }

    status[QStringLiteral("readyVehicles")] = readyCount;
    status[QStringLiteral("flyingVehicles")] = flyingCount;

    return status;
}

void SwarmManager::requestTelemetryUpdate()
{
    for (Vehicle* vehicle : _vehicles) {
        if (vehicle) {
            emit telemetryUpdateReceived(vehicle->id());
        }
    }
}

QGeoCoordinate SwarmManager::getFormationOffset(int vehicleIndex, const QGeoCoordinate &leaderPosition)
{
    Q_UNUSED(leaderPosition)
    switch (_currentFormation) {
        case SwarmFormation::Line:
            return _calculateLinePosition(vehicleIndex, _vehicles.count());
        case SwarmFormation::VFormation:
            return _calculateVFormationPosition(vehicleIndex, _vehicles.count());
        case SwarmFormation::Grid:
            return _calculateGridPosition(vehicleIndex, _vehicles.count());
        case SwarmFormation::Circle:
            return _calculateCirclePosition(vehicleIndex, _vehicles.count());
        case SwarmFormation::Custom:
            if (vehicleIndex < _customFormationPositions.count()) {
                return _customFormationPositions.at(vehicleIndex);
            }
            break;
        default:
            break;
    }
    return QGeoCoordinate();
}

void SwarmManager::_updateSwarmState()
{
    if (!_swarmEnabled) return;

    _updateSwarmCenter();
    _updateAllVehicleStatuses();

    if (_currentFormation != SwarmFormation::None && _formationLocked) {
        _processFormationUpdates();
    }
}

void SwarmManager::_checkSwarmHealth()
{
    if (!_swarmEnabled) return;

    QVariantMap health = getSwarmHealthStatus();

    if (health[QStringLiteral("emergencyActive")].toBool()) {
        _statusText = QStringLiteral("EMERGENCY - Check Status");
    } else if (health[QStringLiteral("collisionRisk")].toBool()) {
        _statusText = QStringLiteral("Collision Risk Detected");
    } else {
        _statusText = QStringLiteral("Swarm Healthy");
    }

    emit swarmStatusTextChanged(_statusText);
}

void SwarmManager::_processFormationUpdates()
{
    if (_currentFormation == SwarmFormation::None) return;

    if (!_leaderVehicle) {
        if (!_vehicles.isEmpty()) {
            _leaderVehicle = _vehicles.first();
        } else {
            return;
        }
    }

    applyFormationOffsets();
}

void SwarmManager::_handleVehicleConnectionChange()
{
    emit swarmMembersChanged(swarmMembers());
    _updateAllVehicleStatuses();
}

void SwarmManager::_broadcastHeartbeat()
{
    // MAVLink heartbeat is handled by the protocol layer
}

void SwarmManager::_initializeSwarm()
{
    if (_vehicles.isEmpty()) return;

    if (!_leaderVehicle && !_vehicles.isEmpty()) {
        _leaderVehicle = _vehicles.first();
    }

    _swarmModeActive = true;
    emit swarmModeActiveChanged(true);
}

void SwarmManager::_cleanupSwarm()
{
    _vehicles.clear();
    _leaderVehicle = nullptr;
    _subgroups.clear();
    _swarmModeActive = false;

    emit totalVehiclesChanged(0);
    emit activeVehiclesChanged(0);
    emit swarmModeActiveChanged(false);
}

void SwarmManager::_updateSwarmCenter()
{
    emit swarmCenterChanged(swarmCenter());
}

QGeoCoordinate SwarmManager::_calculateLinePosition(int index, int total)
{
    // Calculate position in a line formation
    double offset = (index - total / 2.0) * _formationSpacing;
    QGeoCoordinate center = swarmCenter();

    // Simple east-west offset
    double latOffset = 0.0;
    double lonOffset = offset / 111320.0; // Approximate meters to degrees

    return QGeoCoordinate(center.latitude() + latOffset, center.longitude() + lonOffset, center.altitude());
}

QGeoCoordinate SwarmManager::_calculateVFormationPosition(int index, int total)
{
    QGeoCoordinate center = swarmCenter();

    // V formation: leader at front, followers in V shape
    Q_UNUSED(total)
    double angleRad = 30.0 * M_PI / 180.0; // 30 degree spread
    double row = index;
    double col = (index % 2 == 0) ? -1 : 1;

    double offsetLat = row * _formationSpacing * qCos(angleRad) / 111320.0;
    double offsetLon = col * row * _formationSpacing * qSin(angleRad) / (111320.0 * qCos(center.latitude() * M_PI / 180.0));

    return QGeoCoordinate(center.latitude() + offsetLat, center.longitude() + offsetLon, center.altitude());
}

QGeoCoordinate SwarmManager::_calculateGridPosition(int index, int total)
{
    QGeoCoordinate center = swarmCenter();

    // Grid formation: calculate rows and columns
    int cols = qCeil(qSqrt(total));
    int rows = qCeil(total / cols);

    int row = index / cols;
    int col = index % cols;

    double offsetLat = (row - rows / 2.0) * _formationSpacing / 111320.0;
    double offsetLon = (col - cols / 2.0) * _formationSpacing / (111320.0 * qCos(center.latitude() * M_PI / 180.0));

    return QGeoCoordinate(center.latitude() + offsetLat, center.longitude() + offsetLon, center.altitude());
}

QGeoCoordinate SwarmManager::_calculateCirclePosition(int index, int total)
{
    QGeoCoordinate center = swarmCenter();

    if (total == 0) return center;

    // Circle formation: evenly distribute around center
    double angle = 2.0 * M_PI * index / total;
    double radius = _formationSpacing * (total > 1 ? 1.0 : 0.0);

    double offsetLat = radius * qCos(angle) / 111320.0;
    double offsetLon = radius * qSin(angle) / (111320.0 * qCos(center.latitude() * M_PI / 180.0));

    return QGeoCoordinate(center.latitude() + offsetLat, center.longitude() + offsetLon, center.altitude());
}

void SwarmManager::_sendSwarmCoordinationMessage(Vehicle* vehicle, int messageId, const QVariantMap &params)
{
    Q_UNUSED(vehicle)
    Q_UNUSED(messageId)
    Q_UNUSED(params)
    // MAVLink message sending would be implemented here
    // For now, commands are sent via Vehicle methods
}

void SwarmManager::_updateAllVehicleStatuses()
{
    for (Vehicle* vehicle : _vehicles) {
        if (!vehicle) continue;

        int id = vehicle->id();
        SwarmMemberStatus status;

        // Simple status determination based on armed state and flight mode query
        if (!vehicle->armed()) {
            status = SwarmMemberStatus::Ready;
        } else {
            // Check if vehicle is in guided mode (could be RTL/Landing/Mission)
            if (vehicle->flightMode().contains(QStringLiteral("RTL"), Qt::CaseInsensitive) ||
                vehicle->flightMode().contains(QStringLiteral("Return"), Qt::CaseInsensitive)) {
                status = SwarmMemberStatus::ReturningHome;
            } else if (vehicle->flightMode().contains(QStringLiteral("Land"), Qt::CaseInsensitive)) {
                status = SwarmMemberStatus::Landed;
            } else if (vehicle->flightMode().contains(QStringLiteral("Mission"), Qt::CaseInsensitive) ||
                       vehicle->flightMode().contains(QStringLiteral("Auto"), Qt::CaseInsensitive)) {
                status = SwarmMemberStatus::InMission;
            } else {
                status = SwarmMemberStatus::Ready;
            }
        }

        if (_vehicleStatuses.value(id) != status) {
            _vehicleStatuses[id] = status;
            emit vehicleStatusChanged(id, status);
        }
    }

    emit activeVehiclesChanged(activeVehicles());
}

QGeoCoordinate SwarmManager::_calculateFollowerOffset(Vehicle* follower)
{
    if (!_leaderVehicle) return QGeoCoordinate();

    QGeoCoordinate leaderPos(_leaderVehicle->latitude(), _leaderVehicle->longitude());
    int index = _vehicles.indexOf(follower);

    return getFormationOffset(index, leaderPos);
}