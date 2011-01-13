#include <QDebug>

#include "MAVLinkSimulationMAV.h"

MAVLinkSimulationMAV::MAVLinkSimulationMAV(MAVLinkSimulationLink *parent, int systemid) :
    QObject(parent),
    link(parent),
    planner(parent, systemid),
    systemid(systemid)
{
    connect(&mainloopTimer, SIGNAL(timeout()), this, SLOT(mainloop()));
    mainloopTimer.start(1000);
    connect(link, SIGNAL(messageReceived(mavlink_message_t)), this, SLOT(handleMessage(mavlink_message_t)));
    mainloop();
}

void MAVLinkSimulationMAV::mainloop()
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(systemid, MAV_COMP_ID_IMU, &msg, MAV_FIXED_WING, MAV_AUTOPILOT_SLUGS);
    link->sendMAVLinkMessage(&msg);
}

void MAVLinkSimulationMAV::handleMessage(const mavlink_message_t& msg)
{
    qDebug() << "MAV:" << systemid << "RECEIVED MESSAGE FROM" << msg.sysid << "COMP" << msg.compid;

    switch(msg.msgid)
    {
    case MAVLINK_MSG_ID_ATTITUDE:
        break;
    }
}
