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
#include "QGCMAVLink.h"
class UAS;
class UASInterface;

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
    UASWaypointManager(UAS* uas=NULL);   ///< Standard constructor
    ~UASWaypointManager();

    /** @name Received message handlers */
    /*@{*/
    void handleWaypointCount(quint8 systemId, quint8 compId, quint16 count);                            ///< Handles received waypoint count messages
    void handleWaypoint(quint8 systemId, quint8 compId, mavlink_mission_item_t *wp);                        ///< Handles received waypoint messages
    void handleWaypointAck(quint8 systemId, quint8 compId, mavlink_mission_ack_t *wpa);                ///< Handles received waypoint ack messages
    void handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_mission_request_t *wpr);        ///< Handles received waypoint request messages
    void handleWaypointReached(quint8 systemId, quint8 compId, mavlink_mission_item_reached_t *wpr);        ///< Handles received waypoint reached messages
    void handleWaypointCurrent(quint8 systemId, quint8 compId, mavlink_mission_current_t *wpc);        ///< Handles received set current waypoint messages
    /*@}*/

    /** @name Remote operations */
    /*@{*/
    void clearWaypointList();                       ///< Sends the waypoint clear all message to the MAV

    void readWaypoints(bool read_to_edit=false);    ///< Requests the MAV's current waypoint list.
    void writeWaypoints();                          ///< Sends the waypoint list to the MAV
    int setCurrentWaypoint(quint16 seq);            ///< Sends the sequence number of the waypoint that should get the new target waypoint to the UAS
    int setCurrentEditable(quint16 seq);          ///< Changes the current waypoint in edit tab
    /*@}*/

    /** @name Waypoint list operations */
    /*@{*/
    const QVector<Waypoint *> &getWaypointEditableList(void) {
        return waypointsEditable;    ///< Returns a const reference to the waypoint list.
    }
    const QVector<Waypoint *> &getWaypointViewOnlyList(void) {
        return waypointsViewOnly;    ///< Returns a const reference to the waypoint list.
    }
    const QVector<Waypoint *> getGlobalFrameWaypointList();  ///< Returns a global waypoint list
    const QVector<Waypoint *> getGlobalFrameAndNavTypeWaypointList(); ///< Returns a global waypoint list containing only waypoints suitable for navigation. Actions and other mission items are filtered out.
    const QVector<Waypoint *> getNavTypeWaypointList(); ///< Returns a waypoint list containing only waypoints suitable for navigation. Actions and other mission items are filtered out.
    int getIndexOf(Waypoint* wp);                   ///< Get the index of a waypoint in the list
    int getGlobalFrameIndexOf(Waypoint* wp);    ///< Get the index of a waypoint in the list, counting only global waypoints
    int getGlobalFrameAndNavTypeIndexOf(Waypoint* wp); ///< Get the index of a waypoint in the list, counting only global AND navigation mode waypoints
    int getNavTypeIndexOf(Waypoint* wp); ///< Get the index of a waypoint in the list, counting only navigation mode waypoints
    int getLocalFrameIndexOf(Waypoint* wp);     ///< Get the index of a waypoint in the list, counting only local waypoints
    int getMissionFrameIndexOf(Waypoint* wp);   ///< Get the index of a waypoint in the list, counting only mission waypoints
    int getGlobalFrameCount(); ///< Get the count of global waypoints in the list
    int getGlobalFrameAndNavTypeCount(); ///< Get the count of global waypoints in navigation mode in the list
    int getNavTypeCount(); ///< Get the count of global waypoints in navigation mode in the list
    int getLocalFrameCount();   ///< Get the count of local waypoints in the list
    /*@}*/

    UAS* getUAS() {
        return this->uas;    ///< Returns the owning UAS
    }

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
    /** @name Waypoint list operations */
    /*@{*/
    void addWaypointEditable(Waypoint *wp, bool enforceFirstActive=true);                 ///< adds a new waypoint to the end of the editable list and changes its sequence number accordingly
    void addWaypointViewOnly(Waypoint *wp);                                               ///< adds a new waypoint to the end of the view-only list and changes its sequence number accordingly
    Waypoint* createWaypoint(bool enforceFirstActive=true);     ///< Creates a waypoint
    int removeWaypoint(quint16 seq);                       ///< locally remove the specified waypoint from the storage
    void moveWaypoint(quint16 cur_seq, quint16 new_seq);   ///< locally move a waypoint from its current position cur_seq to a new position new_seq
    void saveWaypoints(const QString &saveFile);           ///< saves the local waypoint list to saveFile
    void loadWaypoints(const QString &loadFile);           ///< loads a waypoint list from loadFile
    void notifyOfChangeEditable(Waypoint* wp);             ///< Notifies manager to changes to an editable waypoint
    void notifyOfChangeViewOnly(Waypoint* wp);             ///< Notifies manager to changes to a viewonly waypoint, e.g. some widget wants to change "current"
    /*@}*/
    void handleLocalPositionChanged(UASInterface* mav, double x, double y, double z, quint64 time);
    void handleGlobalPositionChanged(UASInterface* mav, double lat, double lon, double alt, quint64 time);

signals:
    void waypointEditableListChanged(void);                 ///< emits signal that the list of editable waypoints has been changed
    void waypointEditableListChanged(int uasid);            ///< emits signal that the list of editable waypoints has been changed
    void waypointEditableChanged(int uasid, Waypoint* wp);  ///< emits signal that a single editable waypoint has been changed
    void waypointViewOnlyListChanged(void);                 ///< emits signal that the list of editable waypoints has been changed
    void waypointViewOnlyListChanged(int uasid);            ///< emits signal that the list of editable waypoints has been changed
    void waypointViewOnlyChanged(int uasid, Waypoint* wp);  ///< emits signal that a single editable waypoint has been changed
    void currentWaypointChanged(quint16);           ///< emits the new current waypoint sequence number
    void updateStatusString(const QString &);       ///< emits the current status string
    void waypointDistanceChanged(double distance);   ///< Distance to next waypoint changed (in meters)

    void loadWPFile();                              ///< emits signal that a file wp has been load
    void readGlobalWPFromUAS(bool value);           ///< emits signal when finish to read Global WP from UAS

private:
    UAS* uas;                                       ///< Reference to the corresponding UAS
    quint32 current_retries;                        ///< The current number of retries left
    quint16 current_wp_id;                          ///< The last used waypoint ID in the current protocol transaction
    quint16 current_count;                          ///< The number of waypoints in the current protocol transaction
    WaypointState current_state;                    ///< The current protocol state
    quint8 current_partner_systemid;                ///< The current protocol communication target system
    quint8 current_partner_compid;                  ///< The current protocol communication target component
    bool read_to_edit;                              ///< If true, after readWaypoints() incoming waypoints will be copied both to "edit"-tab and "view"-tab. Otherwise, only to "view"-tab.

    QVector<Waypoint *> waypointsViewOnly;                  ///< local copy of current waypoint list on MAV
    QVector<Waypoint *> waypointsEditable;                  ///< local editable waypoint list
    Waypoint* currentWaypointEditable;                      ///< The currently used waypoint
    QVector<mavlink_mission_item_t *> waypoint_buffer;  ///< buffer for waypoints during communication
    QTimer protocol_timer;                          ///< Timer to catch timeouts
    bool standalone;                                ///< If standalone is set, do not write to UAS
    quint16 uasid;
};

#endif // UASWAYPOINTMANAGER_H
