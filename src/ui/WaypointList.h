/*=====================================================================

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
#include "WaypointGlobalView.h"


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
    void updatePosition(UASInterface*, double x, double y, double z, quint64 usec);
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
    /** @brief Add a waypoint by mouse click over the map */
    void addWaypointMouse(QPointF coordinate);
    /** @brief it notifies that a global waypoint goes to do created */
    //void setIsWPGlobal(bool value, QPointF centerCoordinate);


    //Update events
    /** @brief sets statusLabel string */
    void updateStatusLabel(const QString &string);
    /** @brief The user wants to change the current waypoint */
    void changeCurrentWaypoint(quint16 seq);
    /** @brief The waypoint planner changed the current waypoint */
    void currentWaypointChanged(quint16 seq);
    /** @brief The waypoint manager informs that the waypoint list was changed */
    void waypointListChanged(void);

    /** @brief The MapWidget informs that a waypoint global was changed on the map */
    void waypointGlobalChanged(const QPointF coordinate, const int indexWP);

    void clearWPWidget();

    //void changeWPPositionBySpinBox(Waypoint* wp);

    // Waypoint operations
    void moveUp(Waypoint* wp);
    void moveDown(Waypoint* wp);
    void removeWaypoint(Waypoint* wp);




signals:
  void clearPathclicked();
  void createWaypointAtMap(const QPointF coordinate);
 // void ChangeWaypointGlobalPosition(int index, QPointF coord);
  void changePositionWPBySpinBox(int indexWP, float lat, float lon);



protected:
    virtual void changeEvent(QEvent *e);

protected:
    QMap<Waypoint*, WaypointView*> wpViews;
    //QMap<Waypoint*, WaypointGlobalView*> wpGlobalViews;
    QVBoxLayout* listLayout;
    UASInterface* uas;
    double mavX;
    double mavY;
    double mavZ;
    double mavYaw;
    QPointF centerMapCoordinate;

private:
    Ui::WaypointList *m_ui;





private slots:
    void on_clearWPListButton_clicked();

};

#endif // WAYPOINTLIST_H
