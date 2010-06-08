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
    void setUAS(UASInterface* uas);
    void redrawList();
    void reenableTransmit();

    //UI Buttons
    void saveWaypoints();
    void loadWaypoints();    
    void transmit();
    void add();
    void moveUp(Waypoint* wp);
    void moveDown(Waypoint* wp);
    void setCurrentWaypoint(Waypoint* wp);

    //To be moved to UASWaypointManager (?)
    void setWaypoint(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool current);
    void addWaypoint(Waypoint* wp);
    void removeWaypointAndName(Waypoint* wp);
    void waypointReached(UASInterface* uas, int waypointId);

protected:
    virtual void changeEvent(QEvent *e);
    void debugOutputWaypoints();

    QVector<Waypoint*> waypoints;
    QMap<int, QString> waypointNames;
    QMap<Waypoint*, WaypointView*> wpViews;
    QVBoxLayout* listLayout;
    QTimer* transmitDelay;
    UASInterface* uas;

private:
    Ui::WaypointList *m_ui;
    void removeWaypoint(Waypoint* wp);

signals:
    void waypointChanged(Waypoint*);
    void currentWaypointChanged(int);
    void removeWaypointId(int);
    void requestWaypoints();
    void clearWaypointList();
};

#endif // WAYPOINTLIST_H
