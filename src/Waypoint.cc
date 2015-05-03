/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Waypoint class
 *
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include <QStringList>
#include <QDebug>

#include "Waypoint.h"

Waypoint::Waypoint(
    QObject *parent,
    quint16 id,
    double  x,
    double  y,
    double  z,
    double  param1,
    double  param2,
    double  param3,
    double  param4,
    bool    autocontinue,
    bool    current,
    int     frame,
    int     action,
    const QString& description
    )
    : QObject(parent)
    , _id(id)
    , _x(x)
    , _y(y)
    , _z(z)
    , _yaw(param4)
    , _frame(frame)
    , _action(action)
    , _autocontinue(autocontinue)
    , _current(current)
    , _orbit(param3)
    , _param1(param1)
    , _param2(param2)
    , _name(QString("WP%1").arg(id, 2, 10, QChar('0')))
    , _description(description)
    , _reachedTime(0)
{

}

Waypoint::Waypoint(const Waypoint& other)
    : QObject(NULL)
{
    *this = other;
}

Waypoint::~Waypoint()
{    
}

const Waypoint& Waypoint::operator=(const Waypoint& other)
{
    _id              = other.getId();
    _x               = other.getX();
    _y               = other.getY();
    _z               = other.getZ();
    _yaw             = other.getYaw();
    _frame           = other.getFrame();
    _action          = other.getAction();
    _autocontinue    = other.getAutoContinue();
    _current         = other.getCurrent();
    _orbit           = other.getParam3();
    _param1          = other.getParam1();
    _param2          = other.getParam2();
    _name            = other.getName();
    _description     = other.getDescription();
    _reachedTime     = other.getReachedTime();
    return *this;
}

bool Waypoint::isNavigationType()
{
    return (_action < MAV_CMD_NAV_LAST);
}

void Waypoint::save(QTextStream &saveStream)
{
    QString position("%1\t%2\t%3");
    position = position.arg(_x, 0, 'g', 18);
    position = position.arg(_y, 0, 'g', 18);
    position = position.arg(_z, 0, 'g', 18);
    QString parameters("%1\t%2\t%3\t%4");
    parameters = parameters.arg(_param1, 0, 'g', 18).arg(_param2, 0, 'g', 18).arg(_orbit, 0, 'g', 18).arg(_yaw, 0, 'g', 18);
    // FORMAT: <INDEX> <CURRENT WP> <COORD FRAME> <COMMAND> <PARAM1> <PARAM2> <PARAM3> <PARAM4> <PARAM5/X/LONGITUDE> <PARAM6/Y/LATITUDE> <PARAM7/Z/ALTITUDE> <AUTOCONTINUE> <DESCRIPTION>
    // as documented here: http://qgroundcontrol.org/waypoint_protocol
    saveStream << this->getId() << "\t" << this->getCurrent() << "\t" << this->getFrame() << "\t" << this->getAction() << "\t"  << parameters << "\t" << position  << "\t" << this->getAutoContinue() << "\r\n"; //"\t" << this->getDescription() << "\r\n";
}

bool Waypoint::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 12) {
        _id = wpParams[0].toInt();
        _current = (wpParams[1].toInt() == 1 ? true : false);
        _frame = (MAV_FRAME) wpParams[2].toInt();
        _action = (MAV_CMD) wpParams[3].toInt();
        _param1 = wpParams[4].toDouble();
        _param2 = wpParams[5].toDouble();
        _orbit = wpParams[6].toDouble();
        _yaw = wpParams[7].toDouble();
        _x = wpParams[8].toDouble();
        _y = wpParams[9].toDouble();
        _z = wpParams[10].toDouble();
        _autocontinue = (wpParams[11].toInt() == 1 ? true : false);
        //_description = wpParams[12];
        return true;
    }
    return false;
}


void Waypoint::setId(quint16 id)
{
    _id = id;
    _name = QString("WP%1").arg(id, 2, 10, QChar('0'));
    emit changed(this);
}

void Waypoint::setX(double x)
{
    if (!isinf(x) && !isnan(x) && ((_frame == MAV_FRAME_LOCAL_NED) || (_frame == MAV_FRAME_LOCAL_ENU)))
    {
        _x = x;
        emit changed(this);
    }
}

