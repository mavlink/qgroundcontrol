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
 *   @brief Definition of the waypoint protocol handler
 *
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#ifndef UASWAYPOINTMANAGER_H
#define UASWAYPOINTMANAGER_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include "Waypoint.h"
#include <mavlink.h>
class UAS;

class UASWaypointManager : public QObject
{
Q_OBJECT
private:
    enum WaypointState {
        WP_IDLE = 0,
        WP_SENDLIST,
        WP_SENDLIST_SENDWPS,
        WP_GETLIST,
        WP_GETLIST_GETWPS
    }; ///< The possible states for the waypoint protocol

public:
    UASWaypointManager(UAS&);

    void handleWaypointCount(quint8 systemId, quint8 compId, quint16 count);
    void handleWaypoint(quint8 systemId, quint8 compId, mavlink_waypoint_t *wp);
    void handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_waypoint_request_t *wpr);
    void handleWaypointReached(quint8 systemId, quint8 compId, mavlink_waypoint_reached_t *wpr);
    void handleWaypointSetCurrent(quint8 systemId, quint8 compId, mavlink_waypoint_set_current_t *wpr);

private:
    void sendWaypointRequest(quint16 seq);
    void sendWaypoint(quint16 seq);

public slots:
    void timeout();
    void clearWaypointList();
    void requestWaypoints();
    void sendWaypoints(const QVector<Waypoint *> &list);

signals:
    void waypointUpdated(int,quint16,double,double,double,double,bool,bool);    ///< Adds a waypoint to the waypoint list widget
    void currentWaypointChanged(quint16);                                       ///< emits the new current waypoint sequence number
    void updateStatusString(const QString &);                                   ///< emits the current status string

private:
    UAS &uas;                                       ///< Reference to the corresponding UAS
    quint16 current_wp_id;                          ///< The last used waypoint ID in the current protocol transaction
    quint16 current_count;                          ///< The number of waypoints in the current protocol transaction
    WaypointState current_state;                    ///< The current protocol state
    quint8 current_partner_systemid;                ///< The current protocol communication target system
    quint8 current_partner_compid;                  ///< The current protocol communication target component

    QVector<mavlink_waypoint_t *> waypoint_buffer;  ///< communication buffer for waypoints
    QTimer protocol_timer;                          ///< Timer to catch timeouts
};

#endif // UASWAYPOINTMANAGER_H
