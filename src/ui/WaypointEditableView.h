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
 *   @author Alex Trofimov <talex@student.ethz.ch>
 */

#ifndef WAYPOINTEDITABLEVIEW_H
#define WAYPOINTEDITABLEVIEW_H

#include <QtGui/QWidget>
#include "Waypoint.h"
#include <iostream>

enum QGC_WAYPOINTEDITABLEVIEW_MODE {
    QGC_WAYPOINTEDITABLEVIEW_MODE_DEFAULT,
    QGC_WAYPOINTEDITABLEVIEW_MODE_DIRECT_EDITING
};

namespace Ui
{
class WaypointEditableView;
}
class QGCMissionNavWaypoint;
class QGCMissionNavLoiterUnlim;
class QGCMissionNavLoiterTurns;
class QGCMissionNavLoiterTime;
class QGCMissionNavReturnToLaunch;
class QGCMissionNavLand;
class QGCMissionNavTakeoff;
class QGCMissionNavSweep;
class QGCMissionDoJump;
class QGCMissionDoStartSearch;
class QGCMissionDoFinishSearch;
class QGCMissionConditionDelay;
class QGCMissionOther;

class WaypointEditableView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(WaypointEditableView)
public:
    explicit WaypointEditableView(Waypoint* wp, QWidget* parent);
    virtual ~WaypointEditableView();

public:
    void setCurrent(bool state);

public slots:
    void moveUp();
    void moveDown();
    void remove();
    /** @brief Waypoint matching this widget has been deleted */
    void deleted(QObject* waypoint);
    void changedAutoContinue(int);    
    void changedFrame(int state);
    void updateActionView(int action);
    void initializeActionView(int action);

    void changedCurrent(int);
    void updateValues(void);
    void changedAction(int state); //change commandID, including the view
    void changedCommand(int mav_cmd_id); //only update WP->command, but do not change the view. Should only be used for "other" waypoint-type.
    void changedParam1(double value);
    void changedParam2(double value);
    void changedParam3(double value);
    void changedParam4(double value);
    void changedParam5(double value);
    void changedParam6(double value);
    void changedParam7(double value);

protected slots:

protected:
    virtual void changeEvent(QEvent *e);
    Waypoint* wp;
    QGC_WAYPOINTEDITABLEVIEW_MODE viewMode;
    // Widgets for every mission element
    QGCMissionNavWaypoint* MissionNavWaypointWidget;
    QGCMissionNavLoiterUnlim* MissionNavLoiterUnlimWidget;
    QGCMissionNavLoiterTurns* MissionNavLoiterTurnsWidget;
    QGCMissionNavLoiterTime* MissionNavLoiterTimeWidget;
    QGCMissionNavReturnToLaunch* MissionNavReturnToLaunchWidget;
    QGCMissionNavLand* MissionNavLandWidget;
    QGCMissionNavTakeoff* MissionNavTakeoffWidget;
    QGCMissionNavSweep* MissionNavSweepWidget;
    QGCMissionDoJump* MissionDoJumpWidget;
    QGCMissionDoStartSearch* MissionDoStartSearchWidget;
    QGCMissionDoFinishSearch* MissionDoFinishSearchWidget;
    QGCMissionConditionDelay* MissionConditionDelayWidget;
    QGCMissionOther* MissionOtherWidget;


private:
    Ui::WaypointEditableView *m_ui;

signals:
    void moveUpWaypoint(Waypoint*);
    void moveDownWaypoint(Waypoint*);
    void removeWaypoint(Waypoint*);    
    void changeCurrentWaypoint(quint16);
    void setYaw(double);

    void commandBroadcast(int mav_cmd_id);
    void frameBroadcast(MAV_FRAME frame);
    void param1Broadcast(double value);
    void param2Broadcast(double value);
    void param3Broadcast(double value);
    void param4Broadcast(double value);
    void param5Broadcast(double value);
    void param6Broadcast(double value);
    void param7Broadcast(double value);
};

#endif // WAYPOINTEDITABLEVIEW_H
