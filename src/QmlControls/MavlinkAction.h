/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MavlinkActionLog)

class Vehicle;

class MavlinkAction: public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)

public:
    explicit MavlinkAction(QObject *parent = nullptr);
    MavlinkAction(
        const QString &label,
        const QString &description,
        MAV_CMD mavCmd,
        MAV_COMPONENT compId,
        float param1,
        float param2,
        float param3,
        float param4,
        float param5,
        float param6,
        float param7,
        QObject *parent = nullptr
    );
    ~MavlinkAction();

    Q_INVOKABLE void sendTo(Vehicle *vehicle);

    const QString &label() const { return _label; }
    const QString &description() const { return _description; }

private:
    const QString _label;
    const QString _description;
    const MAV_CMD _mavCmd = MAV_CMD_ENUM_END;
    const MAV_COMPONENT _compId = MAV_COMPONENT_ENUM_END;
    const float _params[7]{};
};
