#include <QDebug>
#include <cmath>

#include "MAVLinkSimulationMAV.h"

MAVLinkSimulationMAV::MAVLinkSimulationMAV(MAVLinkSimulationLink *parent, int systemid, double lat, double lon) :
        QObject(parent),
        link(parent),
        planner(parent, systemid),
        systemid(systemid),
        timer25Hz(0),
        timer10Hz(0),
        timer1Hz(0),
        latitude(lat),
        longitude(lon),
        altitude(0.0),
        x(lon),
        y(lat),
        z(550),
        roll(0.0),
        pitch(0.0),
        yaw(0.0),
        globalNavigation(true),
        firstWP(false),
        previousSPX(8.548056),
        previousSPY(47.376389),
        previousSPZ(550),
        previousSPYaw(0.0),
        nextSPX(8.548056),
        nextSPY(47.376389),
        nextSPZ(550),
        nextSPYaw(0.0),
        sys_mode(MAV_MODE_READY),
        sys_state(MAV_STATE_STANDBY),
        nav_mode(MAV_NAV_GROUNDED),
        flying(false)
{
    // Please note: The waypoint planner is running
    connect(&mainloopTimer, SIGNAL(timeout()), this, SLOT(mainloop()));
    connect(&planner, SIGNAL(messageSent(mavlink_message_t)), this, SLOT(handleMessage(mavlink_message_t)));
    connect(link, SIGNAL(messageReceived(mavlink_message_t)), this, SLOT(handleMessage(mavlink_message_t)));
    mainloopTimer.start(20);
    mainloop();
}

void MAVLinkSimulationMAV::mainloop()
{
    // Calculate new simulator values
    //    double maxSpeed = 0.0001; // rad/s in earth coordinate frame

    //        double xNew = // (nextSPX - previousSPX)

    if (flying)
    {
        sys_state = MAV_STATE_ACTIVE;
        sys_mode = MAV_MODE_AUTO;
        nav_mode = MAV_NAV_WAYPOINT;
    }

    // 1 Hz execution
    if (timer1Hz <= 0)
    {
        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(systemid, MAV_COMP_ID_IMU, &msg, MAV_FIXED_WING, MAV_AUTOPILOT_PIXHAWK);
        link->sendMAVLinkMessage(&msg);
        planner.handleMessage(msg);
        timer1Hz = 50;
    }

    // 10 Hz execution
    if (timer10Hz <= 0)
    {
        double radPer100ms = 0.00006;
        double altPer100ms = 0.4;
        double xm = (nextSPX - x);
        double ym = (nextSPY - y);
        double zm = (nextSPZ - z);

        float zsign = (zm < 0) ? -1.0f : 1.0f;

        if (!firstWP)
        {
            //float trueyaw = atan2f(xm, ym);

            float newYaw = atan2f(xm, ym);

            if (fabs(yaw - newYaw) < 90)
            {
                yaw = yaw*0.7 + 0.3*newYaw;
            }
            else
            {
                yaw = newYaw;
            }

            //qDebug() << "SIMULATION MAV: x:" << xm << "y:" << ym << "z:" << zm << "yaw:" << yaw;

            //if (sqrt(xm*xm+ym*ym) > 0.0000001)
            if (flying)
            {
                x += sin(yaw)*radPer100ms;
                y += cos(yaw)*radPer100ms;
                z += altPer100ms*zsign;
            }

            //if (xm < 0.001) xm
        }
        else
        {
            x = nextSPX;
            y = nextSPY;
            z = nextSPZ;
            firstWP = false;
            qDebug() << "INIT STEP";
        }


        // GLOBAL POSITION
        mavlink_message_t msg;
        mavlink_global_position_int_t pos;
        pos.alt = z*1000.0;
        pos.lat = y*1E7;
        pos.lon = x*1E7;
        pos.vx = sin(yaw)*10.0f*100.0f;
        pos.vy = cos(yaw)*10.0f*100.0f;
        pos.vz = altPer100ms*10.0f*100.0f*zsign*-1.0f;
        mavlink_msg_global_position_int_encode(systemid, MAV_COMP_ID_IMU, &msg, &pos);
        link->sendMAVLinkMessage(&msg);
        planner.handleMessage(msg);

        // ATTITUDE
        mavlink_attitude_t attitude;
        attitude.roll = 0.0f;
        attitude.pitch = 0.0f;
        attitude.yaw = yaw;

        mavlink_msg_attitude_encode(systemid, MAV_COMP_ID_IMU, &msg, &attitude);
        link->sendMAVLinkMessage(&msg);

        // SYSTEM STATUS
        mavlink_sys_status_t status;
        status.load = 300;
        status.mode = sys_mode;
        status.nav_mode = nav_mode;
        status.packet_drop = 0;
        status.vbat = 10500;
        status.status = sys_state;

        mavlink_msg_sys_status_encode(systemid, MAV_COMP_ID_IMU, &msg, &status);
        link->sendMAVLinkMessage(&msg);
        timer10Hz = 5;
    }

    // 25 Hz execution
    if (timer25Hz <= 0)
    {
        // The message container to be used for sending
        mavlink_message_t ret;

        #ifdef MAVLINK_ENABLED_PIXHAWK
        // Send which controllers are active
        mavlink_control_status_t control_status;
        control_status.control_att = 1;
        control_status.control_pos_xy = 1;
        control_status.control_pos_yaw = 1;
        control_status.control_pos_z = 1;
        control_status.gps_fix = 2;        // 2D GPS fix
        control_status.position_fix = 3;   // 3D fix from GPS + barometric pressure
        control_status.vision_fix = 0;     // no fix from vision system
        mavlink_msg_control_status_encode(systemid, MAV_COMP_ID_IMU, &ret, &control_status);
        link->sendMAVLinkMessage(&ret);
        #endif //MAVLINK_ENABLED_PIXHAWK

        // Send actual controller outputs
        // This message just shows the direction
        // and magnitude of the control output
//        mavlink_position_controller_output_t pos;
//        pos.x = sin(yaw)*127.0f;
//        pos.y = cos(yaw)*127.0f;
//        pos.z = 0;
//        mavlink_msg_position_controller_output_encode(systemid, MAV_COMP_ID_IMU, &ret, &pos);
//        link->sendMAVLinkMessage(&ret);

        // Send a named value with name "FLOAT" and 0.5f as value

        // The message container to be used for sending
        //mavlink_message_t ret;
        // The C-struct holding the data to be sent
        mavlink_named_value_float_t val;

        // Fill in the data
        // Name of the value, maximum 10 characters
        // see full message specs at:
        // http://pixhawk.ethz.ch/wiki/mavlink/
        strcpy((char *)val.name, "FLOAT");
        // Value, in this case 0.5
        val.value = 0.5f;

        // Encode the data (adding header and checksums, etc.)
        mavlink_msg_named_value_float_encode(systemid, MAV_COMP_ID_IMU, &ret, &val);
        // And send it
        link->sendMAVLinkMessage(&ret);

        // MICROCONTROLLER SEND CODE:

        // uint8_t buf[MAVLINK_MAX_PACKET_LEN];
        // int16_t len = mavlink_msg_to_send_buffer(buf, &ret);
        // uart0_transmit(buf, len);


        // SEND INTEGER VALUE

        // We are reusing the "mavlink_message_t ret"
        // message buffer

        // The C-struct holding the data to be sent
        mavlink_named_value_int_t valint;

        // Fill in the data
        // Name of the value, maximum 10 characters
        // see full message specs at:
        // http://pixhawk.ethz.ch/wiki/mavlink/
        strcpy((char *)valint.name, "INTEGER");
        // Value, in this case 18000
        valint.value = 18000;

        // Encode the data (adding header and checksums, etc.)
        mavlink_msg_named_value_int_encode(systemid, MAV_COMP_ID_IMU, &ret, &valint);
        // And send it
        link->sendMAVLinkMessage(&ret);

        // MICROCONTROLLER SEND CODE:

        // uint8_t buf[MAVLINK_MAX_PACKET_LEN];
        // int16_t len = mavlink_msg_to_send_buffer(buf, &ret);
        // uart0_transmit(buf, len);

        timer25Hz = 2;
    }

    timer1Hz--;
    timer10Hz--;
    timer25Hz--;
}

