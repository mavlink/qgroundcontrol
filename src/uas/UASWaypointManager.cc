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
 *   @brief Implementation of the waypoint protocol handler
 *
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include "UASWaypointManager.h"
#include "UAS.h"

#define PROTOCOL_TIMEOUT_MS 2000    ///< maximum time to wait for pending messages until timeout
#define PROTOCOL_MAX_RETRIES 3      ///< maximum number of send retries (after timeout)

UASWaypointManager::UASWaypointManager(UAS &_uas)
        : uas(_uas),
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
    protocol_timer.stop();

    qDebug() << "Waypoint transaction (state=" << current_state << ") timed out going to state WP_IDLE";

    emit updateStatusString("Operation timed out.");

    current_state = WP_IDLE;
    current_count = 0;
    current_wp_id = 0;
    current_partner_systemid = 0;
    current_partner_compid = 0;
}

void UASWaypointManager::handleWaypointCount(quint8 systemId, quint8 compId, quint16 count)
{
    protocol_timer.start(PROTOCOL_TIMEOUT_MS);

    if (current_state == WP_GETLIST && systemId == current_partner_systemid && compId == current_partner_compid)
    {
        qDebug() << "got waypoint count (" << count << ") from ID " << systemId;

        if (count > 0)
        {
            current_count = count;
            current_wp_id = 0;
            current_state = WP_GETLIST_GETWPS;

            sendWaypointRequest(current_wp_id);
        }
        else
        {
            emit updateStatusString("done.");
            qDebug() << "No waypoints on UAS " << systemId;
        }
    }
}

void UASWaypointManager::handleWaypoint(quint8 systemId, quint8 compId, mavlink_waypoint_t *wp)
{
    protocol_timer.start(PROTOCOL_TIMEOUT_MS);

    if (systemId == current_partner_systemid && compId == current_partner_compid && current_state == WP_GETLIST_GETWPS && wp->seq == current_wp_id)
    {
        if(wp->seq == current_wp_id)
        {
            //update the UI FIXME
            emit waypointUpdated(wp->seq, wp->x, wp->y, wp->z, wp->yaw, wp->autocontinue, wp->current, wp->param1, wp->param2);

            //get next waypoint
            current_wp_id++;

            if(current_wp_id < current_count)
            {
                sendWaypointRequest(current_wp_id);
            }
            else
            {
                sendWaypointAck(0);

                // all waypoints retrieved, change state to idle
                current_state = WP_IDLE;
                current_count = 0;
                current_wp_id = 0;
                current_partner_systemid = 0;
                current_partner_compid = 0;

                protocol_timer.stop();
                emit updateStatusString("done.");

                qDebug() << "got all waypoints from ID " << systemId;
            }
        }
        else
        {
            //TODO: error handling
        }
    }
}

void UASWaypointManager::handleWaypointAck(quint8 systemId, quint8 compId, mavlink_waypoint_ack_t *wpa)
{
    protocol_timer.start(PROTOCOL_TIMEOUT_MS);

    if (systemId == current_partner_systemid && compId == current_partner_compid && (current_state == WP_SENDLIST || current_state == WP_SENDLIST_SENDWPS))
    {
        if(current_wp_id == waypoint_buffer.count()-1 && wpa->type == 0)
        {
            //all waypoints sent and ack received
            current_state = WP_IDLE;

            protocol_timer.stop();
            emit updateStatusString("done.");

            qDebug() << "sent all waypoints to ID " << systemId;
        }
    }
}

void UASWaypointManager::handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_waypoint_request_t *wpr)
{
    protocol_timer.start(PROTOCOL_TIMEOUT_MS);

    if (systemId == current_partner_systemid && compId == current_partner_compid && ((current_state == WP_SENDLIST && wpr->seq == 0) || (current_state == WP_SENDLIST_SENDWPS && (wpr->seq == current_wp_id || wpr->seq == current_wp_id + 1))))
    {
        qDebug() << "handleWaypointRequest";

        if (wpr->seq < waypoint_buffer.count())
        {
            current_state = WP_SENDLIST_SENDWPS;
            current_wp_id = wpr->seq;
            sendWaypoint(current_wp_id);
        }
        else
        {
            //TODO: Error message or something
        }
    }
}

void UASWaypointManager::handleWaypointReached(quint8 systemId, quint8 compId, mavlink_waypoint_reached_t *wpr)
{
    if (systemId == uas.getUASID() && compId == MAV_COMP_ID_WAYPOINTPLANNER)
    {
        emit updateStatusString(QString("Reached waypoint %1").arg(wpr->seq));
    }
}

void UASWaypointManager::handleWaypointSetCurrent(quint8 systemId, quint8 compId, mavlink_waypoint_set_current_t *wpr)
{
    if (systemId == uas.getUASID() && compId == MAV_COMP_ID_WAYPOINTPLANNER)
    {
        qDebug() << "new current waypoint" << wpr->seq;
        emit currentWaypointChanged(wpr->seq);
    }
}

