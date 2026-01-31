#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QTimer>

class QmlObjectListModel;
class Vehicle;

namespace ActuatorTesting {
class Actuator;
}

/// State machine for actuator testing control loop.
///
/// Manages per-actuator state tracking and round-robin command sending.
/// Each actuator transitions through: NotActive -> Active -> StopRequest -> Stopping -> NotActive
///
/// The machine runs a continuous polling loop when active, cycling through
/// actuators and sending MAVLink commands as needed.
class ActuatorTestingStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    /// Per-actuator state
    enum class ActuatorState {
        NotActive,    ///< Not being controlled
        Active,       ///< Actively controlled with a value
        StopRequest,  ///< Request to stop has been made
        Stopping      ///< Stop command sent, waiting for ack
    };
    Q_ENUM(ActuatorState)

    explicit ActuatorTestingStateMachine(Vehicle* vehicle, QmlObjectListModel* actuators, QObject* parent = nullptr);
    ~ActuatorTestingStateMachine() override = default;

    /// Activate or deactivate actuator testing
    void setActive(bool active);
    bool isTestingActive() const { return _testingActive; }

    /// Set an actuator's control value
    /// @param index Actuator index
    /// @param value Control value in actuator's range
    void setChannelTo(int index, float value);

    /// Stop controlling an actuator
    /// @param index Actuator index (-1 for all)
    void stopControl(int index);

    /// Reset all actuator states
    void resetStates();

    /// Get state of a specific actuator
    ActuatorState actuatorState(int index) const;

    /// Check if any actuator is active
    bool hasActiveActuators() const;

    /// Check if a command failed
    bool hadFailure() const { return _hadFailure; }

signals:
    void actuatorStateChanged(int index, ActuatorState state);
    void commandFailed(const QString& message);
    void hadFailureChanged();

private slots:
    void _watchdogTimeout();

private:
    struct ActuatorStateData {
        ActuatorState state = ActuatorState::NotActive;
        float value = 0.0f;
        QElapsedTimer lastUpdated;
    };

    void _buildStateMachine();
    void _sendNext();
    void _sendMavlinkRequest(int function, float value, float timeout);
    void _handleAck(MAV_RESULT result, Vehicle::MavCmdResultFailureCode_t failureCode);

    static void _ackHandlerEntry(void* resultHandlerData, int compId,
                                  const mavlink_command_ack_t& ack,
                                  Vehicle::MavCmdResultFailureCode_t failureCode);

    Vehicle* _vehicle = nullptr;
    QmlObjectListModel* _actuators = nullptr;

    QList<ActuatorStateData> _states;
    int _currentIndex = -1;
    bool _commandInProgress = false;
    bool _testingActive = false;
    bool _hadFailure = false;

    QTimer _watchdogTimer;

    // States
    QGCState* _idleState = nullptr;
    QGCState* _pollingState = nullptr;
    QGCState* _sendingState = nullptr;
    QGCState* _waitingAckState = nullptr;

    static constexpr int WatchdogIntervalMs = 100;
    static constexpr int ActuatorTimeoutMs = 100;
};
