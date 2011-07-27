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
 *   @brief Implementation of the waypoint protocol handler
 *
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include "UASWaypointManager.h"
#include "UAS.h"
#include "mavlink_types.h"
//#include "MainWindow.h"

#define PROTOCOL_TIMEOUT_MS 2000    ///< maximum time to wait for pending messages until timeout
#define PROTOCOL_DELAY_MS 40        ///< minimum delay between sent messages
#define PROTOCOL_MAX_RETRIES 3      ///< maximum number of send retries (after timeout)

UASWaypointManager::UASWaypointManager(UAS &_uas)
    : uas(_uas),
      current_retries(0),
      current_wp_id(0),
      current_count(0),
      current_state(WP_IDLE),
      current_partner_systemid(0),
      current_partner_compid(0),
      protocol_timer(this)
{
    connect(&protocol_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

void UASWaypointManager::timeout()
{
    if (current_retries > 0) {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries--;
        emit updateStatusString(tr("Timeout, retrying (retries left: %1)").arg(current_retries));
        // qDebug() << "Timeout, retrying (retries left:" << current_retries << ")";
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

        // qDebug() << "Waypoint transaction (state=" << current_state << ") timed out going to state WP_IDLE";

        emit updateStatusString("Operation timed out.");

        current_state = WP_IDLE;
        current_count = 0;
        current_wp_id = 0;
        current_partner_systemid = 0;
        current_partner_compid = 0;
    }
}

void UASWaypointManager::handleWaypointCount(quint8 systemId, quint8 compId, quint16 count)
{
    if (current_state == WP_GETLIST && systemId == current_partner_systemid && compId == current_partner_compid) {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries = PROTOCOL_MAX_RETRIES;

        // qDebug() << "got waypoint count (" << count << ") from ID " << systemId;

        if (count > 0) {
            current_count = count;
            current_wp_id = 0;
            current_state = WP_GETLIST_GETWPS;

            sendWaypointRequest(current_wp_id);
        } else {
            protocol_timer.stop();
            emit updateStatusString("done.");
            // qDebug() << "No waypoints on UAS " << systemId;
            current_state = WP_IDLE;
            current_count = 0;
            current_wp_id = 0;
            current_partner_systemid = 0;
            current_partner_compid = 0;
        }
    } else {
        qDebug("Rejecting message, check mismatch: current_state: %d == %d, system id %d == %d, comp id %d == %d", current_state, WP_GETLIST, current_partner_systemid, systemId, current_partner_compid, compId);
    }
}

void UASWaypointManager::handleWaypoint(quint8 systemId, quint8 compId, mavlink_waypoint_t *wp)
{
    if (systemId == current_partner_systemid && compId == current_partner_compid && current_state == WP_GETLIST_GETWPS && wp->seq == current_wp_id) {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries = PROTOCOL_MAX_RETRIES;

        if(wp->seq == current_wp_id) {
            //// qDebug() << "Got WP: " << wp->seq << wp->x <<  wp->y << wp->z << wp->param4 << "auto:" << wp->autocontinue << "curr:" << wp->current << wp->param1 << wp->param2 << "Frame:"<< (MAV_FRAME) wp->frame << "Command:" << (MAV_CMD) wp->command;
            Waypoint *lwp = new Waypoint(wp->seq, wp->x, wp->y, wp->z, wp->param1, wp->param2, wp->param3, wp->param4, wp->autocontinue, wp->current, (MAV_FRAME) wp->frame, (MAV_CMD) wp->command);
            addWaypoint(lwp, false);

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
                current_partner_systemid = 0;
                current_partner_compid = 0;

                protocol_timer.stop();
                emit readGlobalWPFromUAS(false);
                emit updateStatusString("done.");

                // qDebug() << "got all waypoints from ID " << systemId;
            }
        } else {
            emit updateStatusString(tr("Waypoint ID mismatch, rejecting waypoint"));
        }
    } else {
        qDebug("Rejecting message, check mismatch: current_state: %d == %d, system id %d == %d, comp id %d == %d", current_state, WP_GETLIST, current_partner_systemid, systemId, current_partner_compid, compId);
    }
}

void UASWaypointManager::handleWaypointAck(quint8 systemId, quint8 compId, mavlink_waypoint_ack_t *wpa)
{
    if (systemId == current_partner_systemid && compId == current_partner_compid) {
        if((current_state == WP_SENDLIST || current_state == WP_SENDLIST_SENDWPS) && (current_wp_id == waypoint_buffer.count()-1 && wpa->type == 0)) {
            //all waypoints sent and ack received
            protocol_timer.stop();
            current_state = WP_IDLE;
            emit updateStatusString("done.");
            // qDebug() << "sent all waypoints to ID " << systemId;
        } else if(current_state == WP_CLEARLIST) {
            protocol_timer.stop();
            current_state = WP_IDLE;
            emit updateStatusString("done.");
            // qDebug() << "cleared waypoint list of ID " << systemId;
        }
    }
}

void UASWaypointManager::handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_waypoint_request_t *wpr)
{
    if (systemId == current_partner_systemid && compId == current_partner_compid && ((current_state == WP_SENDLIST && wpr->seq == 0) || (current_state == WP_SENDLIST_SENDWPS && (wpr->seq == current_wp_id || wpr->seq == current_wp_id + 1)))) {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries = PROTOCOL_MAX_RETRIES;

        if (wpr->seq < waypoint_buffer.count()) {
            current_state = WP_SENDLIST_SENDWPS;
            current_wp_id = wpr->seq;
            sendWaypoint(current_wp_id);
        } else {
            //TODO: Error message or something
        }
    } else {
        qDebug("Rejecting message, check mismatch: current_state: %d == %d, system id %d == %d, comp id %d == %d", current_state, WP_GETLIST, current_partner_systemid, systemId, current_partner_compid, compId);
    }
}

