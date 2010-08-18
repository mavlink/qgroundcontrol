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
 *   @brief Definition of list of waypoints widget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#ifndef WAYPOINTLIST_H
#define WAYPOINTLIST_H

#include <QtGui/QWidget>
#include <QMap>
#include <QVBoxLayout>
#include <QTimer>
#include "Waypoint.h"
#include "UASInterface.h"
#include "WaypointView.h"

namespace Ui {
    class WaypointList;
}

class WaypointList : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(WaypointList)
        public:
            WaypointList(QWidget* parent = NULL, UASInterface* uas = NULL);
    virtual ~WaypointList();

public slots:
    void updateLocalPosition(UASInterface*, double x, double y, double z, quint64 usec);
    void updateAttitude(UASInterface*, double roll, double pitch, double yaw, quint64 usec);

    void setUAS(UASInterface* uas);

    //Waypoint list operations
    /** @brief Save the local waypoint list to a file */
    void saveWaypoints();
    /** @brief Load a waypoint list from a file */
    void loadWaypoints();    
    /** @brief Transmit the local waypoint list to the UAS */
    void transmit();
    /** @brief Read the remote waypoint list */
    void read();
    /** @brief Add a waypoint */
    void add();
    /** @brief Add a waypoint at the current MAV position */
    void addCurrentPositonWaypoint();

    //Update events
    /** @brief sets statusLabel string */
    void updateStatusLabel(const QString &string);
    /** @brief The user wants to change the current waypoint */
    void changeCurrentWaypoint(quint16 seq);
    /** @brief The waypoint planner changed the current waypoint */
    void currentWaypointChanged(quint16 seq);
    /** @brief The waypoint manager informs that the waypoint list was changed */
    void waypointListChanged(void);

    // Waypoint operations
    void moveUp(Waypoint* wp);
    void moveDown(Waypoint* wp);
    void removeWaypoint(Waypoint* wp);

protected:
    virtual void changeEvent(QEvent *e);

protected:
    QMap<Waypoint*, WaypointView*> wpViews;
    QVBoxLayout* listLayout;
    UASInterface* uas;
    double mavX;
    double mavY;
    double mavZ;
    double mavYaw;

private:
    Ui::WaypointList *m_ui;
};

#endif // WAYPOINTLIST_H
