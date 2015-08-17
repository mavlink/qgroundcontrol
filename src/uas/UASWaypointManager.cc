/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009-2012 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Implementation of the waypoint protocol handler
 *
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include "UASWaypointManager.h"
#include "UAS.h"
#include "mavlink_types.h"
#include "UASManager.h"
#include "QGCMessageBox.h"

#define PROTOCOL_TIMEOUT_MS 2000    ///< maximum time to wait for pending messages until timeout
#define PROTOCOL_DELAY_MS 20        ///< minimum delay between sent messages
#define PROTOCOL_MAX_RETRIES 5      ///< maximum number of send retries (after timeout)
const float UASWaypointManager::defaultAltitudeHomeOffset   = 30.0f;
UASWaypointManager::UASWaypointManager(UAS* _uas)
    : uas(_uas),
      current_retries(0),
      current_wp_id(0),
      current_count(0),
      current_state(WP_IDLE),
      current_partner_systemid(0),
      current_partner_compid(0),
      currentWaypointEditable(),
      protocol_timer(this),
	  _updateWPlist_timer(this)
{
    _offlineEditingModeMessage = tr("You are in offline editing mode. Make sure to save your mission to a file before connecting to a system - you will need to load the file into the system, the offline list will be cleared on connect.");
    
    if (uas)
    {
        uasid = uas->getUASID();
        connect(&protocol_timer, SIGNAL(timeout()), this, SLOT(timeout()));
        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(handleLocalPositionChanged(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,double,quint64)), this, SLOT(handleGlobalPositionChanged(UASInterface*,double,double,double,double,quint64)));
    }
    else
    {
        uasid = 0;
    }
    
    // We signal to ourselves here so that tiemrs are started and stopped on correct thread
    connect(this, SIGNAL(_startProtocolTimer(void)), this, SLOT(_startProtocolTimerOnThisThread(void)));
    connect(this, SIGNAL(_stopProtocolTimer(void)), this, SLOT(_stopProtocolTimerOnThisThread(void)));
	_updateWPlist_timer.setInterval(1500);
	_updateWPlist_timer.setSingleShot(true);
	connect(&_updateWPlist_timer, SIGNAL(timeout()), this, SLOT(_updateWPonTimer()));
}

UASWaypointManager::~UASWaypointManager()
{

}

void UASWaypointManager::timeout()
{
    if (current_retries > 0) {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries--;
        emit updateStatusString(tr("Timeout, retrying (retries left: %1)").arg(current_retries));

        if (current_state == WP_GETLIST) {
            sendWaypointRequestList();
        } else if (current_state == WP_GETLIST_GETWPS) {
            sendWaypointRequest(current_wp_id);
        } else if (current_state == WP_SENDLIST) {
            sendWaypointCount();
        } else if (current_state == WP_SENDLIST_SENDWPS) {
            sendWaypoint(current_wp_id);
        } else if (current_state == WP_CLEARLIST) {
            sendWaypointClearAll();
        } else if (current_state == WP_SETCURRENT) {
            sendWaypointSetCurrent(current_wp_id);
        }
    } else {
        protocol_timer.stop();

        emit updateStatusString("Operation timed out.");

        current_state = WP_IDLE;
        current_count = 0;
        current_wp_id = 0;
    }
}

void UASWaypointManager::handleLocalPositionChanged(UASInterface* mav, double x, double y, double z, quint64 time)
{
    Q_UNUSED(mav);
    Q_UNUSED(time);
    if (waypointsEditable.count() > 0 && !currentWaypointEditable.isNull() && (currentWaypointEditable->getFrame() == MAV_FRAME_LOCAL_NED || currentWaypointEditable->getFrame() == MAV_FRAME_LOCAL_ENU))
    {
        double xdiff = x-currentWaypointEditable->getX();
        double ydiff = y-currentWaypointEditable->getY();
        double zdiff = z-currentWaypointEditable->getZ();
        double dist = sqrt(xdiff*xdiff + ydiff*ydiff + zdiff*zdiff);
        emit waypointDistanceChanged(dist);
    }
}

void UASWaypointManager::handleGlobalPositionChanged(UASInterface* mav, double lat, double lon, double altAMSL, double altWGS84, quint64 time)
{
    Q_UNUSED(mav);
    Q_UNUSED(time);
    Q_UNUSED(altAMSL);
    Q_UNUSED(altWGS84);
	Q_UNUSED(lon);
	Q_UNUSED(lat);
    if (waypointsEditable.count() > 0 && !currentWaypointEditable.isNull() && (currentWaypointEditable->getFrame() == MAV_FRAME_GLOBAL || currentWaypointEditable->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT))
    {
        // TODO FIXME Calculate distance
        double dist = 0;
        emit waypointDistanceChanged(dist);
    }
}

