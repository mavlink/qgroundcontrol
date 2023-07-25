/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief  Custom Joystick MAV command

#pragma once

#include <QString>
#include <QList>

class Vehicle;

/// Custom MAV command
class JoystickMavCommand
{
public:
    static QList<JoystickMavCommand> load(const QString& jsonFilename);
    QString name() const { return _name; }

    void send(Vehicle* vehicle);
private:
    QString _name;
    int _id = 0;
    bool _showError = false;
    float _param1 = 0.0f;
    float _param2 = 0.0f;
    float _param3 = 0.0f;
    float _param4 = 0.0f;
    float _param5 = 0.0f;
    float _param6 = 0.0f;
    float _param7 = 0.0f;
};

