#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QList>

class Fact;
class QmlObjectListModel;
class Vehicle;

/// State machine for motor assignment workflow.
///
/// States:
/// - Idle: Waiting for initialization
/// - Initialized: Config validated, waiting for user confirmation
/// - AssigningMotors: Clearing and reassigning motor functions (optional)
/// - SpinningMotor: Sending MAVLink command to spin current motor
/// - WaitingForSelection: Waiting for user to identify spinning motor
/// - ApplyingAssignment: Applying final motor ordering
/// - Complete: Assignment finished successfully
/// - Error: Assignment failed
class MotorAssignmentStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit MotorAssignmentStateMachine(Vehicle* vehicle, QmlObjectListModel* actuators, QObject* parent = nullptr);
    ~MotorAssignmentStateMachine() override = default;

    /// Initialize assignment workflow
    /// @return true if initialization succeeded, false if configuration is invalid
    bool initialize(int selectedActuatorIdx, int firstMotorsFunction, int numMotors);

    /// Start the assignment after user confirmation
    void startAssignment();

    /// User selected which motor was spinning
    void selectMotor(int motorIndex);

    /// Abort the assignment
    void abortAssignment();

    // Accessors
    bool isActive() const;
    QString message() const { return _message; }
    int currentMotorIndex() const { return _selectedMotors.size(); }
    int totalMotors() const { return _numMotors; }
    bool needsMotorReassignment() const { return _assignMotors; }

signals:
    void activeChanged();
    void messageChanged();
    void motorSpinStarted(int motorIndex);
    void assignmentComplete();
    void assignmentAborted();
    void assignmentError(const QString& error);

private slots:
    void _onStateChanged();

private:
    void _buildStateMachine();
    void _gatherFunctionFacts();
    bool _validateConfiguration();
    void _clearAndAssignMotors();
    void _applyFinalAssignment();
    void _spinMotor(int motorIndex);
    void _handleSpinAck(MAV_RESULT result, Vehicle::MavCmdResultFailureCode_t failureCode);

    static void _ackHandlerEntry(void* resultHandlerData, int compId,
                                  const mavlink_command_ack_t& ack,
                                  Vehicle::MavCmdResultFailureCode_t failureCode);

    // Configuration
    Vehicle* _vehicle = nullptr;
    QmlObjectListModel* _actuators = nullptr;

    // Assignment parameters
    int _selectedActuatorIdx = -1;
    int _firstMotorsFunction = 0;
    int _numMotors = 0;
    bool _assignMotors = false;

    // Runtime state
    QList<int> _selectedMotors;
    QList<QList<Fact*>> _functionFacts;
    QString _message;
    bool _commandInProgress = false;

    // Timing constants
    static constexpr int SpinTimeoutDefaultMs = 1000;
    static constexpr int SpinTimeoutAfterAssignMs = 3000;

    // States (owned by state machine)
    QGCState* _idleState = nullptr;
    QGCState* _initializedState = nullptr;
    QGCState* _assigningMotorsState = nullptr;
    QGCState* _spinningMotorState = nullptr;
    QGCState* _waitingForSelectionState = nullptr;
    QGCState* _applyingAssignmentState = nullptr;
    QGCFinalState* _completeState = nullptr;
    QGCState* _errorState = nullptr;
};
