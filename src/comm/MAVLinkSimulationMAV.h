#ifndef MAVLINKSIMULATIONMAV_H
#define MAVLINKSIMULATIONMAV_H

#include <QObject>
#include <QTimer>

#include "MAVLinkSimulationLink.h"
#include "MAVLinkSimulationWaypointPlanner.h"

class MAVLinkSimulationMAV : public QObject
{
    Q_OBJECT
public:
    explicit MAVLinkSimulationMAV(MAVLinkSimulationLink *parent, int systemid, double lat=47.376389, double lon=8.548056, int version=MAVLINK_VERSION);

signals:

public slots:
    void mainloop();
    void handleMessage(const mavlink_message_t& msg);

protected:
    MAVLinkSimulationLink* link;
    MAVLinkSimulationWaypointPlanner planner;
    int systemid;
    QTimer mainloopTimer;
    int timer25Hz;
    int timer10Hz;
    int timer1Hz;
    double latitude;
    double longitude;
    double altitude;
    double x;
    double y;
    double z;
    double roll;
    double pitch;
    double yaw;
    double rollspeed;
    double pitchspeed;
    double yawspeed;

    bool globalNavigation;
    bool firstWP;

    double previousSPX;
    double previousSPY;
    double previousSPZ;
    double previousSPYaw;

    double nextSPX;
    double nextSPY;
    double nextSPZ;
    double nextSPYaw;
    uint8_t sys_mode;
    uint8_t sys_state;
    uint8_t nav_mode;
    bool flying;
    int mavlink_version;

    // FIXME MAVLINKV10PORTINGNEEDED
//    static inline uint16_t mavlink_msg_heartbeat_pack_version_free(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t type, uint8_t autopilot, uint8_t version) {
//        uint16_t i = 0;
//        msg->msgid = MAVLINK_MSG_ID_HEARTBEAT;

//        i += put_uint8_t_by_index(type, i, msg->payload); // Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
//        i += put_uint8_t_by_index(autopilot, i, msg->payload); // Type of the Autopilot: 0: Generic, 1: PIXHAWK, 2: SLUGS, 3: Ardupilot (up to 15 types), defined in MAV_AUTOPILOT_TYPE ENUM
//        i += put_uint8_t_by_index(version, i, msg->payload); // MAVLink version

//        return mavlink_finalize_message(msg, system_id, component_id, i);
//    }

};

#endif // MAVLINKSIMULATIONMAV_H