void UASWaypointManager::handleWaypointCount(quint8 systemId, quint8 compId, quint16 count)
{
    if (current_state == WP_GETLIST && systemId == current_partner_systemid) {
        emit _startProtocolTimer(); // Start timer on correct thread
        current_retries = PROTOCOL_MAX_RETRIES;

        //Clear the old edit-list before receiving the new one
        if (read_to_edit == true){
            while(waypointsEditable.count()>0) {
                Waypoint *t = waypointsEditable[0];
                waypointsEditable.removeAt(0);
                delete t;
            }
            emit waypointEditableListChanged();
        }

        if (count > 0) {
            current_count = count;
            current_wp_id = 0;
            current_state = WP_GETLIST_GETWPS;
            sendWaypointRequest(current_wp_id);
        } else {
            emit _stopProtocolTimer();  // Stop the time on our thread
            QTime time = QTime::currentTime();
            emit updateStatusString(tr("Done. (updated at %1)").arg(time.toString()));
            current_state = WP_IDLE;
            current_count = 0;
            current_wp_id = 0;
        }


    } else {
		if (current_state != WP_GETLIST_GETWPS && systemId == current_partner_systemid)
		{
			qDebug("Requesting new waypoints. Propably changed onboard.");
			if (!_updateWPlist_timer.isActive())
			{
				current_state = WP_IDLE;
				_updateWPlist_timer.start();
			}
		}
		else
		{
			qDebug("Rejecting waypoint count message, check mismatch: current_state: %d == %d, system id %d == %d, comp id %d == %d", current_state, WP_GETLIST, current_partner_systemid, systemId, current_partner_compid, compId);
		}
	}
}

void UASWaypointManager::handleWaypoint(quint8 systemId, quint8 compId, mavlink_mission_item_t *wp)
{
    if (systemId == current_partner_systemid && current_state == WP_GETLIST_GETWPS && wp->seq == current_wp_id) {
        emit _startProtocolTimer(); // Start timer on our thread
        current_retries = PROTOCOL_MAX_RETRIES;

        if(wp->seq == current_wp_id) {

            Waypoint *lwp_vo = new Waypoint(
                NULL,
                wp->seq, wp->x,
                wp->y,
                wp->z,
                wp->param1,
                wp->param2,
                wp->param3,
                wp->param4,
                wp->autocontinue,
                wp->current,
                (MAV_FRAME) wp->frame,
                (MAV_CMD) wp->command);

            addWaypointViewOnly(lwp_vo);

            if (read_to_edit == true) {
                Waypoint *lwp_ed = new Waypoint(
                    NULL,
                    wp->seq,
                    wp->x,
                    wp->y,
                    wp->z,
                    wp->param1,
                    wp->param2,
                    wp->param3,
                    wp->param4,
                    wp->autocontinue,
                    wp->current,
                    (MAV_FRAME) wp->frame,
                    (MAV_CMD) wp->command);
                addWaypointEditable(lwp_ed, false);
                if (wp->current == 1) currentWaypointEditable = lwp_ed;
            }


            //get next waypoint
            current_wp_id++;

            if(current_wp_id < current_count) {
                sendWaypointRequest(current_wp_id);
            } else {
                sendWaypointAck(0);

                // all waypoints retrieved, change state to idle
                current_state = WP_IDLE;
                current_count = 0;
                current_wp_id = 0;

                emit _stopProtocolTimer(); // Stop timer on our thread
                emit readGlobalWPFromUAS(false);
                QTime time = QTime::currentTime();
                emit updateStatusString(tr("Done. (updated at %1)").arg(time.toString()));

            }
        } else {
            emit updateStatusString(tr("Waypoint ID mismatch, rejecting waypoint"));
        }
    } else if (systemId == current_partner_systemid
            && wp->seq < waypointsViewOnly.size() && waypointsViewOnly[wp->seq]->getAction()) {
        // accept single sent waypoints because they can contain updates about remaining DO_JUMP repetitions
        // but only update view only side
        Waypoint *lwp_vo = new Waypoint(
            NULL,
            wp->seq,
            wp->x,
            wp->y,
            wp->z,
            wp->param1,
            wp->param2,
            wp->param3,
            wp->param4,
            wp->autocontinue,
            wp->current,
            (MAV_FRAME) wp->frame,
            (MAV_CMD) wp->command);

        waypointsViewOnly.replace(wp->seq, lwp_vo);
        emit waypointViewOnlyListChanged();
        emit waypointViewOnlyListChanged(uasid);

    } else {
        qDebug("Rejecting waypoint message, check mismatch: current_state: %d == %d, system id %d == %d, comp id %d == %d", current_state, WP_GETLIST_GETWPS, current_partner_systemid, systemId, current_partner_compid, compId);
    }
}