void MAVLinkSimulationMAV::handleMessage(const mavlink_message_t& msg)
{
    //qDebug() << "MAV:" << systemid << "RECEIVED MESSAGE FROM" << msg.sysid << "COMP" << msg.compid;

    switch(msg.msgid)
    {
    case MAVLINK_MSG_ID_ATTITUDE:
        break;
    case MAVLINK_MSG_ID_SET_MODE:
        {
            mavlink_set_mode_t mode;
            mavlink_msg_set_mode_decode(&msg, &mode);
            if (systemid == mode.target) sys_mode = mode.mode;
        }
        break;
    case MAVLINK_MSG_ID_ACTION:
        {
            mavlink_action_t action;
            mavlink_msg_action_decode(&msg, &action);
            if (systemid == action.target && (action.target_component == 0 || action.target_component == MAV_COMP_ID_IMU))
            {
                switch (action.action)
                {
                case MAV_ACTION_TAKEOFF:
                    flying = true;
                    break;
                default:
                    {
                        mavlink_statustext_t text;
                        mavlink_message_t r_msg;
                        sprintf((char*)text.text, "MAV%d ignored unknown action %d", systemid, action.action);
                        mavlink_msg_statustext_encode(systemid, MAV_COMP_ID_IMU, &r_msg, &text);
                        link->sendMAVLinkMessage(&r_msg);
                    }
                    break;
                }
            }
        }
        break;
    case MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET:
        {
            mavlink_local_position_setpoint_set_t sp;
            mavlink_msg_local_position_setpoint_set_decode(&msg, &sp);
            if (sp.target_system == this->systemid)
            {
                previousSPX = nextSPX;
                previousSPY = nextSPY;
                previousSPZ = nextSPZ;
                nextSPX = sp.x;
                nextSPY = sp.y;
                nextSPZ = sp.z;

                // Rotary wing
                //nextSPYaw = sp.yaw;

                // Airplane
                //yaw = atan2(previousSPX-nextSPX, previousSPY-nextSPY);

                //if (!firstWP) firstWP = true;
            }
            //qDebug() << "UPDATED SP:" << "X" << nextSPX << "Y" << nextSPY << "Z" << nextSPZ;
        }
        break;
    }
}