void UASWaypointManager::handleWaypointReached(quint8 systemId, quint8 compId, mavlink_waypoint_reached_t *wpr)
{
    if (systemId == uas.getUASID() && compId == MAV_COMP_ID_WAYPOINTPLANNER) {
        emit updateStatusString(QString("Reached waypoint %1").arg(wpr->seq));
    }
}

void UASWaypointManager::handleWaypointCurrent(quint8 systemId, quint8 compId, mavlink_waypoint_current_t *wpc)
{
    if (systemId == uas.getUASID() && compId == MAV_COMP_ID_WAYPOINTPLANNER) {
        // FIXME Petri
        if (current_state == WP_SETCURRENT) {
            protocol_timer.stop();
            current_state = WP_IDLE;

            // update the local main storage
            if (wpc->seq < waypoints.size()) {
                for(int i = 0; i < waypoints.size(); i++) {
                    if (waypoints[i]->getId() == wpc->seq) {
                        waypoints[i]->setCurrent(true);
                    } else {
                        waypoints[i]->setCurrent(false);
                    }
                }
            }

            //// qDebug() << "Updated waypoints list";
        }
        emit updateStatusString(QString("New current waypoint %1").arg(wpc->seq));
        //emit update to UI widgets
        emit currentWaypointChanged(wpc->seq);
        //// qDebug() << "new current waypoint" << wpc->seq;
    }
}

void UASWaypointManager::notifyOfChange(Waypoint* wp)
{
    // qDebug() << "WAYPOINT CHANGED: ID:" << wp->getId();
    // If only one waypoint was changed, emit only WP signal
    if (wp != NULL) {
        emit waypointChanged(uas.getUASID(), wp);
    } else {
        emit waypointListChanged();
        emit waypointListChanged(uas.getUASID());
    }
}

int UASWaypointManager::setCurrentWaypoint(quint16 seq)
{
    if (seq < waypoints.size()) {
        if(current_state == WP_IDLE) {
            //update local main storage
            for(int i = 0; i < waypoints.size(); i++) {
                if (waypoints[i]->getId() == seq) {
                    waypoints[i]->setCurrent(true);
                } else {
                    waypoints[i]->setCurrent(false);
                }
            }

            //TODO: signal changed waypoint list

            //send change to UAS - important to note: if the transmission fails, we have inconsistencies
            protocol_timer.start(PROTOCOL_TIMEOUT_MS);
            current_retries = PROTOCOL_MAX_RETRIES;

            current_state = WP_SETCURRENT;
            current_wp_id = seq;
            current_partner_systemid = uas.getUASID();
            current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

            sendWaypointSetCurrent(current_wp_id);

            //emit waypointListChanged();

            return 0;
        }
    }
    return -1;
}

/**
 * @warning Make sure the waypoint stays valid for the whole application lifecycle!
 * @param enforceFirstActive Enforces that the first waypoint is set as active
 * @see createWaypoint() is more suitable for most use cases
 */
