#ifndef MAVLINKSIMULATIONWAYPOINTPLANNER_H
#define MAVLINKSIMULATIONWAYPOINTPLANNER_H

#include <QObject>
#include <vector>

#include "MAVLinkSimulationLink.h"
#include "QGCMAVLink.h"

enum PX_WAYPOINTPLANNER_STATES {
    PX_WPP_IDLE = 0,
    PX_WPP_SENDLIST,
    PX_WPP_SENDLIST_SENDWPS,
    PX_WPP_GETLIST,
    PX_WPP_GETLIST_GETWPS,
    PX_WPP_GETLIST_GOTALL
};

class MAVLinkSimulationWaypointPlanner : public QObject
{
    Q_OBJECT
public:
    explicit MAVLinkSimulationWaypointPlanner(MAVLinkSimulationLink *parent, int systemid);

signals:
    void messageSent(const mavlink_message_t& msg);

public slots:
    void handleMessage(const mavlink_message_t& msg);

protected:
    MAVLinkSimulationLink* link;
    bool idle;      				///< indicates if the system is following the waypoints or is waiting
    uint16_t current_active_wp_id;		///< id of current waypoint
    bool yawReached;						///< boolean for yaw attitude reached
    bool posReached;						///< boolean for position reached
    uint64_t timestamp_lastoutside_orbit;///< timestamp when the MAV was last outside the orbit or had the wrong yaw value
    uint64_t timestamp_firstinside_orbit;///< timestamp when the MAV was the first time after a waypoint change inside the orbit and had the correct yaw value

    std::vector<mavlink_mission_item_t*> waypoints1;	///< vector1 that holds the waypoints
    std::vector<mavlink_mission_item_t*> waypoints2;	///< vector2 that holds the waypoints

    std::vector<mavlink_mission_item_t*>* waypoints;		///< pointer to the currently active waypoint vector
    std::vector<mavlink_mission_item_t*>* waypoints_receive_buffer;	///< pointer to the receive buffer waypoint vector
    PX_WAYPOINTPLANNER_STATES current_state;
    uint16_t protocol_current_wp_id;
    uint16_t protocol_current_count;
    uint8_t protocol_current_partner_systemid;
    uint8_t protocol_current_partner_compid;
    uint64_t protocol_timestamp_lastaction;
    unsigned int protocol_timeout;
    uint64_t timestamp_last_send_setpoint;
    uint8_t systemid;
    uint8_t compid;
    unsigned int setpointDelay;
    float yawTolerance;
    bool verbose;
    bool debug;
    bool silent;

    void send_waypoint_ack(uint8_t target_systemid, uint8_t target_compid, uint8_t type);
    void send_waypoint_current(uint16_t seq);
    void send_setpoint(uint16_t seq);
    void send_waypoint_count(uint8_t target_systemid, uint8_t target_compid, uint16_t count);
    void send_waypoint(uint8_t target_systemid, uint8_t target_compid, uint16_t seq);
    void send_waypoint_request(uint8_t target_systemid, uint8_t target_compid, uint16_t seq);
    void send_waypoint_reached(uint16_t seq);
    float distanceToSegment(uint16_t seq, float x, float y, float z);
    float distanceToPoint(uint16_t seq, float x, float y, float z);
    float distanceToPoint(uint16_t seq, float x, float y);
    void mavlink_handler(const mavlink_message_t* msg);

};

#endif // MAVLINKSIMULATIONWAYPOINTPLANNER_H
