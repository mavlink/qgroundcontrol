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

Waypoint::Waypoint(quint16 id, float x, float y, float z, float yaw, bool autocontinue, bool current)
{
    this->id = id;
    this->x = x;
    this->y = y;
    this->z = z;
    this->yaw = yaw;
    this->autocontinue = autocontinue;
    this->current = current;
}

Waypoint::~Waypoint()
{

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
