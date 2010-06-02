#include "UASWaypointManager.h"
#include "UAS.h"

UASWaypointManager::UASWaypointManager(UAS &_uas)
        : uas(_uas)
{
}

void UASWaypointManager::waypointChanged(Waypoint*)
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
        mavlink_message_t message;
        mavlink_waypoint_request_list_t wprl;

        wprl.target_system = uas.getUASID();
        wprl.target_system = MAV_COMP_ID_WAYPOINTPLANNER;

        current_state = WP_GETLIST;
        current_wp_id = 0;
        current_partner_systemid = uas.getUASID();
        current_partner_compid = MAV_COMP_ID_WAYPOINTPLANNER;

        uas.sendMessage(message);
    }
}

void UASWaypointManager::handleWaypointCount(quint8 systemId, quint8 compId, quint16 count)
{
    if (current_state == WP_GETLIST && systemId == current_partner_systemid && compId == current_partner_compid)
    {
        current_count = count;

        mavlink_message_t message;
        mavlink_waypoint_request_t wpr;

        wpr.target_system = uas.getUASID();
        wpr.target_system = MAV_COMP_ID_WAYPOINTPLANNER;
        wpr.seq = current_wp_id;

        current_state = WP_GETLIST_GETWPS;

        uas.sendMessage(message);
    }
}

void UASWaypointManager::getWaypoint(quint16 seq)
{

}

void UASWaypointManager::clearWaypointList()
{
}
