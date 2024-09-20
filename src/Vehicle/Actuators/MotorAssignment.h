/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Vehicle.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

class QmlObjectListModel;

/**
 * Handles automatic motor ordering assignment by spinning individual motors, and then having the user
 * to select the corresponding motor.
 */
class MotorAssignment : public QObject
{
    Q_OBJECT
public:
    MotorAssignment(QObject* parent, Vehicle* vehicle, QmlObjectListModel* actuators);

    virtual ~MotorAssignment() = default;

    /**
     * Initialize assignment, and set the UI confirmation message to be shown to the user.
     * @return true on success, false on failure (message will contain the error message)
     */
    bool initAssignment(int selectedActuatorIdx, int firstMotorsFunction, int numMotors);

    /**
     * Start the assignment, called after confirming.
     */
    void start();

    void selectMotor(int motorIndex);

    void spinCurrentMotor();

    void abort();

    bool active() const { return _state == State::Running; }

    const QString& message() const { return _message; }

signals:
    void activeChanged();
    void messageChanged();
    void onAbort();

private slots:
    void spinTimeout();

private:
    static constexpr int _spinTimeoutDefaultSec = 1000;
    static constexpr int _spinTimeoutHighSec = 3000; ///< wait a bit longer after assigning motors, so ESCs can initialize

    static void ackHandlerEntry(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    void ackHandler(MAV_RESULT commandResult, Vehicle::MavCmdResultFailureCode_t failureCode);
    void sendMavlinkRequest(int function, float value);

    enum class State {
        Idle,
        Init,
        Running
    };

    Vehicle* _vehicle{nullptr};
    QmlObjectListModel* _actuators{nullptr}; ///< list of ActuatorOutput*

    int _selectedActuatorIdx{-1};
    int _firstMotorsFunction{};
    int _numMotors{};
    QTimer _spinTimer;

    State _state{State::Idle};
    bool _assignMotors{false};
    QList<int> _selectedMotors{};
    bool _commandInProgress{false};
    QList<QList<Fact*>> _functionFacts;
    QString _message; ///< current message to show to the user after initializing
};

