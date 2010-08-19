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

/**
 * @brief Implementation of the MAVLINK waypoint protocol
 *
 * This class handles the communication with a waypoint manager on the MAV.
 * All waypoints are stored in the QVector waypoints, modifications can be done with the WaypointList widget.
 * Notice that currently the access to the internal waypoint storage is not guarded nor thread-safe. This works as long as no other widget alters the data.
 *
 * See http://qgroundcontrol.org/waypoint_protocol for more information about the protocol and the states.
 */
class UASWaypointManager : public QObject
{
Q_OBJECT
private:
    enum WaypointState {
        WP_IDLE = 0,        ///< Waiting for commands
        WP_SENDLIST,        ///< Initial state for sending waypoints to the MAV
        WP_SENDLIST_SENDWPS,///< Sending waypoints
        WP_GETLIST,         ///< Initial state for retrieving wayppoints from the MAV
        WP_GETLIST_GETWPS,  ///< Receiving waypoints
        WP_CLEARLIST,       ///< Clearing waypoint list on the MAV
        WP_SETCURRENT       ///< Setting new current waypoint on the MAV
    }; ///< The possible states for the waypoint protocol

public:
    UASWaypointManager(UAS&);   ///< Standard constructor.

    /** @name Received message handlers */
    /*@{*/
    void handleWaypointCount(quint8 systemId, quint8 compId, quint16 count);                            ///< Handles received waypoint count messages
    void handleWaypoint(quint8 systemId, quint8 compId, mavlink_waypoint_t *wp);                        ///< Handles received waypoint messages
    void handleWaypointAck(quint8 systemId, quint8 compId, mavlink_waypoint_ack_t *wpa);                ///< Handles received waypoint ack messages
    void handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_waypoint_request_t *wpr);        ///< Handles received waypoint request messages
    void handleWaypointReached(quint8 systemId, quint8 compId, mavlink_waypoint_reached_t *wpr);        ///< Handles received waypoint reached messages
    void handleWaypointCurrent(quint8 systemId, quint8 compId, mavlink_waypoint_current_t *wpc);        ///< Handles received set current waypoint messages
    /*@}*/

    /** @name Remote operations */
    /*@{*/
    void clearWaypointList();                       ///< Sends the waypoint clear all message to the MAV
    void readWaypoints();                           ///< Requests the MAV's current waypoint list
    void writeWaypoints();                          ///< Sends the local waypoint list to the MAV
    int setCurrentWaypoint(quint16 seq);            ///< Changes the current waypoint and sends the sequence number of the waypoint that should get the new target waypoint to the UAS
    /*@}*/

    /** @name Local waypoint list operations */
    /*@{*/
    const QVector<Waypoint *> &getWaypointList(void) { return waypoints; }  ///< Returns a const reference to the local waypoint list.
    void localAddWaypoint(Waypoint *wp);                        ///< locally adds a new waypoint to the end of the list and changes its sequence number accordingly
    int localRemoveWaypoint(quint16 seq);                       ///< locally remove the specified waypoint from the storage
    void localMoveWaypoint(quint16 cur_seq, quint16 new_seq);   ///< locally move a waypoint from its current position cur_seq to a new position new_seq
    void localSaveWaypoints(const QString &saveFile);           ///< saves the local waypoint list to saveFile
    void localLoadWaypoints(const QString &loadFile);           ///< loads a waypoint list from loadFile
    /*@}*/

    /** @name Global waypoint list operations */
    /*@{*/
    const QVector<Waypoint *> &getGlobalWaypointList(void) { return waypoints; }  ///< Returns a const reference to the global waypoint list.
    void globalAddWaypoint(Waypoint *wp);                        ///< locally adds a new waypoint to the end of the list and changes its sequence number accordingly
    int globalRemoveWaypoint(quint16 seq);                       ///< locally remove the specified waypoint from the storage
    /*@}*/

private:
    /** @name Message send functions */
    /*@{*/
    void sendWaypointClearAll();
    void sendWaypointSetCurrent(quint16 seq);
    void sendWaypointCount();
    void sendWaypointRequestList();
    void sendWaypointRequest(quint16 seq);          ///< Requests a waypoint with sequence number seq
    void sendWaypoint(quint16 seq);                 ///< Sends a waypoint with sequence number seq
    void sendWaypointAck(quint8 type);              ///< Sends a waypoint ack
    /*@}*/

public slots:
    void timeout();                                 ///< Called by the timer if a response times out. Handles send retries.

signals:
    void waypointListChanged(void);                 ///< emits signal that the local waypoint list has been changed
    void currentWaypointChanged(quint16);           ///< emits the new current waypoint sequence number
    void updateStatusString(const QString &);       ///< emits the current status string

private:
    UAS &uas;                                       ///< Reference to the corresponding UAS
    quint32 current_retries;                        ///< The current number of retries left
    quint16 current_wp_id;                          ///< The last used waypoint ID in the current protocol transaction
    quint16 current_count;                          ///< The number of waypoints in the current protocol transaction
    WaypointState current_state;                    ///< The current protocol state
    quint8 current_partner_systemid;                ///< The current protocol communication target system
    quint8 current_partner_compid;                  ///< The current protocol communication target component

    QVector<Waypoint *> waypoints;                  ///< local waypoint list (main storage)
    QVector<mavlink_waypoint_t *> waypoint_buffer;  ///< buffer for waypoints during communication
    QTimer protocol_timer;                          ///< Timer to catch timeouts
};

#endif // UASWAYPOINTMANAGER_H
