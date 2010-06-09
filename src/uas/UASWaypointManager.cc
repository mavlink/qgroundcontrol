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
        qDebug() << "handleWaypointCount";

        current_count = count;

        mavlink_message_t message;
        mavlink_waypoint_request_t wpr;

        wpr.target_system = uas.getUASID();
        wpr.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
        wpr.seq = 0;

        current_wp_id = 0;
        current_state = WP_GETLIST_GETWPS;

        mavlink_msg_waypoint_request_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpr);
        uas.sendMessage(message);
    }
}

void UASWaypointManager::handleWaypoint(quint8 systemId, quint8 compId, mavlink_waypoint_t *wp)
{
    if (systemId == current_partner_systemid && compId == current_partner_compid && current_state == WP_GETLIST_GETWPS && wp->seq == current_wp_id)
    {
        qDebug() << "handleWaypoint";

        if(wp->seq == current_wp_id)
        {
            //update the UI FIXME
            emit waypointUpdated(uas.getUASID(), wp->seq, wp->x, wp->y, wp->z, wp->yaw, wp->autocontinue, wp->current);

            //get next waypoint
            current_wp_id++;

            if(current_wp_id < current_count)
            {
                mavlink_message_t message;
                mavlink_waypoint_request_t wpr;

                wpr.target_system = uas.getUASID();
                wpr.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
                wpr.seq = current_wp_id;

                mavlink_msg_waypoint_request_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpr);
                uas.sendMessage(message);
            }
            else
            {
                // all waypoints retrieved, change state to idle
                current_state = WP_IDLE;
                current_count = 0;
                current_wp_id = 0;
                current_partner_systemid = 0;
                current_partner_compid = 0;
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
    if (systemId == current_partner_systemid && compId == current_partner_compid && current_state == WP_SENDLIST && wpr->seq == current_wp_id)
    {
        qDebug() << "handleWaypointRequest";

        if (wpr->seq < 0)
        {
            //TODO: send waypoint
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

void UASWaypointManager::currentWaypointChanged(int)
{

}

void UASWaypointManager::removeWaypointId(int)
{

}

void UASWaypointManager::requestWaypoints()
{
    if(current_state == WP_IDLE)
    {
        qDebug() << "requestWaypoints";
        mavlink_message_t message;
        mavlink_waypoint_request_list_t wprl;

        wprl.target_system = uas.getUASID();
        wprl.target_component = MAV_COMP_ID_WAYPOINTPLANNER;

        current_state = WP_GETLIST;
        current_wp_id = 0;
        current_partner_systemid = uas.getUASID();
        current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

        mavlink_msg_waypoint_request_list_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wprl);
        uas.sendMessage(message);
    }
}

void UASWaypointManager::sendWaypoints(void)
{
    mavlink_message_t message;
    mavlink_waypoint_count_t wpc;

    wpc.target_system = uas.getUASID();
    wpc.target_component = MAV_COMP_ID_WAYPOINTPLANNER;
    wpc.count = 0;  //TODO

    mavlink_msg_waypoint_count_encode(uas.mavlink->getSystemId(), uas.mavlink->getComponentId(), &message, &wpc);
    uas.sendMessage(message);
}

void UASWaypointManager::getWaypoint(quint16 seq)
{

}

void UASWaypointManager::waypointChanged(Waypoint*)
{

}
