#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QList>
#include <QtCore/QVariantMap>
#include <QtCore/QMetaType>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtPositioning/QGeoCoordinate>

// Forward declaration - Vehicle.h is included in the .cpp file
class Vehicle;

/// Formation types for swarm coordination
enum class SwarmFormation {
    None,
    Line,
    VFormation,
    Grid,
    Circle,
    Custom
};

/// Swarm member status
enum class SwarmMemberStatus {
    Disconnected,
    Connecting,
    Ready,
    InMission,
    ReturningHome,
    Emergency,
    Landed
};

/// Swarm coordination mode
enum class SwarmCoordinationMode {
    Independent,
    LeaderFollower,
    Broadcast,
    Formation
};

/// @brief Core swarm management class for multi-UAV coordination
///
/// This class provides centralized swarm management, coordination, and control
/// capabilities for operating multiple UAVs simultaneously.
class SwarmManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool swarmEnabled READ swarmEnabled WRITE setSwarmEnabled NOTIFY swarmEnabledChanged)
    Q_PROPERTY(bool swarmModeActive READ swarmModeActive NOTIFY swarmModeActiveChanged)
    Q_PROPERTY(int totalVehicles READ totalVehicles NOTIFY totalVehiclesChanged)
    Q_PROPERTY(int activeVehicles READ activeVehicles NOTIFY activeVehiclesChanged)
    Q_PROPERTY(Vehicle* leaderVehicle READ leaderVehicle WRITE setLeaderVehicle NOTIFY leaderVehicleChanged)
    Q_PROPERTY(SwarmFormation currentFormation READ currentFormation WRITE setCurrentFormation NOTIFY currentFormationChanged)
    Q_PROPERTY(SwarmCoordinationMode coordinationMode READ coordinationMode WRITE setCoordinationMode NOTIFY coordinationModeChanged)
    Q_PROPERTY(QVariantList swarmMembers READ swarmMembers NOTIFY swarmMembersChanged)
    Q_PROPERTY(double formationSpacing READ formationSpacing WRITE setFormationSpacing NOTIFY formationSpacingChanged)
    Q_PROPERTY(QString swarmStatusText READ swarmStatusText NOTIFY swarmStatusTextChanged)
    Q_PROPERTY(bool allVehiclesReady READ allVehiclesReady NOTIFY allVehiclesReadyChanged)
    Q_PROPERTY(bool emergencyStopActive READ emergencyStopActive NOTIFY emergencyStopActiveChanged)
    Q_PROPERTY(QGeoCoordinate swarmCenter READ swarmCenter NOTIFY swarmCenterChanged)

public:
    explicit SwarmManager(QObject *parent = nullptr);
    ~SwarmManager();

    static SwarmManager *instance();

    // Swarm state accessors
    bool swarmEnabled() const { return _swarmEnabled; }
    bool swarmModeActive() const { return _swarmModeActive && _swarmEnabled; }
    int totalVehicles() const { return _vehicles.count(); }
    int activeVehicles() const;
    Vehicle* leaderVehicle() const { return _leaderVehicle; }
    SwarmFormation currentFormation() const { return _currentFormation; }
    SwarmCoordinationMode coordinationMode() const { return _coordinationMode; }
    QVariantList swarmMembers() const;
    double formationSpacing() const { return _formationSpacing; }
    QString swarmStatusText() const { return _statusText; }
    bool allVehiclesReady() const;
    bool emergencyStopActive() const { return _emergencyStopActive; }
    QGeoCoordinate swarmCenter() const;

    // Swarm configuration
    Q_INVOKABLE void setSwarmEnabled(bool enabled);
    Q_INVOKABLE void setLeaderVehicle(Vehicle *vehicle);
    Q_INVOKABLE void setCurrentFormation(SwarmFormation formation);
    Q_INVOKABLE void setCoordinationMode(SwarmCoordinationMode mode);
    Q_INVOKABLE void setFormationSpacing(double spacing);

    // Vehicle management
    Q_INVOKABLE void addVehicle(Vehicle *vehicle);
    Q_INVOKABLE void removeVehicle(Vehicle *vehicle);
    Q_INVOKABLE void selectVehicle(int vehicleId);
    Q_INVOKABLE void deselectVehicle(int vehicleId);
    Q_INVOKABLE void deselectAllVehicles();
    Q_INVOKABLE Vehicle* getVehicleById(int vehicleId) const;
    Q_INVOKABLE QVariantList getSelectedVehicles() const;
    Q_INVOKABLE int vehicleCount() const { return _vehicles.count(); }

    // Swarm coordination commands
    Q_INVOKABLE void synchronizedTakeoff(double altitude);
    Q_INVOKABLE void synchronizedLand();
    Q_INVOKABLE void synchronizedRTL();
    Q_INVOKABLE void emergencyStopAll();
    Q_INVOKABLE void resumeFromEmergency();
    Q_INVOKABLE void broadcastCommand(int mavlinkCommand, const QVariantMap &params = QVariantMap());
    Q_INVOKABLE void executeFormationFlight();
    Q_INVOKABLE void executeLeaderFollower(double separation);
    Q_INVOKABLE void holdPosition();
    Q_INVOKABLE void returnAllToHome();
    Q_INVOKABLE void pauseAllMissions();
    Q_INVOKABLE void resumeAllMissions();
    Q_INVOKABLE void syncWaypoints();
    Q_INVOKABLE void distributeWaypoints(const QVariantList &waypoints);

    // Formation management
    Q_INVOKABLE void setCustomFormation(const QVariantList &positions);
    Q_INVOKABLE QVariantList calculateFormationPositions(int vehicleCount, SwarmFormation formation);
    Q_INVOKABLE void applyFormationOffsets();
    Q_INVOKABLE void lockFormation();
    Q_INVOKABLE void unlockFormation();

    // Subgroup control
    Q_INVOKABLE void createSubgroup(const QList<int> &vehicleIds, const QString &name);
    Q_INVOKABLE void controlSubgroup(const QString &subgroupName, const QString &command);
    Q_INVOKABLE QVariantList getSubgroupVehicles(const QString &subgroupName) const;
    Q_INVOKABLE void removeSubgroup(const QString &subgroupName);

    // Health and monitoring
    Q_INVOKABLE double getAverageBatteryLevel() const;
    Q_INVOKABLE double getMinSignalStrength() const;
    Q_INVOKABLE bool checkCollisionRisk();
    Q_INVOKABLE QVariantMap getSwarmHealthStatus() const;
    Q_INVOKABLE void requestTelemetryUpdate();

    // Waypoint synchronization
    Q_INVOKABLE void syncMissionWaypoints();
    Q_INVOKABLE void updateWaypointsForFormation(const QVariantList &baseWaypoints);

    // Formation position calculation helper
    Q_INVOKABLE QGeoCoordinate getFormationOffset(int vehicleIndex, const QGeoCoordinate &leaderPosition);

