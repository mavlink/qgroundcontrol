/*===================================================================
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
#include <iostream>

namespace Ui {
    class WaypointView;
}
class Ui_QGCCustomWaypointAction;

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
    /** @brief Waypoint matching this widget has been deleted */
    void deleted(QObject* waypoint);
    void changedAutoContinue(int);
    void updateFrameView(int frame);
    void changedFrame(int state);
    void updateActionView(int action);
    void changedAction(int state);
    void changedCurrent(int);
    void updateValues(void);
    
    void setYaw(int);   //hidden degree to radian conversion
    
protected:
    virtual void changeEvent(QEvent *e);
    Waypoint* wp;
    // Special widgets extendending the
    // waypoint view to mission capabilities
    Ui_QGCCustomWaypointAction* customCommand;
    
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