void UASWaypointManager::handleWaypointAck(quint8 systemId, quint8 compId, mavlink_mission_ack_t *wpa)
{
    if (systemId != current_partner_systemid) {
        return;
    }

    // Check if the current partner component ID is generic. If it is, we might need to update
    if (current_partner_compid == MAV_COMP_ID_MISSIONPLANNER) {
        current_partner_compid = compId;
    }

    if (compId == current_partner_compid || compId == MAV_COMP_ID_ALL) {
        if((current_state == WP_SENDLIST || current_state == WP_SENDLIST_SENDWPS) && (current_wp_id == waypoint_buffer.count()-1 && wpa->type == 0)) {
            //all waypoints sent and ack received
            emit _stopProtocolTimer();  // Stop timer on our thread
            current_state = WP_IDLE;
            readWaypoints(false); //Update "Onboard Waypoints"-tab immediately after the waypoint list has been sent.
            QTime time = QTime::currentTime();
            emit updateStatusString(tr("Done. (updated at %1)").arg(time.toString()));
        } else if((current_state == WP_SENDLIST || current_state == WP_SENDLIST_SENDWPS) && wpa->type != 0) {
            //give up transmitting if a WP is rejected
            switch (wpa->type)
            {
            case MAV_MISSION_UNSUPPORTED_FRAME:
                emit updateStatusString(tr("ERROR: Coordinate frame unsupported."));
                break;
            case MAV_MISSION_UNSUPPORTED:
                emit updateStatusString(tr("ERROR: Unsupported command."));
                break;
            case MAV_MISSION_NO_SPACE:
                emit updateStatusString(tr("ERROR: Mission count exceeds storage."));
                break;
            case MAV_MISSION_INVALID:
            case MAV_MISSION_INVALID_PARAM1:
            case MAV_MISSION_INVALID_PARAM2:
            case MAV_MISSION_INVALID_PARAM3:
            case MAV_MISSION_INVALID_PARAM4:
            case MAV_MISSION_INVALID_PARAM5_X:
            case MAV_MISSION_INVALID_PARAM6_Y:
            case MAV_MISSION_INVALID_PARAM7:
                emit updateStatusString(tr("ERROR: A specified parameter was invalid."));
                break;
            case MAV_MISSION_INVALID_SEQUENCE:
                emit updateStatusString(tr("ERROR: Mission received out of sequence."));
                break;
            case MAV_MISSION_DENIED:
                emit updateStatusString(tr("ERROR: UAS not accepting missions."));
                break;
            case MAV_MISSION_ERROR:
            default:
                emit updateStatusString(tr("ERROR: Unspecified error"));
                break;
            }
            emit _stopProtocolTimer();  // Stop timer on our thread
            current_state = WP_IDLE;
        } else if(current_state == WP_CLEARLIST) {
            emit _stopProtocolTimer(); // Stop timer on our thread
            current_state = WP_IDLE;
            QTime time = QTime::currentTime();
            emit updateStatusString(tr("Done. (updated at %1)").arg(time.toString()));
        }
    }
}

void UASWaypointManager::handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_mission_request_t *wpr)
{
    if (systemId == current_partner_systemid && ((current_state == WP_SENDLIST && wpr->seq == 0) || (current_state == WP_SENDLIST_SENDWPS && (wpr->seq == current_wp_id || wpr->seq == current_wp_id + 1)))) {
        emit _startProtocolTimer();  // Start timer on our thread
        current_retries = PROTOCOL_MAX_RETRIES;

        if (wpr->seq < waypoint_buffer.count()) {
            current_state = WP_SENDLIST_SENDWPS;
            current_wp_id = wpr->seq;
            sendWaypoint(current_wp_id);
        } else {
            //TODO: Error message or something
        }
    } else {
        qDebug("Rejecting waypoint request message, check mismatch: current_state: %d == %d, system id %d == %d, comp id %d == %d", current_state, WP_SENDLIST_SENDWPS, current_partner_systemid, systemId, current_partner_compid, compId);
    }
}

void UASWaypointManager::handleWaypointReached(quint8 systemId, quint8 compId, mavlink_mission_item_reached_t *wpr)
{
	Q_UNUSED(compId);
    if (!uas) return;
    if (systemId == uasid) {
        emit updateStatusString(tr("Reached waypoint %1").arg(wpr->seq));
    }
}