private:
    double _calculateDistance(Vehicle* v1, Vehicle* v2) const;
    void _emitCollisionWarning(int vehicleId1, int vehicleId2);

signals:
    void swarmEnabledChanged(bool enabled);
    void swarmModeActiveChanged(bool active);
    void totalVehiclesChanged(int count);
    void activeVehiclesChanged(int count);
    void leaderVehicleChanged(Vehicle *vehicle);
    void currentFormationChanged(SwarmFormation formation);
    void coordinationModeChanged(SwarmCoordinationMode mode);
    void swarmMembersChanged(const QVariantList &members);
    void formationSpacingChanged(double spacing);
    void swarmStatusTextChanged(const QString &status);
    void allVehiclesReadyChanged(bool ready);
    void emergencyStopActiveChanged(bool active);
    void swarmCenterChanged(const QGeoCoordinate &center);
    void vehicleStatusChanged(int vehicleId, SwarmMemberStatus status);
    void formationUpdateRequired();
    void collisionWarning(int vehicleId1, int vehicleId2);
    void subgroupCreated(const QString &name, const QList<int> &vehicleIds);
    void subgroupCommandSent(const QString &subgroupName, const QString &command);
    void synchronizedCommandCompleted(const QString &command, bool success);
    void telemetryUpdateReceived(int vehicleId);

private slots:
    void _updateSwarmState();
    void _checkSwarmHealth();
    void _processFormationUpdates();
    void _handleVehicleConnectionChange();
    void _broadcastHeartbeat();

private:
    void _initializeSwarm();
    void _cleanupSwarm();
    void _updateSwarmCenter();
    QGeoCoordinate _calculateLinePosition(int index, int total);
    QGeoCoordinate _calculateVFormationPosition(int index, int total);
    QGeoCoordinate _calculateGridPosition(int index, int total);
    QGeoCoordinate _calculateCirclePosition(int index, int total);
    void _sendSwarmCoordinationMessage(Vehicle *vehicle, int messageId, const QVariantMap &params);
    void _updateAllVehicleStatuses();

    static SwarmManager *_instance;

    bool _swarmEnabled = false;
    bool _swarmModeActive = false;
    bool _emergencyStopActive = false;
    bool _formationLocked = false;

    QTimer *_swarmUpdateTimer = nullptr;
    QTimer *_healthCheckTimer = nullptr;
    QTimer *_heartbeatTimer = nullptr;

    QList<Vehicle*> _vehicles;
    Vehicle *_leaderVehicle = nullptr;

    SwarmFormation _currentFormation = SwarmFormation::None;
    SwarmCoordinationMode _coordinationMode = SwarmCoordinationMode::Independent;
    double _formationSpacing = 10.0; // meters

    QMap<QString, QList<int>> _subgroups;
    QList<QGeoCoordinate> _customFormationPositions;

    QString _statusText;
    int _swarmUpdateInterval = 100; // ms

    QMap<int, SwarmMemberStatus> _vehicleStatuses;
    QMap<int, QGeoCoordinate> _vehicleLastPositions;
};

Q_DECLARE_METATYPE(SwarmFormation)
Q_DECLARE_METATYPE(SwarmMemberStatus)
Q_DECLARE_METATYPE(SwarmCoordinationMode)