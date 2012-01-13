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
#include "QGCMAVLink.h"
#include "QGC.h"

class Waypoint : public QObject
{
    Q_OBJECT

public:
    Waypoint(quint16 id = 0, double x = 0.0, double y = 0.0, double z = 0.0, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0,
             bool autocontinue = true, bool current = false, MAV_FRAME frame=MAV_FRAME_GLOBAL, MAV_CMD action=MAV_CMD_NAV_WAYPOINT, const QString& description=QString(""));
    ~Waypoint();

    quint16 getId() const {
        return id;
    }
    double getX() const {
        return x;
    }
    double getY() const {
        return y;
    }
    double getZ() const {
        return z;
    }
    double getLatitude() const {
        return x;
    }
    double getLongitude() const {
        return y;
    }
    double getAltitude() const {
        return z;
    }
    double getYaw() const {
        return yaw;
    }
    bool getAutoContinue() const {
        return autocontinue;
    }
    bool getCurrent() const {
        return current;
    }
    double getLoiterOrbit() const {
        return orbit;
    }
    double getAcceptanceRadius() const {
        return param2;
    }
    double getHoldTime() const {
        return param1;
    }
    double getParam1() const {
        return param1;
    }
    double getParam2() const {
        return param2;
    }
    double getParam3() const {
        return orbit;
    }
    double getParam4() const {
        return yaw;
    }
    double getParam5() const {
        return x;
    }
    double getParam6() const {
        return y;
    }
    double getParam7() const {
        return z;
    }
    int getTurns() const {
        return param1;
    }
    MAV_FRAME getFrame() const {
        return frame;
    }
    MAV_CMD getAction() const {
        return action;
    }
    const QString& getName() const {
        return name;
    }
    const QString& getDescription() const {
        return description;
    }
    /** @brief Returns true if x, y, z contain reasonable navigation data */
    bool isNavigationType();

    void save(QTextStream &saveStream);
    bool load(QTextStream &loadStream);


protected:
    quint16 id;
    double x;
    double y;
    double z;
    double yaw;
    MAV_FRAME frame;
    MAV_CMD action;
    bool autocontinue;
    bool current;
    double orbit;
    double param1;
    double param2;
    int turns;
    QString name;
    QString description;
    quint64 reachedTime;

public slots:
    void setId(quint16 id);
    void setX(double x);
    void setY(double y);
    void setZ(double z);
    void setLatitude(double lat);
    void setLongitude(double lon);
    void setAltitude(double alt);
    /** @brief Yaw angle in COMPASS DEGREES: 0-360 */
    void setYaw(int yaw);
    /** @brief Yaw angle in COMPASS DEGREES: 0-360 */
    void setYaw(double yaw);
    /** @brief Set the waypoint action */
    void setAction(int action);
    void setAction(MAV_CMD action);
    void setFrame(MAV_FRAME frame);
    void setAutocontinue(bool autoContinue);
    void setCurrent(bool current);
    void setLoiterOrbit(double orbit);
    void setParam1(double param1);
    void setParam2(double param2);
    void setParam3(double param3);
    void setParam4(double param4);
    void setParam5(double param5);
    void setParam6(double param6);
    void setParam7(double param7);
    void setAcceptanceRadius(double radius);
    void setHoldTime(int holdTime);
    void setHoldTime(double holdTime);
    /** @brief Number of turns for loiter waypoints */
    void setTurns(int turns);
    /** @brief Set waypoint as reached */
    void setReached() { reachedTime = QGC::groundTimeMilliseconds(); }
    /** @brief Wether this waypoint has been reached yet */
    bool isReached() { return (reachedTime > 0); }
    /** @brief Get the time this waypoint was reached */
    quint64 getReachedTime() { return reachedTime; }

signals:
    /** @brief Announces a change to the waypoint data */
    void changed(Waypoint* wp);    
};

#endif // WAYPOINT_H