void UASWaypointManager::handleWaypointCurrent(quint8 systemId, quint8 compId, mavlink_mission_current_t *wpc)
{
    Q_UNUSED(compId);
    if (!uas) return;
    if (systemId == uasid) {
        // FIXME Petri
        if (current_state == WP_SETCURRENT) {
            emit _stopProtocolTimer();  // Stop timer on our thread
            current_state = WP_IDLE;

            // update the local main storage
            if (wpc->seq < waypointsViewOnly.size()) {
                for(int i = 0; i < waypointsViewOnly.size(); i++) {
                    if (waypointsViewOnly[i]->getId() == wpc->seq) {
                        waypointsViewOnly[i]->setCurrent(true);
                    } else {
                        waypointsViewOnly[i]->setCurrent(false);
                    }
                }
            }
        }
        emit updateStatusString(tr("New current waypoint %1").arg(wpc->seq));
        //emit update to UI widgets
        emit currentWaypointChanged(wpc->seq);
    }
}

void UASWaypointManager::notifyOfChangeEditable(Waypoint* wp)
{
    // If only one waypoint was changed, emit only WP signal
    if (wp != NULL) {
        emit waypointEditableChanged(uasid, wp);
    } else {
        emit waypointEditableListChanged();
        emit waypointEditableListChanged(uasid);
    }
}

void UASWaypointManager::notifyOfChangeViewOnly(Waypoint* wp)
{
    if (wp != NULL) {
        emit waypointViewOnlyChanged(uasid, wp);
    } else {
        emit waypointViewOnlyListChanged();
        emit waypointViewOnlyListChanged(uasid);
    }
}


int UASWaypointManager::setCurrentWaypoint(quint16 seq)
{
    if (seq < waypointsViewOnly.size()) {
        if(current_state == WP_IDLE) {

            //send change to UAS - important to note: if the transmission fails, we have inconsistencies
            emit _startProtocolTimer();  // Start timer on our thread
            current_retries = PROTOCOL_MAX_RETRIES;

            current_state = WP_SETCURRENT;
            current_wp_id = seq;
            current_partner_systemid = uasid;
            current_partner_compid = MAV_COMP_ID_MISSIONPLANNER;

            sendWaypointSetCurrent(current_wp_id);

            return 0;
        }
    }
    return -1;
}

int UASWaypointManager::setCurrentEditable(quint16 seq)
{
    if (seq < waypointsEditable.count()) {
        if(current_state == WP_IDLE) {
            //update local main storage
            for (int i = 0; i < waypointsEditable.count(); i++) {
                if (waypointsEditable[i]->getId() == seq) {
                    waypointsEditable[i]->setCurrent(true);
                } else {
                    waypointsEditable[i]->setCurrent(false);
                }
            }

            return 0;
        }
    }
    return -1;
}

void UASWaypointManager::addWaypointViewOnly(Waypoint *wp)
{
    if (wp)
    {
        waypointsViewOnly.insert(waypointsViewOnly.size(), wp);
        connect(wp, SIGNAL(changed(Waypoint*)), this, SLOT(notifyOfChangeViewOnly(Waypoint*)));

        emit waypointViewOnlyListChanged();
        emit waypointViewOnlyListChanged(uasid);
    }
}


/**
 * @warning Make sure the waypoint stays valid for the whole application lifecycle!
 * @param enforceFirstActive Enforces that the first waypoint is set as active
 * @see createWaypoint() is more suitable for most use cases
 */
void UASWaypointManager::addWaypointEditable(Waypoint *wp, bool enforceFirstActive)
{
    if (wp)
    {
        // Check if this is the first waypoint in an offline list
        if (waypointsEditable.count() == 0 && uas == NULL) {
            QGCMessageBox::critical(tr("Waypoint Manager"),  _offlineEditingModeMessage);
        }

        wp->setId(waypointsEditable.count());
        if (enforceFirstActive && waypointsEditable.count() == 0)
        {
            wp->setCurrent(true);
            currentWaypointEditable = wp;
        }
        waypointsEditable.insert(waypointsEditable.count(), wp);
        connect(wp, SIGNAL(changed(Waypoint*)), this, SLOT(notifyOfChangeEditable(Waypoint*)));

        emit waypointEditableListChanged();
        emit waypointEditableListChanged(uasid);
    }
}

/**
 * @param enforceFirstActive Enforces that the first waypoint is set as active
 */
Waypoint* UASWaypointManager::createWaypoint(bool enforceFirstActive)
{
    // Check if this is the first waypoint in an offline list
    if (waypointsEditable.count() == 0 && uas == NULL) {
        QGCMessageBox::critical(tr("Waypoint Manager"),  _offlineEditingModeMessage);
    }

    Waypoint* wp = new Waypoint();
    wp->setId(waypointsEditable.count());
    wp->setFrame((MAV_FRAME)getFrameRecommendation());
    wp->setAltitude(getAltitudeRecommendation());
    wp->setAcceptanceRadius(getAcceptanceRadiusRecommendation());
    if (enforceFirstActive && waypointsEditable.count() == 0)
    {
        wp->setCurrent(true);
        currentWaypointEditable = wp;
    }
    waypointsEditable.append(wp);
    connect(wp, SIGNAL(changed(Waypoint*)), this, SLOT(notifyOfChangeEditable(Waypoint*)));

    emit waypointEditableListChanged();
    emit waypointEditableListChanged(uasid);
    return wp;
}

