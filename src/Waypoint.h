/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

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

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <QObject>
#include <QString>

class Waypoint : public QObject
{
    Q_OBJECT

public:
    Waypoint(quint16 id = 0, float x = 0.0f, float y = 0.0f, float z = 0.0f, float yaw = 0.0f, bool autocontinue = false, bool current = false);
    ~Waypoint();

    quint16 getId() const { return id; }
    float getX() const { return x; }
    float getY() const { return y; }
    float getZ() const { return z; }
    float getYaw() const { return yaw; }
    bool getAutoContinue() const { return autocontinue; }
    bool getCurrent() const { return current; }

private:
    quint16 id;
    float x;
    float y;
    float z;
    float yaw;
    bool autocontinue;
    bool current;

public slots:
    void setId(quint16 id);
    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setYaw(float yaw);
    void setAutocontinue(bool autoContinue);
    void setCurrent(bool current);
};

#endif // WAYPOINT_H
