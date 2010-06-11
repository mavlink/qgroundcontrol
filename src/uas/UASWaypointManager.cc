#include "UASWaypointManager.h"
#include "UAS.h"

UASWaypointManager::UASWaypointManager(UAS &_uas)
        : uas(_uas),
        current_wp_id(0),
        current_count(0),
        current_state(WP_IDLE),
        current_partner_systemid(0),
        current_partner_compid(0)
{
}

void UASWaypointManager::handleWaypointCount(quint8 systemId, quint8 compId, quint16 count)
{
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
    if (systemId == current_partner_systemid && compId == current_partner_compid && current_state == WP_GETLIST_GETWPS && wp->seq == current_wp_id)
    {
        qDebug() << "got waypoint (" << wp->seq << ") from ID " << systemId;

        if(wp->seq == current_wp_id)
        {
            //update the UI FIXME
            emit waypointUpdated(uas.getUASID(), wp->seq, wp->x, wp->y, wp->z, wp->yaw, wp->autocontinue, wp->current);

            //get next waypoint
            current_wp_id++;

            if(current_wp_id < current_count)
            {
                sendWaypointRequest(current_wp_id);
            }
            else
            {
                // all waypoints retrieved, change state to idle
                current_state = WP_IDLE;
                current_count = 0;
                current_wp_id = 0;
                current_partner_systemid = 0;
                current_partner_compid = 0;

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

void UASWaypointManager::handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_waypoint_request_t *wpr)
{
    if (systemId == current_partner_systemid && compId == current_partner_compid && ((current_state == WP_SENDLIST && wpr->seq == 0) || (current_state == WP_SENDLIST_SENDWPS && (wpr->seq == current_wp_id || wpr->seq == current_wp_id + 1)) || (current_state == WP_IDLE && wpr->seq == current_count-1)))
    {
        qDebug() << "handleWaypointRequest";

        if (wpr->seq < waypoint_buffer.count())
        {
            current_state = WP_SENDLIST_SENDWPS;
            current_wp_id = wpr->seq;
            sendWaypoint(current_wp_id);

            if(current_wp_id == waypoint_buffer.count()-1)
            {
                //all waypoints sent, but we still have to wait for a possible rerequest of the last waypoint
                current_state = WP_IDLE;

                emit updateStatusString("done.");

                qDebug() << "send all waypoints to ID " << systemId;
            }
        }
        else
        {
            //TODO: Error message or something
        }
    }
}

void UASWaypointManager::clearWaypointList()
{

}

void UASWaypointManager::currentWaypointChanged(quint16)
{

}

void UASWaypointManager::removeWaypointId(quint16)
{

}

void UASWaypointManager::requestWaypoints()
{
    if(current_state == WP_IDLE)
    {
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

void UASWaypointManager::sendWaypoints(const QVector<Waypoint*> &list)
{
    if (current_state == WP_IDLE)
    {
        if (list.count() > 0)
        {
            current_count = list.count();
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
                const Waypoint *cur_s = list.at(i);

                cur_d->autocontinue = cur_s->getAutoContinue();
                cur_d->current = cur_s->getCurrent();
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

void UASWaypointManager::waypointChanged(Waypoint*)
{

}