int UASWaypointManager::removeWaypoint(quint16 seq)
{
    if (seq < waypointsEditable.count())
    {
        Waypoint *t = waypointsEditable[seq];

        if (t->getCurrent() == true) //trying to remove the current waypoint
        {
            if (seq+1 < waypointsEditable.count()) // setting the next waypoint as current
            {
                waypointsEditable[seq+1]->setCurrent(true);
            }
            else if (seq-1 >= 0) // if deleting the last on the list, then setting the previous waypoint as current
            {
                waypointsEditable[seq-1]->setCurrent(true);
            }
        }

        waypointsEditable.removeAt(seq);
        delete t;
        t = NULL;

        for(int i = seq; i < waypointsEditable.count(); i++)
        {
            waypointsEditable[i]->setId(i);
        }

        emit waypointEditableListChanged();
        emit waypointEditableListChanged(uasid);
        return 0;
    }
    return -1;
}

void UASWaypointManager::moveWaypoint(quint16 cur_seq, quint16 new_seq)
{
    if (cur_seq != new_seq && cur_seq < waypointsEditable.count() && new_seq < waypointsEditable.count())
    {
        Waypoint *t = waypointsEditable[cur_seq];
        if (cur_seq < new_seq) {
            for (int i = cur_seq; i < new_seq; i++)
            {
                waypointsEditable[i] = waypointsEditable[i+1];
                waypointsEditable[i]->setId(i);
            }
        }
        else
        {
            for (int i = cur_seq; i > new_seq; i--)
            {
                waypointsEditable[i] = waypointsEditable[i-1];
                waypointsEditable[i]->setId(i);
            }
        }
        waypointsEditable[new_seq] = t;
        waypointsEditable[new_seq]->setId(new_seq);

        emit waypointEditableListChanged();
        emit waypointEditableListChanged(uasid);
    }
}

