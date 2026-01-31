#pragma once

#include "Vehicle.h"

#include <QtCore/QObject>
#include <QtCore/QString>

class QmlObjectListModel;
class MotorAssignmentStateMachine;

/// Handles automatic motor ordering assignment by spinning individual motors,
/// and then having the user select the corresponding motor.
///
/// This class provides the QML-facing API while delegating to
/// MotorAssignmentStateMachine for state management.
class MotorAssignment : public QObject
{
    Q_OBJECT
public:
    MotorAssignment(QObject* parent, Vehicle* vehicle, QmlObjectListModel* actuators);
    ~MotorAssignment() override;

    /// Initialize assignment, and set the UI confirmation message to be shown to the user.
    /// @return true on success, false on failure (message will contain the error message)
    bool initAssignment(int selectedActuatorIdx, int firstMotorsFunction, int numMotors);

    /// Start the assignment, called after confirming.
    void start();

    void selectMotor(int motorIndex);

    void spinCurrentMotor();

    void abort();

    bool active() const;

    const QString& message() const { return _message; }

signals:
    void activeChanged();
    void messageChanged();
    void onAbort();

private slots:
    void _onStateMachineActiveChanged();
    void _onStateMachineMessageChanged();
    void _onAssignmentComplete();
    void _onAssignmentAborted();
    void _onAssignmentError(const QString& error);

private:
    MotorAssignmentStateMachine* _stateMachine = nullptr;
    QString _message;
};