void UASWaypointManager::addWaypoint(Waypoint *wp, bool enforceFirstActive)
{
    if (wp) {
        wp->setId(waypoints.size());
        if (enforceFirstActive && waypoints.size() == 0) wp->setCurrent(true);
        waypoints.insert(waypoints.size(), wp);
        connect(wp, SIGNAL(changed(Waypoint*)), this, SLOT(notifyOfChange(Waypoint*)));

        emit waypointListChanged();
        emit waypointListChanged(uas.getUASID());
    }
}

/**
 * @param enforceFirstActive Enforces that the first waypoint is set as active
 */
Waypoint* UASWaypointManager::createWaypoint(bool enforceFirstActive)
{
    Waypoint* wp = new Waypoint();
    wp->setId(waypoints.size());
    if (enforceFirstActive && waypoints.size() == 0) wp->setCurrent(true);
    waypoints.insert(waypoints.size(), wp);
    connect(wp, SIGNAL(changed(Waypoint*)), this, SLOT(notifyOfChange(Waypoint*)));

    emit waypointListChanged();
    emit waypointListChanged(uas.getUASID());
    return wp;
}

int UASWaypointManager::removeWaypoint(quint16 seq)
{
    if (seq < waypoints.size()) {
        Waypoint *t = waypoints[seq];
        waypoints.remove(seq);
        delete t;
        t = NULL;

        for(int i = seq; i < waypoints.size(); i++) {
            waypoints[i]->setId(i);
        }

        emit waypointListChanged();
        emit waypointListChanged(uas.getUASID());
        return 0;
    }
    return -1;
}

void UASWaypointManager::moveWaypoint(quint16 cur_seq, quint16 new_seq)
{
    if (cur_seq != new_seq && cur_seq < waypoints.size() && new_seq < waypoints.size()) {
        Waypoint *t = waypoints[cur_seq];
        if (cur_seq < new_seq) {
            for (int i = cur_seq; i < new_seq; i++) {
                waypoints[i] = waypoints[i+1];
                //waypoints[i]->setId(i);       // let the local IDs stay the same so that they can be found more easily by the user
            }
        } else {
            for (int i = cur_seq; i > new_seq; i--) {
                waypoints[i] = waypoints[i-1];
                // waypoints[i]->setId(i);
            }
        }
        waypoints[new_seq] = t;
        //waypoints[new_seq]->setId(new_seq);

        emit waypointListChanged();
        emit waypointListChanged(uas.getUASID());
    }
}

