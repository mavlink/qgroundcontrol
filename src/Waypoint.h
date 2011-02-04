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
#include <QTextStream>
#include "mavlink_types.h"

class Waypoint : public QObject
{
    Q_OBJECT

public:
    Waypoint(quint16 id = 0, double x = 0.0f, double y = 0.0f, double z = 0.0f, double yaw = 0.0f, bool autocontinue = false,
             bool current = false, double orbit = 0.15f, int holdTime = 0,
             MAV_FRAME frame=MAV_FRAME_GLOBAL, MAV_ACTION action=MAV_ACTION_NAVIGATE);
    ~Waypoint();

    quint16 getId() const { return id; }
    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }
    double getYaw() const { return yaw; }
    bool getAutoContinue() const { return autocontinue; }
    bool getCurrent() const { return current; }
    double getOrbit() const { return orbit; }
    double getHoldTime() const { return holdTime; }
    MAV_FRAME getFrame() const { return frame; }
    MAV_ACTION getAction() const { return action; }
    const QString& getName() const { return name; }

    void save(QTextStream &saveStream);
    bool load(QTextStream &loadStream);


protected:
    quint16 id;
    double x;
    double y;
    double z;
    double yaw;
    MAV_FRAME frame;
    MAV_ACTION action;
    bool autocontinue;
    bool current;
    double orbit;
    int holdTime;
    QString name;

public slots:
    void setId(quint16 id);
    void setX(double x);
    void setY(double y);
    void setZ(double z);
    void setYaw(double yaw);
    void setAction(MAV_ACTION action);
    void setFrame(MAV_FRAME frame);
    void setAutocontinue(bool autoContinue);
    void setCurrent(bool current);
    void setOrbit(double orbit);
    void setHoldTime(int holdTime);


//    //for QDoubleSpin
//    void setX(double x);
//    void setY(double y);
//    void setZ(double z);
//    void setYaw(double yaw);
//    void setOrbit(double orbit);

signals:
    /** @brief Announces a change to the waypoint data */
    void changed(Waypoint* wp);
};

#endif // WAYPOINT_H
