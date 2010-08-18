/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

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

Waypoint::Waypoint(quint16 _id, float _x, float _y, float _z, float _yaw, bool _autocontinue, bool _current, float _orbit, int _holdTime)
: id(_id),
  x(_x),
  y(_y),
  z(_z),
  yaw(_yaw),
  autocontinue(_autocontinue),
  current(_current),
  orbit(_orbit),
  holdTime(_holdTime)
{
}

Waypoint::~Waypoint()
{

}

void Waypoint::save(QTextStream &saveStream)
{
    saveStream << "\t" << this->getId() << "\t" << this->getX() << "\t" << this->getY()  << "\t" << this->getZ()  << "\t" << this->getYaw()  << "\t" << this->getAutoContinue() << "\t" << this->getCurrent() << "\t" << this->getOrbit() << "\t" << this->getHoldTime() << "\n";
}

bool Waypoint::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 10)
    {
        this->id = wpParams[1].toInt();
        this->x = wpParams[2].toDouble();
        this->y = wpParams[3].toDouble();
        this->z = wpParams[4].toDouble();
        this->yaw = wpParams[5].toDouble();
        this->autocontinue = (wpParams[6].toInt() == 1 ? true : false);
        this->current = (wpParams[7].toInt() == 1 ? true : false);
        this->orbit = wpParams[8].toDouble();
        this->holdTime = wpParams[9].toInt();
        return true;
    }
    return false;
}

void Waypoint::setId(quint16 id)
{
    this->id = id;
}

void Waypoint::setX(float x)
{
    this->x = x;
}

void Waypoint::setY(float y)
{
    this->y = y;
}

void Waypoint::setZ(float z)
{
    this->z = z;
}

void Waypoint::setYaw(float yaw)
{
    this->yaw = yaw;
}

void Waypoint::setAutocontinue(bool autoContinue)
{
    this->autocontinue = autoContinue;
}

void Waypoint::setCurrent(bool current)
{
    this->current = current;
}

void Waypoint::setOrbit(float orbit)
{
    this->orbit = orbit;
}

void Waypoint::setHoldTime(int holdTime)
{
    this->holdTime = holdTime;
}

void Waypoint::setX(double x)
{
    this->x = x;
}

void Waypoint::setY(double y)
{
    this->y = y;
}

void Waypoint::setZ(double z)
{
    this->z = z;
}

void Waypoint::setYaw(double yaw)
{
    this->yaw = yaw;
}

void Waypoint::setOrbit(double orbit)
{
    this->orbit = orbit;
}