void UASWaypointManager::saveWaypoints(const QString &saveFile)
{
    QFile file(saveFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    //write the waypoint list version to the first line for compatibility check
    out << "QGC WPL 110\r\n";

    for (int i = 0; i < waypoints.size(); i++) {
        waypoints[i]->setId(i);
        waypoints[i]->save(out);
    }
    file.close();
}

void UASWaypointManager::loadWaypoints(const QString &loadFile)
{
    QFile file(loadFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while(waypoints.size()>0) {
        Waypoint *t = waypoints[0];
        waypoints.remove(0);
        delete t;
    }

    QTextStream in(&file);

    const QStringList &version = in.readLine().split(" ");

    if (!(version.size() == 3 && version[0] == "QGC" && version[1] == "WPL" && version[2] == "110")) {
        emit updateStatusString(tr("The waypoint file is not compatible with the current version of QGroundControl."));
        //MainWindow::instance()->showCriticalMessage(tr("Error loading waypoint file"),tr("The waypoint file is not compatible with the current version of QGroundControl."));
    } else {
        while (!in.atEnd()) {
            Waypoint *t = new Waypoint();
            if(t->load(in)) {
                t->setId(waypoints.size());
                waypoints.insert(waypoints.size(), t);
            } else {
                emit updateStatusString(tr("The waypoint file is corrupted. Load operation only partly succesful."));
                //MainWindow::instance()->showCriticalMessage(tr("Error loading waypoint file"),);
                break;
            }
        }
    }

    file.close();

    emit loadWPFile();
    emit waypointListChanged();
    emit waypointListChanged(uas.getUASID());
}


//void UASWaypointManager::globalAddWaypoint(Waypoint *wp)
//{
//    // FIXME Will be removed
//    Q_UNUSED(wp);
//}

//int UASWaypointManager::globalRemoveWaypoint(quint16 seq)
//{
//    // FIXME Will be removed
//    Q_UNUSED(seq);
//    return 0;
//}

//void UASWaypointManager::waypointUpdated(Waypoint*)
//{
//    // FIXME Add rate limiter
//    emit waypointListChanged(uas.getUASID());
//}

void UASWaypointManager::clearWaypointList()
{
    if(current_state == WP_IDLE) {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries = PROTOCOL_MAX_RETRIES;

        current_state = WP_CLEARLIST;
        current_wp_id = 0;
        current_partner_systemid = uas.getUASID();
        current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

        sendWaypointClearAll();
    }
}

const QVector<Waypoint *> UASWaypointManager::getGlobalFrameWaypointList()
{
    // TODO Keep this global frame list up to date
    // with complete waypoint list
    // instead of filtering on each request
    QVector<Waypoint*> wps;
    foreach (Waypoint* wp, waypoints) {
        if (wp->getFrame() == MAV_FRAME_GLOBAL) {
            wps.append(wp);
        }
    }
    return wps;
}

const QVector<Waypoint *> UASWaypointManager::getGlobalFrameAndNavTypeWaypointList()
{
    // TODO Keep this global frame list up to date
    // with complete waypoint list
    // instead of filtering on each request
    QVector<Waypoint*> wps;
    foreach (Waypoint* wp, waypoints) {
        if (wp->getFrame() == MAV_FRAME_GLOBAL && wp->isNavigationType()) {
            wps.append(wp);
        }
    }
    return wps;
}

const QVector<Waypoint *> UASWaypointManager::getNavTypeWaypointList()
{
    // TODO Keep this global frame list up to date
    // with complete waypoint list
    // instead of filtering on each request
    QVector<Waypoint*> wps;
    foreach (Waypoint* wp, waypoints) {
        if (wp->isNavigationType()) {
            wps.append(wp);
        }
    }
    return wps;
}

int UASWaypointManager::getIndexOf(Waypoint* wp)
{
    return waypoints.indexOf(wp);
}

int UASWaypointManager::getGlobalFrameIndexOf(Waypoint* wp)
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_GLOBAL) {
            if (p == wp) {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getGlobalFrameAndNavTypeIndexOf(Waypoint* wp)
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_GLOBAL && p->isNavigationType()) {
            if (p == wp) {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getNavTypeIndexOf(Waypoint* wp)
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->isNavigationType()) {
            if (p == wp) {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getGlobalFrameCount()
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_GLOBAL) {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getGlobalFrameAndNavTypeCount()
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_GLOBAL && p->isNavigationType()) {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getNavTypeCount()
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->isNavigationType()) {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getLocalFrameCount()
{
    // Search through all waypoints,
    // counting only those in global frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_GLOBAL) {
            i++;
        }
    }

    return i;
}

int UASWaypointManager::getLocalFrameIndexOf(Waypoint* wp)
{
    // Search through all waypoints,
    // counting only those in local frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_LOCAL) {
            if (p == wp) {
                return i;
            }
            i++;
        }
    }

    return -1;
}

int UASWaypointManager::getMissionFrameIndexOf(Waypoint* wp)
{
    // Search through all waypoints,
    // counting only those in mission frame
    int i = 0;
    foreach (Waypoint* p, waypoints) {
        if (p->getFrame() == MAV_FRAME_MISSION) {
            if (p == wp) {
                return i;
            }
            i++;
        }
    }

    return -1;
}

void UASWaypointManager::readWaypoints()
{
    emit readGlobalWPFromUAS(true);
    if(current_state == WP_IDLE) {
        while(waypoints.size()>0) {
            delete waypoints.back();
            waypoints.pop_back();

        }

        protocol_timer.start(PROTOCOL_TIMEOUT_MS);
        current_retries = PROTOCOL_MAX_RETRIES;

        current_state = WP_GETLIST;
        current_wp_id = 0;
        current_partner_systemid = uas.getUASID();
        current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

        sendWaypointRequestList();

    }
}

void UASWaypointManager::writeWaypoints()
{
    if (current_state == WP_IDLE) {
        // Send clear all if count == 0
        if (waypoints.count() > 0) {
            protocol_timer.start(PROTOCOL_TIMEOUT_MS);
            current_retries = PROTOCOL_MAX_RETRIES;

            current_count = waypoints.count();
            current_state = WP_SENDLIST;
            current_wp_id = 0;
            current_partner_systemid = uas.getUASID();
            current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

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
                waypoint_buffer.push_back(new mavlink_waypoint_t);
                mavlink_waypoint_t *cur_d = waypoint_buffer.back();
                memset(cur_d, 0, sizeof(mavlink_waypoint_t));   //initialize with zeros
                const Waypoint *cur_s = waypoints.at(i);

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
        }
    } else if (waypoints.count() == 0) {
        sendWaypointClearAll();
    } else {
        //we're in another transaction, ignore command
        // qDebug() << "UASWaypointManager::sendWaypoints() doing something else ignoring command";
    }
}

void UASWaypointManager::sendWaypointClearAll()
{
    mavlink_message_t message;
    mavlink_waypoint_clear_all_t wpca;

    wpca.target_system = uas.getUASID();
    wpca.target_component = MAV_COMP_ID_WAYPOINTPLANNER;

    emit updateStatusString(QString("Clearing waypoint list..."));

    mavlink_msg_waypoint_clear_all_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpca);
    uas.sendMessage(message);
    MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    // qDebug() << "sent waypoint clear all to ID " << wpca.target_system;
}

