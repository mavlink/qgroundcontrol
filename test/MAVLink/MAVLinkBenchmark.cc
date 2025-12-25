/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkBenchmark.h"
#include "MAVLinkLib.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(MAVLinkBenchmark)

// Pre-packed messages for unpack benchmarks
static mavlink_message_t s_heartbeatMsg;
static mavlink_message_t s_attitudeMsg;
static mavlink_message_t s_gpsRawMsg;
static uint8_t s_heartbeatBytes[MAVLINK_MAX_PACKET_LEN];
static uint16_t s_heartbeatLen = 0;

// Initialize static test data
static struct StaticInit {
    StaticInit() {
        // Pack a heartbeat message
        mavlink_msg_heartbeat_pack(
            1,                          // system_id
            MAV_COMP_ID_AUTOPILOT1,     // component_id
            &s_heartbeatMsg,
            MAV_TYPE_QUADROTOR,         // type
            MAV_AUTOPILOT_PX4,          // autopilot
            MAV_MODE_FLAG_SAFETY_ARMED, // base_mode
            0,                          // custom_mode
            MAV_STATE_ACTIVE            // system_status
        );

        // Pack an attitude message
        mavlink_msg_attitude_pack(
            1,
            MAV_COMP_ID_AUTOPILOT1,
            &s_attitudeMsg,
            12345678,   // time_boot_ms
            0.1f,       // roll
            0.2f,       // pitch
            1.5f,       // yaw
            0.01f,      // rollspeed
            0.02f,      // pitchspeed
            0.03f       // yawspeed
        );

        // Pack a GPS raw message (more complex)
        mavlink_msg_gps_raw_int_pack(
            1,
            MAV_COMP_ID_AUTOPILOT1,
            &s_gpsRawMsg,
            12345678000ULL,     // time_usec
            GPS_FIX_TYPE_3D_FIX,// fix_type
            473977420,          // lat (47.3977420)
            85455940,           // lon (8.5455940)
            450000,             // alt (450m in mm)
            100,                // eph
            100,                // epv
            1500,               // vel (15 m/s in cm/s)
            9000,               // cog (90 degrees in cdeg)
            12,                 // satellites_visible
            440000,             // alt_ellipsoid
            50,                 // h_acc
            50,                 // v_acc
            10,                 // vel_acc
            100,                // hdg_acc
            0                   // yaw
        );

        // Serialize heartbeat to bytes for parsing benchmark
        s_heartbeatLen = mavlink_msg_to_send_buffer(s_heartbeatBytes, &s_heartbeatMsg);
    }
} s_init;

void MAVLinkBenchmark::benchmarkHeartbeatPack()
{
    mavlink_message_t msg;

    QBENCHMARK {
        mavlink_msg_heartbeat_pack(
            1,
            MAV_COMP_ID_AUTOPILOT1,
            &msg,
            MAV_TYPE_QUADROTOR,
            MAV_AUTOPILOT_PX4,
            MAV_MODE_FLAG_SAFETY_ARMED,
            0,
            MAV_STATE_ACTIVE
        );
    }
}

void MAVLinkBenchmark::benchmarkHeartbeatUnpack()
{
    mavlink_heartbeat_t heartbeat;

    QBENCHMARK {
        mavlink_msg_heartbeat_decode(&s_heartbeatMsg, &heartbeat);
    }
}

void MAVLinkBenchmark::benchmarkAttitudePack()
{
    mavlink_message_t msg;

    QBENCHMARK {
        mavlink_msg_attitude_pack(
            1,
            MAV_COMP_ID_AUTOPILOT1,
            &msg,
            12345678,
            0.1f, 0.2f, 1.5f,
            0.01f, 0.02f, 0.03f
        );
    }
}

void MAVLinkBenchmark::benchmarkAttitudeUnpack()
{
    mavlink_attitude_t attitude;

    QBENCHMARK {
        mavlink_msg_attitude_decode(&s_attitudeMsg, &attitude);
    }
}

void MAVLinkBenchmark::benchmarkGpsRawPack()
{
    mavlink_message_t msg;

    QBENCHMARK {
        mavlink_msg_gps_raw_int_pack(
            1,
            MAV_COMP_ID_AUTOPILOT1,
            &msg,
            12345678000ULL,
            GPS_FIX_TYPE_3D_FIX,
            473977420, 85455940, 450000,
            100, 100, 1500, 9000, 12,
            440000, 50, 50, 10, 100, 0
        );
    }
}

void MAVLinkBenchmark::benchmarkMessageRoundTrip()
{
    mavlink_message_t msg;
    mavlink_heartbeat_t heartbeat;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

    QBENCHMARK {
        // Pack
        mavlink_msg_heartbeat_pack(
            1,
            MAV_COMP_ID_AUTOPILOT1,
            &msg,
            MAV_TYPE_QUADROTOR,
            MAV_AUTOPILOT_PX4,
            MAV_MODE_FLAG_SAFETY_ARMED,
            0,
            MAV_STATE_ACTIVE
        );

        // Serialize (includes CRC calculation)
        mavlink_msg_to_send_buffer(buffer, &msg);

        // Decode
        mavlink_msg_heartbeat_decode(&msg, &heartbeat);
    }
}

void MAVLinkBenchmark::benchmarkParseByteStream()
{
    mavlink_message_t msg;
    mavlink_status_t status;

    QBENCHMARK {
        // Parse byte-by-byte as if receiving from serial/UDP
        for (uint16_t i = 0; i < s_heartbeatLen; i++) {
            mavlink_parse_char(MAVLINK_COMM_0, s_heartbeatBytes[i], &msg, &status);
        }
    }
}
