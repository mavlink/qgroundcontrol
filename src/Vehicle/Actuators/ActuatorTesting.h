/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>

#include <QmlObjectListModel.h>

#include <QTimer>
#include "Vehicle.h"
#include "MAVLinkProtocol.h"

namespace ActuatorTesting {

class Actuator : public QObject
{
    Q_OBJECT
public:
    Actuator(QObject* parent, const QString& label, float min, float max, float defaultValue, int function, bool isMotor)
        : QObject(parent), _label(label), _min(min), _max(max), _defaultValue(defaultValue), _function(function),
          _isMotor(isMotor) {}

    Q_PROPERTY(QString label              READ label                             CONSTANT)
    Q_PROPERTY(float min                  READ min                               CONSTANT)
    Q_PROPERTY(float max                  READ max                               CONSTANT)
    Q_PROPERTY(float defaultValue         READ defaultValue                      CONSTANT)
    Q_PROPERTY(int function               READ function                          CONSTANT)
    Q_PROPERTY(bool isMotor               READ isMotor                           CONSTANT)

    const QString& label() const { return _label; }
    float min() const { return _min; }
    float max() const { return _max; }
    float defaultValue() const { return _defaultValue; }
    int function() const { return _function; }
    bool isMotor() const { return _isMotor; }

private:
    const QString _label;
    const float _min;
    const float _max;
    const float _defaultValue;
    const int _function;
    const bool _isMotor;
};

class ActuatorTest : public QObject
{
    Q_OBJECT
public:
    ActuatorTest(Vehicle* vehicle);

    ~ActuatorTest();

    Q_PROPERTY(QmlObjectListModel* actuators         READ actuators                NOTIFY actuatorsChanged)
    Q_PROPERTY(Actuator* allMotorsActuator           READ allMotorsActuator        NOTIFY actuatorsChanged)
    Q_PROPERTY(bool hadFailure                       READ hadFailure               NOTIFY hadFailureChanged)

    QmlObjectListModel* actuators() { return _actuators; }
    Actuator* allMotorsActuator() { return _allMotorsActuator; }

    void updateFunctions(const QList<Actuator*>& actuators);

    /**
     * (de-)activate actuator testing. No channels can be changed while deactivated
     */
    Q_INVOKABLE void setActive(bool active);

    /**
     * Control an actuator.
     * This needs to be called at least every 100ms for each controlled channel, to refresh that channel,
     * otherwise it will be stopped.
     * @param index actuator index
     * @param value control value in range defined by the actuator
     */
    Q_INVOKABLE void setChannelTo(int index, float value);

    /**
     * Stop controlling an actuator
     * @param index actuator index (-1 for all)
     */
    Q_INVOKABLE void stopControl(int index);

    bool hadFailure() const { return _hadFailure; }

signals:
    void actuatorsChanged();
    void hadFailureChanged();

private:

    struct ActuatorState {
        enum class State {
            NotActive,
            Active,
            StopRequest,
            Stopping
        };
        State state{State::NotActive};
        float value{0.f};
        QElapsedTimer lastUpdated;
    };

    void resetStates();

    static void ackHandlerEntry(void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress,
            Vehicle::MavCmdResultFailureCode_t failureCode);
    void ackHandler(MAV_RESULT commandResult, Vehicle::MavCmdResultFailureCode_t failureCode);
    void sendMavlinkRequest(int function, float value, float timeout);

    void sendNext();
    void watchdogTimeout();

    QmlObjectListModel* _actuators{new QmlObjectListModel(this)}; ///< list of Actuator*
    Vehicle* _vehicle{nullptr};
    Actuator* _allMotorsActuator{nullptr};
    bool _active{false};
    bool _commandInProgress{false};

    QList<ActuatorState> _states;
    int _currentState{-1};
    QTimer _watchdogTimer;
    bool _hadFailure{false};
};

} // namespace ActuatorTesting