void UASWaypointManager::sendWaypointSetCurrent(quint16 seq)
{
    mavlink_message_t message;
    mavlink_waypoint_set_current_t wpsc;

    wpsc.target_system = uas.getUASID();
    wpsc.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
    wpsc.seq = seq;

    emit updateStatusString(QString("Updating target waypoint..."));

    mavlink_msg_waypoint_set_current_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpsc);
    uas.sendMessage(message);
    MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    // qDebug() << "sent waypoint set current (" << wpsc.seq << ") to ID " << wpsc.target_system;
}

void UASWaypointManager::sendWaypointCount()
{
    mavlink_message_t message;
    mavlink_waypoint_count_t wpc;

    wpc.target_system = uas.getUASID();
    wpc.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
    wpc.count = current_count;

    // qDebug() << "sent waypoint count (" << wpc.count << ") to ID " << wpc.target_system;
    emit updateStatusString(QString("Starting to transmit waypoints..."));

    mavlink_msg_waypoint_count_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpc);
    uas.sendMessage(message);
    MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    // qDebug() << "sent waypoint count (" << wpc.count << ") to ID " << wpc.target_system;
}

void UASWaypointManager::sendWaypointRequestList()
{
    mavlink_message_t message;
    mavlink_waypoint_request_list_t wprl;

    wprl.target_system = uas.getUASID();
    wprl.target_component = MAV_COMP_ID_WAYPOINTPLANNER;

    emit updateStatusString(QString("Requesting waypoint list..."));

    mavlink_msg_waypoint_request_list_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wprl);
    uas.sendMessage(message);
    MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    // qDebug() << "sent waypoint list request to ID " << wprl.target_system;


}

void UASWaypointManager::sendWaypointRequest(quint16 seq)
{
    mavlink_message_t message;
    mavlink_waypoint_request_t wpr;

    wpr.target_system = uas.getUASID();
    wpr.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
    wpr.seq = seq;

    emit updateStatusString(QString("Retrieving waypoint ID %1 of %2 total").arg(wpr.seq).arg(current_count));

    mavlink_msg_waypoint_request_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpr);
    uas.sendMessage(message);
    MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    // qDebug() << "sent waypoint request (" << wpr.seq << ") to ID " << wpr.target_system;
}

void UASWaypointManager::sendWaypoint(quint16 seq)
{
    mavlink_message_t message;
    // qDebug() <<" WP Buffer count: "<<waypoint_buffer.count();

    if (seq < waypoint_buffer.count()) {

        mavlink_waypoint_t *wp;


        wp = waypoint_buffer.at(seq);
        wp->target_system = uas.getUASID();
        wp->target_component = MAV_COMP_ID_WAYPOINTPLANNER;

        emit updateStatusString(QString("Sending waypoint ID %1 of %2 total").arg(wp->seq).arg(current_count));

        // qDebug() << "sent waypoint (" << wp->seq << ") to ID " << wp->target_system<<" WP Buffer count: "<<waypoint_buffer.count();

        mavlink_msg_waypoint_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, wp);
        uas.sendMessage(message);
        MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    }
}

void UASWaypointManager::sendWaypointAck(quint8 type)
{
    mavlink_message_t message;
    mavlink_waypoint_ack_t wpa;

    wpa.target_system = uas.getUASID();
    wpa.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
    wpa.type = type;

    mavlink_msg_waypoint_ack_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpa);
    uas.sendMessage(message);
    MG::SLEEP::usleep(PROTOCOL_DELAY_MS * 1000);

    // qDebug() << "sent waypoint ack (" << wpa.type << ") to ID " << wpa.target_system;
}
