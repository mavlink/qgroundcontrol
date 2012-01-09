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
#include "WaypointEditableView.h"
#include "WaypointViewOnlyView.h"
#include "UnconnectedUASInfoWidget.h"
//#include "PopupMessage.h"


namespace Ui
{
class WaypointList;
}

class WaypointList : public QWidget
{
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
    /** @brief Read the remote waypoint list to both tabs */
    void read();
    /** @brief Read the remote waypoint list to "view"-tab only*/
    void refresh();
    /** @brief Add a waypoint to "edit"-tab */
    void addEditable();
    /** @brief Add a waypoint to "view"-tab */
   // void addViewOnly();
    /** @brief Add a waypoint at the current MAV position */
    void addCurrentPositionWaypoint();
    /** @brief Add a waypoint by mouse click over the map */

    //Update events
    /** @brief sets statusLabel string */
    void updateStatusLabel(const QString &string);
    /** @brief The user wants to change the current waypoint */
    void changeCurrentWaypoint(quint16 seq);
    /** @brief Current waypoint in edit-tab was changed, so the list must be updated (to contain only one waypoint checked as "current")  */
    void currentWaypointEditableChanged(quint16 seq);
    /** @brief Current waypoint on UAV was changed, update view-tab  */
    void currentWaypointViewOnlyChanged(quint16 seq);
    /** @brief The waypoint manager informs that one editable waypoint was changed */
    void updateWaypointEditable(int uas, Waypoint* wp);
    /** @brief The waypoint manager informs that one viewonly waypoint was changed */
    void updateWaypointViewOnly(int uas, Waypoint* wp);
    /** @brief The waypoint manager informs that the editable waypoint list was changed */
    void waypointEditableListChanged(void);
    /** @brief The waypoint manager informs that the waypoint list on the MAV was changed */
    void waypointViewOnlyListChanged(void);

//    /** @brief The MapWidget informs that a waypoint global was changed on the map */
//    void waypointGlobalChanged(const QPointF coordinate, const int indexWP);

    void clearWPWidget();

    //void changeWPPositionBySpinBox(Waypoint* wp);

    // Waypoint operations
    void moveUp(Waypoint* wp);
    void moveDown(Waypoint* wp);
    void removeWaypoint(Waypoint* wp);

//    void setIsLoadFileWP();
//    void setIsReadGlobalWP(bool value);




signals:
    void clearPathclicked();
    void createWaypointAtMap(const QPointF coordinate);

protected:
    virtual void changeEvent(QEvent *e);

protected:
    QMap<Waypoint*, WaypointEditableView*> wpEditableViews;
    QMap<Waypoint*, WaypointViewOnlyView*> wpViewOnlyViews;
    QVBoxLayout* viewOnlyListLayout;
    QVBoxLayout* editableListLayout;
    UASInterface* uas;
    UASWaypointManager* WPM;
    double mavX;
    double mavY;
    double mavZ;
    double mavYaw;
    QPointF centerMapCoordinate;
    bool loadFileGlobalWP;
    bool readGlobalWP;
    bool showOfflineWarning;

private:
    Ui::WaypointList *m_ui;





private slots:
    void on_clearWPListButton_clicked();

};

#endif // WAYPOINTLIST_H
