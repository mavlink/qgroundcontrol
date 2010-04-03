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
    Waypoint(QString name, int id = 0, double x = 0.0f, double y = 0.0f, double z = 0.0f, double yaw = 0.0f, bool autocontinue = false, bool current = false);
    ~Waypoint();

    int getId();

    int id;

    double x;
    double y;
    double z;

    double yaw;
    bool autocontinue;
    bool current;

    QString name;


public slots:
    void setName(QString name);
    void setX(double x);
    void setY(double y);
    void setZ(double z);
    void setYaw(double yaw);
    void setAutocontinue(bool autoContinue);
    void setCurrent(bool current);


};

#endif // WAYPOINT_H
