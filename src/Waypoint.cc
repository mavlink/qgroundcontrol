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

#include "Waypoint.h"
#include <QStringList>

Waypoint::Waypoint(quint16 _id, double _x, double _y, double _z, double _yaw, bool _autocontinue, bool _current, double _orbit, int _holdTime, MAV_FRAME _frame, MAV_ACTION _action)
    : id(_id),
    x(_x),
    y(_y),
    z(_z),
    yaw(_yaw),
    frame(_frame),
    action(_action),
    autocontinue(_autocontinue),
    current(_current),
    orbit(0),
    orbitDirection(0),
    param1(_orbit),
    param2(_holdTime),
    name(QString("WP%1").arg(id, 2, 10, QChar('0')))
{
}

Waypoint::~Waypoint()
{

}

void Waypoint::save(QTextStream &saveStream)
{
    QString position("%1\t%2\t%3\t%4");
    position = position.arg(x, 0, 'g', 18);
    position = position.arg(y, 0, 'g', 18);
    position = position.arg(z, 0, 'g', 18);
    position = position.arg(yaw, 0, 'g', 8);
    saveStream << this->getId() << "\t" << this->getFrame() << "\t" << this->getAction() << "\t"  << orbit << "\t" << orbitDirection << "\t" << param1 << "\t" << param2 << "\t" << this->getCurrent() << "\t" << position  << "\t" << this->getAutoContinue() << "\r\n";
}

bool Waypoint::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 13)
    {
        this->id = wpParams[0].toInt();
        this->frame = (MAV_FRAME) wpParams[1].toInt();
        this->action = (MAV_ACTION) wpParams[2].toInt();
        this->orbit = wpParams[3].toDouble();
        this->orbitDirection = wpParams[4].toInt();
        this->param1 = wpParams[5].toDouble();
        this->param2 = wpParams[6].toDouble();
        this->current = (wpParams[7].toInt() == 1 ? true : false);
        this->x = wpParams[8].toDouble();
        this->y = wpParams[9].toDouble();
        this->z = wpParams[10].toDouble();
        this->yaw = wpParams[11].toDouble();
        this->autocontinue = (wpParams[12].toInt() == 1 ? true : false);
        return true;
    }
    return false;
}


void Waypoint::setId(quint16 id)
{
    this->id = id;
    this->name = QString("WP%1").arg(id, 2, 10, QChar('0'));
    emit changed(this);
}

void Waypoint::setX(double x)
{
    if (this->x != x)
    {
        this->x = x;
        emit changed(this);
    }
}

void Waypoint::setY(double y)
{
    if (this->y != y)
    {
        this->y = y;
        emit changed(this);
    }
}

void Waypoint::setZ(double z)
{
    if (this->z != z)
    {
        this->z = z;
        emit changed(this);
    }
}

void Waypoint::setYaw(double yaw)
{
    if (this->yaw != yaw)
    {
        this->yaw = yaw;
        emit changed(this);
    }
}

void Waypoint::setAction(int action)
{
    if (this->action != (MAV_ACTION)action)
    {
        this->action = (MAV_ACTION)action;
        emit changed(this);
    }
}

void Waypoint::setAction(MAV_ACTION action)
{
    if (this->action != action)
    {
        this->action = action;
        emit changed(this);
    }
}

void Waypoint::setFrame(MAV_FRAME frame)
{
    if (this->frame != frame)
    {
        this->frame = frame;
        emit changed(this);
    }
}

void Waypoint::setAutocontinue(bool autoContinue)
{
    if (this->autocontinue != autocontinue)
    {
        this->autocontinue = autoContinue;
        emit changed(this);
    }
}

void Waypoint::setCurrent(bool current)
{
    if (this->current != current)
    {
        this->current = current;
        emit changed(this);
    }
}

void Waypoint::setAcceptanceRadius(double radius)
{
    if (this->param1 != radius)
    {
        this->param1 = radius;
        emit changed(this);
    }
}

void Waypoint::setParam1(double param1)
{
    if (this->param1 != param1)
    {
        this->param1 = param1;
        emit changed(this);
    }
}

void Waypoint::setParam2(double param2)
{
    if (this->param2 != param2)
    {
        this->param2 = param2;
        emit changed(this);
    }
}

void Waypoint::setParam3(double param3)
{
    if (this->orbit != param3)
    {
        this->orbit = param3;
        emit changed(this);
    }
}

void Waypoint::setParam4(double param4)
{
    if (this->x != param4)
    {
        this->x = param4;
        emit changed(this);
    }
}

void Waypoint::setParam5(double param5)
{
    if (this->y != param5)
    {
        this->y = param5;
        emit changed(this);
    }
}

void Waypoint::setParam6(double param6)
{
    if (this->z != param6)
    {
        this->z = param6;
        emit changed(this);
    }
}

void Waypoint::setLoiterOrbit(double orbit)
{
    if (this->orbit != orbit)
    {
        this->orbit = orbit;
        emit changed(this);
    }
}

void Waypoint::setHoldTime(int holdTime)
{
    if (this->param2 != holdTime)
    {
        this->param2 = holdTime;
        emit changed(this);
    }
}

void Waypoint::setTurns(int turns)
{
    if (this->param1 != turns)
    {
        this->param1 = turns;
        emit changed(this);
    }
}

//void Waypoint::setX(double x)
//{
//    if (this->x != static_cast<double>(x))
//    {
//        this->x = x;
//        emit changed(this);
//    }
//}

//void Waypoint::setY(double y)
//{
//    if (this->y != static_cast<double>(y))
//    {
//        this->y = y;
//        emit changed(this);
//    }
//}

//void Waypoint::setZ(double z)
//{
//    if (this->z != static_cast<double>(z))
//    {
//        this->z = z;
//        emit changed(this);
//    }
//}

//void Waypoint::setYaw(double yaw)
//{
//    if (this->yaw != static_cast<double>(yaw))
//    {
//        this->yaw = yaw;
//        emit changed(this);
//    }
//}

//void Waypoint::setOrbit(double orbit)
//{
//    if (this->orbit != static_cast<double>(orbit))
//    {
//        this->orbit = orbit;
//        emit changed(this);
//    }
//}