void UASWaypointManager::clearWaypointList()
{
    if(current_state == WP_IDLE)
    {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);

        mavlink_message_t message;
        mavlink_waypoint_clear_all_t wpca;

        wpca.target_system = uas.getUASID();
        wpca.target_component = MAV_COMP_ID_WAYPOINTPLANNER;

        current_state = WP_CLEARLIST;
        current_wp_id = 0;
        current_partner_systemid = uas.getUASID();
        current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

        const QString str = QString("clearing waypoint list...");
        emit updateStatusString(str);

        mavlink_msg_waypoint_clear_all_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpca);
        uas.sendMessage(message);
    }
}

void UASWaypointManager::requestWaypoints()
{
    if(current_state == WP_IDLE)
    {
        protocol_timer.start(PROTOCOL_TIMEOUT_MS);

        mavlink_message_t message;
        mavlink_waypoint_request_list_t wprl;

        wprl.target_system = uas.getUASID();
        wprl.target_component = MAV_COMP_ID_WAYPOINTPLANNER;

        current_state = WP_GETLIST;
        current_wp_id = 0;
        current_partner_systemid = uas.getUASID();
        current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

        const QString str = QString("requesting waypoint list...");
        emit updateStatusString(str);

        mavlink_msg_waypoint_request_list_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wprl);
        uas.sendMessage(message);

        qDebug() << "sent waypoint list request to ID " << wprl.target_system;
    }
}

void UASWaypointManager::sendWaypoints()
{
    if (current_state == WP_IDLE)
    {
        if (waypoints.count() > 0)
        {
            protocol_timer.start(PROTOCOL_TIMEOUT_MS);

            current_count = waypoints.count();
            current_state = WP_SENDLIST;
            current_wp_id = 0;
            current_partner_systemid = uas.getUASID();
            current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

            //clear local buffer
            while(!waypoint_buffer.empty())
            {
                delete waypoint_buffer.back();
                waypoint_buffer.pop_back();
            }

            //copy waypoint data to local buffer
            for (int i=0; i < current_count; i++)
            {
                waypoint_buffer.push_back(new mavlink_waypoint_t);
                mavlink_waypoint_t *cur_d = waypoint_buffer.back();
                memset(cur_d, 0, sizeof(mavlink_waypoint_t));   //initialize with zeros
                const Waypoint *cur_s = waypoints.at(i);

                cur_d->autocontinue = cur_s->getAutoContinue();
                cur_d->current = cur_s->getCurrent();
                cur_d->orbit = 0.f;     //FIXME
                cur_d->param1 = cur_s->getOrbit();
                cur_d->param2 = cur_s->getHoldTime();
                cur_d->type = 1;        //FIXME
                cur_d->seq = i;
                cur_d->x = cur_s->getX();
                cur_d->y = cur_s->getY();
                cur_d->z = cur_s->getZ();
                cur_d->yaw = cur_s->getYaw();
            }

            //send the waypoint count to UAS (this starts the send transaction)
            mavlink_message_t message;
            mavlink_waypoint_count_t wpc;

            wpc.target_system = uas.getUASID();
            wpc.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
            wpc.count = current_count;

            const QString str = QString("start transmitting waypoints...");
            emit updateStatusString(str);

            mavlink_msg_waypoint_count_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpc);
            uas.sendMessage(message);

            qDebug() << "sent waypoint count (" << wpc.count << ") to ID " << wpc.target_system;
        }
    }
    else
    {
        //we're in another transaction, ignore command
        qDebug() << "UASWaypointManager::sendWaypoints() doing something else ignoring command";
    }
}

void UASWaypointManager::sendWaypointRequest(quint16 seq)
{
    mavlink_message_t message;
    mavlink_waypoint_request_t wpr;

    wpr.target_system = uas.getUASID();
    wpr.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
    wpr.seq = seq;

    const QString str = QString("retrieving waypoint ID %1 of %2 total").arg(wpr.seq).arg(current_count);
    emit updateStatusString(str);

    mavlink_msg_waypoint_request_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpr);
    uas.sendMessage(message);

    qDebug() << "sent waypoint request (" << wpr.seq << ") to ID " << wpr.target_system;
}

void UASWaypointManager::sendWaypoint(quint16 seq)
{
    mavlink_message_t message;

    if (seq < waypoint_buffer.count())
    {
        mavlink_waypoint_t *wp;

        wp = waypoint_buffer.at(seq);
        wp->target_system = uas.getUASID();
        wp->target_component = MAV_COMP_ID_WAYPOINTPLANNER;

        const QString str = QString("sending waypoint ID %1 of %2 total").arg(wp->seq).arg(current_count);
        emit updateStatusString(str);

        mavlink_msg_waypoint_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, wp);
        uas.sendMessage(message);

        qDebug() << "sent waypoint (" << wp->seq << ") to ID " << wp->target_system;
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

    qDebug() << "sent waypoint ack (" << wpa.type << ") to ID " << wpa.target_system;
}
