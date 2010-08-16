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
 *   @brief Displays one waypoint
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#ifndef WAYPOINTVIEW_H
#define WAYPOINTVIEW_H

#include <QtGui/QWidget>
#include "Waypoint.h"

namespace Ui {
    class WaypointView;
}

class WaypointView : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(WaypointView)
        public:
            explicit WaypointView(Waypoint* wp, QWidget* parent);
    virtual ~WaypointView();

public:
    void setCurrent(bool state);

public slots:
    void moveUp();
    void moveDown();
    void remove();
    void changedAutoContinue(int);
    void changedCurrent(int);
    void updateValues(void);

    void setYaw(int);   //hidden degree to radian conversion

protected:
    virtual void changeEvent(QEvent *e);
    Waypoint* wp;

private:
    Ui::WaypointView *m_ui;

signals:
    void moveUpWaypoint(Waypoint*);
    void moveDownWaypoint(Waypoint*);
    void removeWaypoint(Waypoint*);
    void currentWaypointChanged(quint16);
    void changeCurrentWaypoint(quint16);
    void setYaw(double);
};

#endif // WAYPOINTVIEW_H
