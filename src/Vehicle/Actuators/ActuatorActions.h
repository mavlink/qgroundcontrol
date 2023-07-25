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

#include "Common.h"

#include <QmlObjectListModel.h>

namespace ActuatorActions {

struct Config {
    enum class Type {
        beep = ACTUATOR_CONFIGURATION_BEEP,                            ///< beep actuator
        set3DModeOn = ACTUATOR_CONFIGURATION_3D_MODE_ON,               ///< motors: enable 3D mode (reversible)
        set3DModeOff = ACTUATOR_CONFIGURATION_3D_MODE_OFF,             ///< motors: disable 3D mode (reversible)
        setSpinDirection1 = ACTUATOR_CONFIGURATION_SPIN_DIRECTION1,    ///< motors: set spin direction 1
        setSpinDirection2 = ACTUATOR_CONFIGURATION_SPIN_DIRECTION2,    ///< motors: set spin direction 2
    };

    QString typeToLabel() const;

    Type type;
    Condition condition;
    QSet<QString> actuatorTypes;
};

class Action : public QObject
{
    Q_OBJECT
public:
    Action(QObject* parent, const Config& action, const QString& label,
            int outputFunction, Vehicle* vehicle);

    Q_PROPERTY(QString label                     READ label              CONSTANT)

    const QString& label() const { return _label; }

    Q_INVOKABLE void trigger();

private:
    static void ackHandlerEntry(void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress,
            Vehicle::MavCmdResultFailureCode_t failureCode);
    void ackHandler(MAV_RESULT commandResult, Vehicle::MavCmdResultFailureCode_t failureCode);
    void sendMavlinkRequest();

    const QString _label;
    const int _outputFunction;
    const Config::Type _type;
    Vehicle* _vehicle{nullptr};

    bool _commandInProgress{false};
};

class ActionGroup : public QObject
{
    Q_OBJECT
public:
    ActionGroup(QObject* parent, const QString& label, Config::Type type);

    Q_PROPERTY(QString label                     READ label              CONSTANT)
    Q_PROPERTY(QmlObjectListModel* actions       READ actions            CONSTANT)

    QmlObjectListModel* actions() { return _actions; }
    const QString& label() const { return _label; }

    void addAction(Action* action) { _actions->append(action); }

    Config::Type type() const { return _type; }

private:
    const QString _label;
    QmlObjectListModel* _actions = new QmlObjectListModel(this); ///< list of Action*
    const Config::Type _type;
};

} // namespace ActuatorActions