void Waypoint::setY(double y)
{
    if (!isinf(y) && !isnan(y) && ((_frame == MAV_FRAME_LOCAL_NED) || (_frame == MAV_FRAME_LOCAL_ENU)))
    {
        _y = y;
        emit changed(this);
    }
}

void Waypoint::setZ(double z)
{
    if (!isinf(z) && !isnan(z) && ((_frame == MAV_FRAME_LOCAL_NED) || (_frame == MAV_FRAME_LOCAL_ENU)))
    {
        _z = z;
        emit changed(this);
    }
}

void Waypoint::setLatitude(double lat)
{
    if (_x != lat && ((_frame == MAV_FRAME_GLOBAL) || (_frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        _x = lat;
        emit changed(this);
    }
}

void Waypoint::setLongitude(double lon)
{
    if (_y != lon && ((_frame == MAV_FRAME_GLOBAL) || (_frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        _y = lon;
        emit changed(this);
    }
}

void Waypoint::setAltitude(double altitude)
{
    if (_z != altitude && ((_frame == MAV_FRAME_GLOBAL) || (_frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        _z = altitude;
        emit changed(this);
    }
}

void Waypoint::setYaw(int yaw)
{
    if (_yaw != yaw)
    {
        _yaw = yaw;
        emit changed(this);
    }
}

void Waypoint::setYaw(double yaw)
{
    if (_yaw != yaw)
    {
        _yaw = yaw;
        emit changed(this);
    }
}

void Waypoint::setAction(int /*MAV_CMD*/ action)
{
    if (_action != action) {
        _action = action;

        // Flick defaults according to WP type

        if (_action == MAV_CMD_NAV_TAKEOFF) {
            // We default to 15 degrees minimum takeoff pitch
            _param1 = 15.0;
        }

        emit changed(this);
    }
}

void Waypoint::setFrame(int /*MAV_FRAME*/ frame)
{
    if (_frame != frame) {
        _frame = frame;
        emit changed(this);
    }
}

void Waypoint::setAutocontinue(bool autoContinue)
{
    if (_autocontinue != autoContinue) {
        _autocontinue = autoContinue;
        emit changed(this);
    }
}

void Waypoint::setCurrent(bool current)
{
    if (_current != current)
    {
        _current = current;
        // The current waypoint index is handled by the list
        // and not part of the individual waypoint update state
    }
}

void Waypoint::setAcceptanceRadius(double radius)
{
    if (_param2 != radius)
    {
        _param2 = radius;
        emit changed(this);
    }
}

void Waypoint::setParam1(double param1)
{
    //// // qDebug() << "SENDER:" << QObject::sender();
    //// // qDebug() << "PARAM1 SET REQ:" << param1;
    if (_param1 != param1)
    {
        _param1 = param1;
        emit changed(this);
    }
}

void Waypoint::setParam2(double param2)
{
    if (_param2 != param2)
    {
        _param2 = param2;
        emit changed(this);
    }
}

void Waypoint::setParam3(double param3)
{
    if (_orbit != param3) {
        _orbit = param3;
        emit changed(this);
    }
}

void Waypoint::setParam4(double param4)
{
    if (_yaw != param4) {
        _yaw = param4;
        emit changed(this);
    }
}

void Waypoint::setParam5(double param5)
{
    if (_x != param5) {
        _x = param5;
        emit changed(this);
    }
}

void Waypoint::setParam6(double param6)
{
    if (_y != param6) {
        _y = param6;
        emit changed(this);
    }
}

void Waypoint::setParam7(double param7)
{
    if (_z != param7) {
        _z = param7;
        emit changed(this);
    }
}

void Waypoint::setLoiterOrbit(double orbit)
{
    if (_orbit != orbit) {
        _orbit = orbit;
        emit changed(this);
    }
}

void Waypoint::setHoldTime(int holdTime)
{
    if (_param1 != holdTime) {
        _param1 = holdTime;
        emit changed(this);
    }
}

void Waypoint::setHoldTime(double holdTime)
{
    if (_param1 != holdTime) {
        _param1 = holdTime;
        emit changed(this);
    }
}

void Waypoint::setTurns(int turns)
{
    if (_param1 != turns) {
        _param1 = turns;
        emit changed(this);
    }
}