void UASWaypointManager::saveWaypoints(const QString &saveFile)
{
    QFile file(saveFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    //write the waypoint list version to the first line for compatibility check
    out << "QGC WPL 120\r\n";

    for (int i = 0; i < waypointsEditable.count(); i++)
    {
        waypointsEditable[i]->setId(i);
        waypointsEditable[i]->save(out);
    }
    file.close();
}

void UASWaypointManager::loadWaypoints(const QString &loadFile)
{
    QFile file(loadFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while(waypointsEditable.count()>0) {
        Waypoint *t = waypointsEditable[0];
        waypointsEditable.removeAt(0);
        delete t;
    }

    QTextStream in(&file);

    const QStringList &version = in.readLine().split(" ");

    if (!(version.size() == 3 && version[0] == "QGC" && version[1] == "WPL" && version[2] == "120"))
    {
        emit updateStatusString(tr("The waypoint file is not compatible with the current version of QGroundControl."));
    }
    else
    {
        while (!in.atEnd())
        {
            Waypoint *t = new Waypoint();
            if(t->load(in))
            {
              //Use the existing function to add waypoints to the map instead of doing it manually
              //Indeed, we should connect our waypoints to the map in order to synchronize them
              //t->setId(waypointsEditable.count());
              // waypointsEditable.insert(waypointsEditable.count(), t);
              addWaypointEditable(t, false);
            }
            else
            {
                emit updateStatusString(tr("The waypoint file is corrupted. Load operation only partly succesful."));
                break;
            }
        }
    }

    file.close();

    emit loadWPFile();
    emit waypointEditableListChanged();
    emit waypointEditableListChanged(uasid);
}

void UASWaypointManager::clearWaypointList()
{
    if (current_state == WP_IDLE)
    {
        emit _startProtocolTimer(); // Start timer on our thread
        current_retries = PROTOCOL_MAX_RETRIES;

        current_state = WP_CLEARLIST;
        current_wp_id = 0;
        current_partner_systemid = uasid;
        current_partner_compid = MAV_COMP_ID_MISSIONPLANNER;

        sendWaypointClearAll();
    }
}

const QList<Waypoint *> UASWaypointManager::getGlobalFrameWaypointList()
{
    // TODO Keep this global frame list up to date
    // with complete waypoint list
    // instead of filtering on each request
    QList<Waypoint*> wps;
    foreach (Waypoint* wp, waypointsEditable)
    {
        if (wp->getFrame() == MAV_FRAME_GLOBAL || wp->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT)
        {
            wps.append(wp);
        }
    }
    return wps;
}

const QList<Waypoint *> UASWaypointManager::getGlobalFrameAndNavTypeWaypointList()
{
    // TODO Keep this global frame list up to date
    // with complete waypoint list
    // instead of filtering on each request
    QList<Waypoint*> wps;
    foreach (Waypoint* wp, waypointsEditable)
    {
        if ((wp->getFrame() == MAV_FRAME_GLOBAL || wp->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) && wp->isNavigationType())
        {
            wps.append(wp);
        }
    }
    return wps;
}

const QList<Waypoint *> UASWaypointManager::getNavTypeWaypointList()
{
    // TODO Keep this global frame list up to date
    // with complete waypoint list
    // instead of filtering on each request
    QList<Waypoint*> wps;
    foreach (Waypoint* wp, waypointsEditable)
    {
        if (wp->isNavigationType())
        {
            wps.append(wp);
        }
    }
    return wps;
}

int UASWaypointManager::getIndexOf(Waypoint* wp)
{
    return waypointsEditable.indexOf(wp);
}

int UASWaypointManager::getGlobalFrameIndexOf(Waypoint* wp)
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable) {
        if (p->getFrame() == MAV_FRAME_GLOBAL || wp->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT)
        {
            if (p == wp)
            {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getGlobalFrameAndNavTypeIndexOf(Waypoint* wp)
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable) {
        if ((p->getFrame() == MAV_FRAME_GLOBAL || wp->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) && p->isNavigationType())
        {
            if (p == wp)
            {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getNavTypeIndexOf(Waypoint* wp)
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable)
    {
        if (p->isNavigationType())
        {
            if (p == wp)
            {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getGlobalFrameCount()
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable)
    {
        if (p->getFrame() == MAV_FRAME_GLOBAL || p->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT)
        {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getGlobalFrameAndNavTypeCount()
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable) {
        if ((p->getFrame() == MAV_FRAME_GLOBAL || p->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) && p->isNavigationType())
        {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getNavTypeCount()
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable) {
        if (p->isNavigationType()) {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getLocalFrameCount()
{
    // Search through all waypointsEditable,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable)
    {
        if (p->getFrame() == MAV_FRAME_LOCAL_NED || p->getFrame() == MAV_FRAME_LOCAL_ENU)
        {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getLocalFrameIndexOf(Waypoint* wp)
{
    // Search through all waypointsEditable,
    // counting only those in local frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable)
    {
        if (p->getFrame() == MAV_FRAME_LOCAL_NED || p->getFrame() == MAV_FRAME_LOCAL_ENU)
        {
            if (p == wp)
            {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getMissionFrameIndexOf(Waypoint* wp)
{
    // Search through all waypointsEditable,
    // counting only those in mission frame
    int i = 0;
    foreach (Waypoint* p, waypointsEditable)
    {
        if (p->getFrame() == MAV_FRAME_MISSION)
        {
            if (p == wp)
            {
                return i;
            }
            i++;
        }
    }

    return -1;
}


/**
 * @param readToEdit If true, incoming waypoints will be copied both to "edit"-tab and "view"-tab. Otherwise, only to "view"-tab.
 */
void UASWaypointManager::readWaypoints(bool readToEdit)
{
    read_to_edit = readToEdit;
    emit readGlobalWPFromUAS(true);
    if(current_state == WP_IDLE) {


        //Clear the old view-list before receiving the new one
        while(waypointsViewOnly.size()>0) {
            Waypoint *t = waypointsViewOnly[0];
            waypointsViewOnly.removeAt(0);
            delete t;
        }
        emit waypointViewOnlyListChanged();
        /* THIS PART WAS MOVED TO handleWaypointCount. THE EDIT-LIST SHOULD NOT BE CLEARED UNLESS THERE IS A RESPONSE FROM UAV.
        //Clear the old edit-list before receiving the new one
        if (read_to_edit == true){
            while(waypointsEditable.count()>0) {
                Waypoint *t = waypointsEditable[0];
                waypointsEditable.remove(0);
                delete t;
            }
            emit waypointEditableListChanged();
        }
        */
        
        // We are signalling ourselves here so that the timer gets started on the right thread
        emit _startProtocolTimer();

        current_retries = PROTOCOL_MAX_RETRIES;

        current_state = WP_GETLIST;
        current_wp_id = 0;
        current_partner_systemid = uasid;
        current_partner_compid = MAV_COMP_ID_MISSIONPLANNER;

        sendWaypointRequestList();

    }
}
bool UASWaypointManager::guidedModeSupported()
{
    return (uas->getAutopilotType() == MAV_AUTOPILOT_ARDUPILOTMEGA);
}

void UASWaypointManager::goToWaypoint(Waypoint *wp)
{
    //Don't try to send a guided mode message to an AP that does not support it.
    if (uas->getAutopilotType() == MAV_AUTOPILOT_ARDUPILOTMEGA)
    {
        mavlink_mission_item_t mission;
        memset(&mission, 0, sizeof(mavlink_mission_item_t));   //initialize with zeros
        //const Waypoint *cur_s = waypointsEditable.at(i);

        mission.autocontinue = 0;
        mission.current = 2; //2 for guided mode
        mission.param1 = wp->getParam1();
        mission.param2 = wp->getParam2();
        mission.param3 = wp->getParam3();
        mission.param4 = wp->getParam4();
        mission.frame = wp->getFrame();
        mission.command = wp->getAction();
        mission.seq = 0;     // don't read out the sequence number of the waypoint class
        mission.x = wp->getX();
        mission.y = wp->getY();
        mission.z = wp->getZ();
        mavlink_message_t message;
        mission.target_system = uasid;
        mission.target_component = MAV_COMP_ID_MISSIONPLANNER;
        mavlink_msg_mission_item_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &mission);
        uas->sendMessage(message);
        QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
    }
}

void UASWaypointManager::writeWaypoints()
{
    if (current_state == WP_IDLE) {
        // Send clear all if count == 0
        if (waypointsEditable.count() > 0) {
            emit _startProtocolTimer();  // Start timer on our thread
            current_retries = PROTOCOL_MAX_RETRIES;

            current_count = waypointsEditable.count();
            current_state = WP_SENDLIST;
            current_wp_id = 0;
            current_partner_systemid = uasid;
            current_partner_compid = MAV_COMP_ID_MISSIONPLANNER;

            //clear local buffer
            // Why not replace with waypoint_buffer.clear() ?
            // because this will lead to memory leaks, the waypoint-structs
            // have to be deleted, clear() would only delete the pointers.
            while(!waypoint_buffer.empty()) {
                delete waypoint_buffer.back();
                waypoint_buffer.pop_back();
            }

            bool noCurrent = true;

            //copy waypoint data to local buffer
            for (int i=0; i < current_count; i++) {
                waypoint_buffer.push_back(new mavlink_mission_item_t);
                mavlink_mission_item_t *cur_d = waypoint_buffer.back();
                memset(cur_d, 0, sizeof(mavlink_mission_item_t));   //initialize with zeros
                const Waypoint *cur_s = waypointsEditable.at(i);

                cur_d->autocontinue = cur_s->getAutoContinue();
                cur_d->current = cur_s->getCurrent() & noCurrent;   //make sure only one current waypoint is selected, the first selected will be chosen
                cur_d->param1 = cur_s->getParam1();
                cur_d->param2 = cur_s->getParam2();
                cur_d->param3 = cur_s->getParam3();
                cur_d->param4 = cur_s->getParam4();
                cur_d->frame = cur_s->getFrame();
                cur_d->command = cur_s->getAction();
                cur_d->seq = i;     // don't read out the sequence number of the waypoint class
                cur_d->x = cur_s->getX();
                cur_d->y = cur_s->getY();
                cur_d->z = cur_s->getZ();

                if (cur_s->getCurrent() && noCurrent)
                    noCurrent = false;
                if (i == (current_count - 1) && noCurrent == true) //not a single waypoint was set as "current"
                    cur_d->current = true; // set the last waypoint as current. Or should it better be the first waypoint ?
            }




            //send the waypoint count to UAS (this starts the send transaction)
            sendWaypointCount();
        } else if (waypointsEditable.count() == 0)
        {
            clearWaypointList();
        }
    }
    else
    {
        // We're in another transaction, ignore command
        qDebug() << tr("UASWaypointManager::sendWaypoints() doing something else. Ignoring command");
    }
}

void UASWaypointManager::sendWaypointClearAll()
{
    if (!uas) return;

    // Send the message.
    mavlink_message_t message;
    mavlink_mission_clear_all_t wpca = {(quint8)uasid, MAV_COMP_ID_MISSIONPLANNER};
    mavlink_msg_mission_clear_all_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &wpca);
    uas->sendMessage(message);

    // And update the UI.
    emit updateStatusString(tr("Clearing waypoint list..."));

    QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
}

void UASWaypointManager::sendWaypointSetCurrent(quint16 seq)
{
    if (!uas) return;

    // Send the message.
    mavlink_message_t message;
    mavlink_mission_set_current_t wpsc = {seq, (quint8)uasid, MAV_COMP_ID_MISSIONPLANNER};
    mavlink_msg_mission_set_current_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &wpsc);
    uas->sendMessage(message);

    // And update the UI.
    emit updateStatusString(tr("Updating target waypoint..."));

    QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
}

void UASWaypointManager::sendWaypointCount()
{
    if (!uas) return;


    // Tell the UAS how many missions we'll sending.
    mavlink_message_t message;
    mavlink_mission_count_t wpc = {current_count, (quint8)uasid, MAV_COMP_ID_MISSIONPLANNER};
    mavlink_msg_mission_count_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &wpc);
    uas->sendMessage(message);

    // And update the UI.
    emit updateStatusString(tr("Starting to transmit waypoints..."));

    QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
}

void UASWaypointManager::sendWaypointRequestList()
{
    if (!uas) return;

    // Send a MISSION_REQUEST message to the uas for this mission manager, using the MISSIONPLANNER component.
    mavlink_message_t message;
    mavlink_mission_request_list_t wprl = {(quint8)uasid, MAV_COMP_ID_MISSIONPLANNER};
    mavlink_msg_mission_request_list_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &wprl);
    uas->sendMessage(message);

    // And update the UI.
    QString statusMsg(tr("Requesting waypoint list..."));
    qDebug() << __FILE__ << __LINE__ << statusMsg;
    emit updateStatusString(statusMsg);

    QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
}

void UASWaypointManager::sendWaypointRequest(quint16 seq)
{
    if (!uas) return;

    // Send a MISSION_REQUEST message to the UAS's MISSIONPLANNER component.
    mavlink_message_t message;
    mavlink_mission_request_t wpr = {seq, (quint8)uasid, MAV_COMP_ID_MISSIONPLANNER};
    mavlink_msg_mission_request_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &wpr);
    uas->sendMessage(message);

    // And update the UI.
    emit updateStatusString(tr("Retrieving waypoint ID %1 of %2").arg(wpr.seq).arg(current_count));

    QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
}

void UASWaypointManager::sendWaypoint(quint16 seq)
{
    if (!uas) return;
    mavlink_message_t message;

    if (seq < waypoint_buffer.count()) {

        // Fetch the current mission to send, and set it to apply to the curent UAS.
        mavlink_mission_item_t *wp = waypoint_buffer.at(seq);
        wp->target_system = uasid;
        wp->target_component = MAV_COMP_ID_MISSIONPLANNER;

        // Transmit the new mission
        mavlink_msg_mission_item_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, wp);
        uas->sendMessage(message);

        // And update the UI.
        emit updateStatusString(tr("Sending waypoint ID %1 of %2 total").arg(wp->seq).arg(current_count));

        QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
    }
}

void UASWaypointManager::sendWaypointAck(quint8 type)
{
    if (!uas) return;

    // Send the message.
    mavlink_message_t message;
    mavlink_mission_ack_t wpa = {(quint8)uasid, MAV_COMP_ID_MISSIONPLANNER, type};
    mavlink_msg_mission_ack_encode(uas->mavlink->getSystemId(), uas->mavlink->getComponentId(), &message, &wpa);
    uas->sendMessage(message);

    QGC::SLEEP::msleep(PROTOCOL_DELAY_MS);
}

UAS* UASWaypointManager::getUAS() {
    return this->uas;    ///< Returns the owning UAS
}

float UASWaypointManager::getAltitudeRecommendation()
{
    if (waypointsEditable.count() > 0) {
        return waypointsEditable.last()->getAltitude();
    } else {
        return UASManager::instance()->getHomeAltitude() + getHomeAltitudeOffsetDefault();
    }
}

int UASWaypointManager::getFrameRecommendation()
{
    if (waypointsEditable.count() > 0) {
        return static_cast<int>(waypointsEditable.last()->getFrame());
    } else {
        return MAV_FRAME_GLOBAL;
    }
}

float UASWaypointManager::getAcceptanceRadiusRecommendation()
{
    if (waypointsEditable.count() > 0)
    {
        return waypointsEditable.last()->getAcceptanceRadius();
    }
    else
    {
        // Default to rotary wing waypoint radius for offline editing
        if (!uas || uas->isRotaryWing())
        {
            return UASInterface::WAYPOINT_RADIUS_DEFAULT_ROTARY_WING;
        }
        else if (uas->isFixedWing())
        {
            return UASInterface::WAYPOINT_RADIUS_DEFAULT_FIXED_WING;
        }
    }

    return 10.0f;
}

float UASWaypointManager::getHomeAltitudeOffsetDefault()
{
    return defaultAltitudeHomeOffset;
}


void UASWaypointManager::_startProtocolTimerOnThisThread(void)
{
    protocol_timer.start(PROTOCOL_TIMEOUT_MS);
}

void UASWaypointManager::_stopProtocolTimerOnThisThread(void)
{
    protocol_timer.stop();
}


void UASWaypointManager::_updateWPonTimer()
{
    while (current_state != WP_IDLE)
    {
        QGC::SLEEP::msleep(100);
    }
    readWaypoints(true);
}
