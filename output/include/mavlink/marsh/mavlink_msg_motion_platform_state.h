#pragma once
// MESSAGE MOTION_PLATFORM_STATE PACKING

#define MAVLINK_MSG_ID_MOTION_PLATFORM_STATE 52502


typedef struct __mavlink_motion_platform_state_t {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 float x; /*< [m] X axis (surge) position, positive forward.*/
 float y; /*< [m] Y axis (sway) position, positive right.*/
 float z; /*< [m] Z axis (heave) position, positive down.*/
 float roll; /*< [rad] Roll position, positive right.*/
 float pitch; /*< [rad] Pitch position, positive nose up.*/
 float yaw; /*< [rad] Yaw position, positive right.*/
 float vel_x; /*< [m/s] X axis (surge) velocity, positive forward.*/
 float vel_y; /*< [m/s] Y axis (sway) velocity, positive right.*/
 float vel_z; /*< [m/s] Z axis (heave) velocity, positive down.*/
 float vel_roll; /*< [rad/s] Roll velocity, positive right.*/
 float vel_pitch; /*< [rad/s] Pitch velocity, positive nose up.*/
 float vel_yaw; /*< [rad/s] Yaw velocity, positive right.*/
 float acc_x; /*< [m/s/s] X axis (surge) acceleration, positive forward.*/
 float acc_y; /*< [m/s/s] Y axis (sway) acceleration, positive right.*/
 float acc_z; /*< [m/s/s] Z axis (heave) acceleration, positive down.*/
 float acc_roll; /*<  Roll acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd*/
 float acc_pitch; /*<  Pitch acceleration, positive nose up. Unit rad/s/s, currently not part of mavschema.xsd*/
 float acc_yaw; /*<  Yaw acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd*/
 uint8_t health; /*<  Generic system health (error and warning) status.*/
 uint8_t mode; /*<  Generic system operating mode.*/
} mavlink_motion_platform_state_t;

#define MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN 78
#define MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN 78
#define MAVLINK_MSG_ID_52502_LEN 78
#define MAVLINK_MSG_ID_52502_MIN_LEN 78

#define MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC 88
#define MAVLINK_MSG_ID_52502_CRC 88



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_MOTION_PLATFORM_STATE { \
    52502, \
    "MOTION_PLATFORM_STATE", \
    21, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_motion_platform_state_t, time_boot_ms) }, \
         { "health", NULL, MAVLINK_TYPE_UINT8_T, 0, 76, offsetof(mavlink_motion_platform_state_t, health) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 77, offsetof(mavlink_motion_platform_state_t, mode) }, \
         { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_motion_platform_state_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_motion_platform_state_t, y) }, \
         { "z", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_motion_platform_state_t, z) }, \
         { "roll", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_motion_platform_state_t, roll) }, \
         { "pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_motion_platform_state_t, pitch) }, \
         { "yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_motion_platform_state_t, yaw) }, \
         { "vel_x", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_motion_platform_state_t, vel_x) }, \
         { "vel_y", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_motion_platform_state_t, vel_y) }, \
         { "vel_z", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_motion_platform_state_t, vel_z) }, \
         { "vel_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_motion_platform_state_t, vel_roll) }, \
         { "vel_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_motion_platform_state_t, vel_pitch) }, \
         { "vel_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 48, offsetof(mavlink_motion_platform_state_t, vel_yaw) }, \
         { "acc_x", NULL, MAVLINK_TYPE_FLOAT, 0, 52, offsetof(mavlink_motion_platform_state_t, acc_x) }, \
         { "acc_y", NULL, MAVLINK_TYPE_FLOAT, 0, 56, offsetof(mavlink_motion_platform_state_t, acc_y) }, \
         { "acc_z", NULL, MAVLINK_TYPE_FLOAT, 0, 60, offsetof(mavlink_motion_platform_state_t, acc_z) }, \
         { "acc_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 64, offsetof(mavlink_motion_platform_state_t, acc_roll) }, \
         { "acc_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 68, offsetof(mavlink_motion_platform_state_t, acc_pitch) }, \
         { "acc_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 72, offsetof(mavlink_motion_platform_state_t, acc_yaw) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_MOTION_PLATFORM_STATE { \
    "MOTION_PLATFORM_STATE", \
    21, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_motion_platform_state_t, time_boot_ms) }, \
         { "health", NULL, MAVLINK_TYPE_UINT8_T, 0, 76, offsetof(mavlink_motion_platform_state_t, health) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 77, offsetof(mavlink_motion_platform_state_t, mode) }, \
         { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_motion_platform_state_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_motion_platform_state_t, y) }, \
         { "z", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_motion_platform_state_t, z) }, \
         { "roll", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_motion_platform_state_t, roll) }, \
         { "pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_motion_platform_state_t, pitch) }, \
         { "yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_motion_platform_state_t, yaw) }, \
         { "vel_x", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_motion_platform_state_t, vel_x) }, \
         { "vel_y", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_motion_platform_state_t, vel_y) }, \
         { "vel_z", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_motion_platform_state_t, vel_z) }, \
         { "vel_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_motion_platform_state_t, vel_roll) }, \
         { "vel_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_motion_platform_state_t, vel_pitch) }, \
         { "vel_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 48, offsetof(mavlink_motion_platform_state_t, vel_yaw) }, \
         { "acc_x", NULL, MAVLINK_TYPE_FLOAT, 0, 52, offsetof(mavlink_motion_platform_state_t, acc_x) }, \
         { "acc_y", NULL, MAVLINK_TYPE_FLOAT, 0, 56, offsetof(mavlink_motion_platform_state_t, acc_y) }, \
         { "acc_z", NULL, MAVLINK_TYPE_FLOAT, 0, 60, offsetof(mavlink_motion_platform_state_t, acc_z) }, \
         { "acc_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 64, offsetof(mavlink_motion_platform_state_t, acc_roll) }, \
         { "acc_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 68, offsetof(mavlink_motion_platform_state_t, acc_pitch) }, \
         { "acc_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 72, offsetof(mavlink_motion_platform_state_t, acc_yaw) }, \
         } \
}
#endif

/**
 * @brief Pack a motion_platform_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param health  Generic system health (error and warning) status.
 * @param mode  Generic system operating mode.
 * @param x [m] X axis (surge) position, positive forward.
 * @param y [m] Y axis (sway) position, positive right.
 * @param z [m] Z axis (heave) position, positive down.
 * @param roll [rad] Roll position, positive right.
 * @param pitch [rad] Pitch position, positive nose up.
 * @param yaw [rad] Yaw position, positive right.
 * @param vel_x [m/s] X axis (surge) velocity, positive forward.
 * @param vel_y [m/s] Y axis (sway) velocity, positive right.
 * @param vel_z [m/s] Z axis (heave) velocity, positive down.
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @param acc_roll  Roll acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_pitch  Pitch acceleration, positive nose up. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_yaw  Yaw acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_motion_platform_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint8_t health, uint8_t mode, float x, float y, float z, float roll, float pitch, float yaw, float vel_x, float vel_y, float vel_z, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z, float acc_roll, float acc_pitch, float acc_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, x);
    _mav_put_float(buf, 8, y);
    _mav_put_float(buf, 12, z);
    _mav_put_float(buf, 16, roll);
    _mav_put_float(buf, 20, pitch);
    _mav_put_float(buf, 24, yaw);
    _mav_put_float(buf, 28, vel_x);
    _mav_put_float(buf, 32, vel_y);
    _mav_put_float(buf, 36, vel_z);
    _mav_put_float(buf, 40, vel_roll);
    _mav_put_float(buf, 44, vel_pitch);
    _mav_put_float(buf, 48, vel_yaw);
    _mav_put_float(buf, 52, acc_x);
    _mav_put_float(buf, 56, acc_y);
    _mav_put_float(buf, 60, acc_z);
    _mav_put_float(buf, 64, acc_roll);
    _mav_put_float(buf, 68, acc_pitch);
    _mav_put_float(buf, 72, acc_yaw);
    _mav_put_uint8_t(buf, 76, health);
    _mav_put_uint8_t(buf, 77, mode);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#else
    mavlink_motion_platform_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.roll = roll;
    packet.pitch = pitch;
    packet.yaw = yaw;
    packet.vel_x = vel_x;
    packet.vel_y = vel_y;
    packet.vel_z = vel_z;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;
    packet.acc_roll = acc_roll;
    packet.acc_pitch = acc_pitch;
    packet.acc_yaw = acc_yaw;
    packet.health = health;
    packet.mode = mode;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MOTION_PLATFORM_STATE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
}

/**
 * @brief Pack a motion_platform_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param health  Generic system health (error and warning) status.
 * @param mode  Generic system operating mode.
 * @param x [m] X axis (surge) position, positive forward.
 * @param y [m] Y axis (sway) position, positive right.
 * @param z [m] Z axis (heave) position, positive down.
 * @param roll [rad] Roll position, positive right.
 * @param pitch [rad] Pitch position, positive nose up.
 * @param yaw [rad] Yaw position, positive right.
 * @param vel_x [m/s] X axis (surge) velocity, positive forward.
 * @param vel_y [m/s] Y axis (sway) velocity, positive right.
 * @param vel_z [m/s] Z axis (heave) velocity, positive down.
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @param acc_roll  Roll acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_pitch  Pitch acceleration, positive nose up. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_yaw  Yaw acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_motion_platform_state_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint8_t health, uint8_t mode, float x, float y, float z, float roll, float pitch, float yaw, float vel_x, float vel_y, float vel_z, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z, float acc_roll, float acc_pitch, float acc_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, x);
    _mav_put_float(buf, 8, y);
    _mav_put_float(buf, 12, z);
    _mav_put_float(buf, 16, roll);
    _mav_put_float(buf, 20, pitch);
    _mav_put_float(buf, 24, yaw);
    _mav_put_float(buf, 28, vel_x);
    _mav_put_float(buf, 32, vel_y);
    _mav_put_float(buf, 36, vel_z);
    _mav_put_float(buf, 40, vel_roll);
    _mav_put_float(buf, 44, vel_pitch);
    _mav_put_float(buf, 48, vel_yaw);
    _mav_put_float(buf, 52, acc_x);
    _mav_put_float(buf, 56, acc_y);
    _mav_put_float(buf, 60, acc_z);
    _mav_put_float(buf, 64, acc_roll);
    _mav_put_float(buf, 68, acc_pitch);
    _mav_put_float(buf, 72, acc_yaw);
    _mav_put_uint8_t(buf, 76, health);
    _mav_put_uint8_t(buf, 77, mode);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#else
    mavlink_motion_platform_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.roll = roll;
    packet.pitch = pitch;
    packet.yaw = yaw;
    packet.vel_x = vel_x;
    packet.vel_y = vel_y;
    packet.vel_z = vel_z;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;
    packet.acc_roll = acc_roll;
    packet.acc_pitch = acc_pitch;
    packet.acc_yaw = acc_yaw;
    packet.health = health;
    packet.mode = mode;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MOTION_PLATFORM_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#endif
}

/**
 * @brief Pack a motion_platform_state message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param health  Generic system health (error and warning) status.
 * @param mode  Generic system operating mode.
 * @param x [m] X axis (surge) position, positive forward.
 * @param y [m] Y axis (sway) position, positive right.
 * @param z [m] Z axis (heave) position, positive down.
 * @param roll [rad] Roll position, positive right.
 * @param pitch [rad] Pitch position, positive nose up.
 * @param yaw [rad] Yaw position, positive right.
 * @param vel_x [m/s] X axis (surge) velocity, positive forward.
 * @param vel_y [m/s] Y axis (sway) velocity, positive right.
 * @param vel_z [m/s] Z axis (heave) velocity, positive down.
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @param acc_roll  Roll acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_pitch  Pitch acceleration, positive nose up. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_yaw  Yaw acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_motion_platform_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint8_t health,uint8_t mode,float x,float y,float z,float roll,float pitch,float yaw,float vel_x,float vel_y,float vel_z,float vel_roll,float vel_pitch,float vel_yaw,float acc_x,float acc_y,float acc_z,float acc_roll,float acc_pitch,float acc_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, x);
    _mav_put_float(buf, 8, y);
    _mav_put_float(buf, 12, z);
    _mav_put_float(buf, 16, roll);
    _mav_put_float(buf, 20, pitch);
    _mav_put_float(buf, 24, yaw);
    _mav_put_float(buf, 28, vel_x);
    _mav_put_float(buf, 32, vel_y);
    _mav_put_float(buf, 36, vel_z);
    _mav_put_float(buf, 40, vel_roll);
    _mav_put_float(buf, 44, vel_pitch);
    _mav_put_float(buf, 48, vel_yaw);
    _mav_put_float(buf, 52, acc_x);
    _mav_put_float(buf, 56, acc_y);
    _mav_put_float(buf, 60, acc_z);
    _mav_put_float(buf, 64, acc_roll);
    _mav_put_float(buf, 68, acc_pitch);
    _mav_put_float(buf, 72, acc_yaw);
    _mav_put_uint8_t(buf, 76, health);
    _mav_put_uint8_t(buf, 77, mode);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#else
    mavlink_motion_platform_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.roll = roll;
    packet.pitch = pitch;
    packet.yaw = yaw;
    packet.vel_x = vel_x;
    packet.vel_y = vel_y;
    packet.vel_z = vel_z;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;
    packet.acc_roll = acc_roll;
    packet.acc_pitch = acc_pitch;
    packet.acc_yaw = acc_yaw;
    packet.health = health;
    packet.mode = mode;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MOTION_PLATFORM_STATE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
}

/**
 * @brief Encode a motion_platform_state struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param motion_platform_state C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_motion_platform_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_motion_platform_state_t* motion_platform_state)
{
    return mavlink_msg_motion_platform_state_pack(system_id, component_id, msg, motion_platform_state->time_boot_ms, motion_platform_state->health, motion_platform_state->mode, motion_platform_state->x, motion_platform_state->y, motion_platform_state->z, motion_platform_state->roll, motion_platform_state->pitch, motion_platform_state->yaw, motion_platform_state->vel_x, motion_platform_state->vel_y, motion_platform_state->vel_z, motion_platform_state->vel_roll, motion_platform_state->vel_pitch, motion_platform_state->vel_yaw, motion_platform_state->acc_x, motion_platform_state->acc_y, motion_platform_state->acc_z, motion_platform_state->acc_roll, motion_platform_state->acc_pitch, motion_platform_state->acc_yaw);
}

/**
 * @brief Encode a motion_platform_state struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param motion_platform_state C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_motion_platform_state_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_motion_platform_state_t* motion_platform_state)
{
    return mavlink_msg_motion_platform_state_pack_chan(system_id, component_id, chan, msg, motion_platform_state->time_boot_ms, motion_platform_state->health, motion_platform_state->mode, motion_platform_state->x, motion_platform_state->y, motion_platform_state->z, motion_platform_state->roll, motion_platform_state->pitch, motion_platform_state->yaw, motion_platform_state->vel_x, motion_platform_state->vel_y, motion_platform_state->vel_z, motion_platform_state->vel_roll, motion_platform_state->vel_pitch, motion_platform_state->vel_yaw, motion_platform_state->acc_x, motion_platform_state->acc_y, motion_platform_state->acc_z, motion_platform_state->acc_roll, motion_platform_state->acc_pitch, motion_platform_state->acc_yaw);
}

/**
 * @brief Encode a motion_platform_state struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param motion_platform_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_motion_platform_state_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_motion_platform_state_t* motion_platform_state)
{
    return mavlink_msg_motion_platform_state_pack_status(system_id, component_id, _status, msg,  motion_platform_state->time_boot_ms, motion_platform_state->health, motion_platform_state->mode, motion_platform_state->x, motion_platform_state->y, motion_platform_state->z, motion_platform_state->roll, motion_platform_state->pitch, motion_platform_state->yaw, motion_platform_state->vel_x, motion_platform_state->vel_y, motion_platform_state->vel_z, motion_platform_state->vel_roll, motion_platform_state->vel_pitch, motion_platform_state->vel_yaw, motion_platform_state->acc_x, motion_platform_state->acc_y, motion_platform_state->acc_z, motion_platform_state->acc_roll, motion_platform_state->acc_pitch, motion_platform_state->acc_yaw);
}

/**
 * @brief Send a motion_platform_state message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param health  Generic system health (error and warning) status.
 * @param mode  Generic system operating mode.
 * @param x [m] X axis (surge) position, positive forward.
 * @param y [m] Y axis (sway) position, positive right.
 * @param z [m] Z axis (heave) position, positive down.
 * @param roll [rad] Roll position, positive right.
 * @param pitch [rad] Pitch position, positive nose up.
 * @param yaw [rad] Yaw position, positive right.
 * @param vel_x [m/s] X axis (surge) velocity, positive forward.
 * @param vel_y [m/s] Y axis (sway) velocity, positive right.
 * @param vel_z [m/s] Z axis (heave) velocity, positive down.
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @param acc_roll  Roll acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_pitch  Pitch acceleration, positive nose up. Unit rad/s/s, currently not part of mavschema.xsd
 * @param acc_yaw  Yaw acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_motion_platform_state_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint8_t health, uint8_t mode, float x, float y, float z, float roll, float pitch, float yaw, float vel_x, float vel_y, float vel_z, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z, float acc_roll, float acc_pitch, float acc_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, x);
    _mav_put_float(buf, 8, y);
    _mav_put_float(buf, 12, z);
    _mav_put_float(buf, 16, roll);
    _mav_put_float(buf, 20, pitch);
    _mav_put_float(buf, 24, yaw);
    _mav_put_float(buf, 28, vel_x);
    _mav_put_float(buf, 32, vel_y);
    _mav_put_float(buf, 36, vel_z);
    _mav_put_float(buf, 40, vel_roll);
    _mav_put_float(buf, 44, vel_pitch);
    _mav_put_float(buf, 48, vel_yaw);
    _mav_put_float(buf, 52, acc_x);
    _mav_put_float(buf, 56, acc_y);
    _mav_put_float(buf, 60, acc_z);
    _mav_put_float(buf, 64, acc_roll);
    _mav_put_float(buf, 68, acc_pitch);
    _mav_put_float(buf, 72, acc_yaw);
    _mav_put_uint8_t(buf, 76, health);
    _mav_put_uint8_t(buf, 77, mode);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE, buf, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
#else
    mavlink_motion_platform_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.roll = roll;
    packet.pitch = pitch;
    packet.yaw = yaw;
    packet.vel_x = vel_x;
    packet.vel_y = vel_y;
    packet.vel_z = vel_z;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;
    packet.acc_roll = acc_roll;
    packet.acc_pitch = acc_pitch;
    packet.acc_yaw = acc_yaw;
    packet.health = health;
    packet.mode = mode;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE, (const char *)&packet, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
#endif
}

/**
 * @brief Send a motion_platform_state message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_motion_platform_state_send_struct(mavlink_channel_t chan, const mavlink_motion_platform_state_t* motion_platform_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_motion_platform_state_send(chan, motion_platform_state->time_boot_ms, motion_platform_state->health, motion_platform_state->mode, motion_platform_state->x, motion_platform_state->y, motion_platform_state->z, motion_platform_state->roll, motion_platform_state->pitch, motion_platform_state->yaw, motion_platform_state->vel_x, motion_platform_state->vel_y, motion_platform_state->vel_z, motion_platform_state->vel_roll, motion_platform_state->vel_pitch, motion_platform_state->vel_yaw, motion_platform_state->acc_x, motion_platform_state->acc_y, motion_platform_state->acc_z, motion_platform_state->acc_roll, motion_platform_state->acc_pitch, motion_platform_state->acc_yaw);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE, (const char *)motion_platform_state, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
#endif
}

#if MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_motion_platform_state_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint8_t health, uint8_t mode, float x, float y, float z, float roll, float pitch, float yaw, float vel_x, float vel_y, float vel_z, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z, float acc_roll, float acc_pitch, float acc_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, x);
    _mav_put_float(buf, 8, y);
    _mav_put_float(buf, 12, z);
    _mav_put_float(buf, 16, roll);
    _mav_put_float(buf, 20, pitch);
    _mav_put_float(buf, 24, yaw);
    _mav_put_float(buf, 28, vel_x);
    _mav_put_float(buf, 32, vel_y);
    _mav_put_float(buf, 36, vel_z);
    _mav_put_float(buf, 40, vel_roll);
    _mav_put_float(buf, 44, vel_pitch);
    _mav_put_float(buf, 48, vel_yaw);
    _mav_put_float(buf, 52, acc_x);
    _mav_put_float(buf, 56, acc_y);
    _mav_put_float(buf, 60, acc_z);
    _mav_put_float(buf, 64, acc_roll);
    _mav_put_float(buf, 68, acc_pitch);
    _mav_put_float(buf, 72, acc_yaw);
    _mav_put_uint8_t(buf, 76, health);
    _mav_put_uint8_t(buf, 77, mode);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE, buf, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
#else
    mavlink_motion_platform_state_t *packet = (mavlink_motion_platform_state_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->x = x;
    packet->y = y;
    packet->z = z;
    packet->roll = roll;
    packet->pitch = pitch;
    packet->yaw = yaw;
    packet->vel_x = vel_x;
    packet->vel_y = vel_y;
    packet->vel_z = vel_z;
    packet->vel_roll = vel_roll;
    packet->vel_pitch = vel_pitch;
    packet->vel_yaw = vel_yaw;
    packet->acc_x = acc_x;
    packet->acc_y = acc_y;
    packet->acc_z = acc_z;
    packet->acc_roll = acc_roll;
    packet->acc_pitch = acc_pitch;
    packet->acc_yaw = acc_yaw;
    packet->health = health;
    packet->mode = mode;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE, (const char *)packet, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_CRC);
#endif
}
#endif

#endif

// MESSAGE MOTION_PLATFORM_STATE UNPACKING


/**
 * @brief Get field time_boot_ms from motion_platform_state message
 *
 * @return [ms] Timestamp (time since system boot).
 */
MAVLINK_WIP
static inline uint32_t mavlink_msg_motion_platform_state_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field health from motion_platform_state message
 *
 * @return  Generic system health (error and warning) status.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_motion_platform_state_get_health(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  76);
}

/**
 * @brief Get field mode from motion_platform_state message
 *
 * @return  Generic system operating mode.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_motion_platform_state_get_mode(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  77);
}

/**
 * @brief Get field x from motion_platform_state message
 *
 * @return [m] X axis (surge) position, positive forward.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field y from motion_platform_state message
 *
 * @return [m] Y axis (sway) position, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field z from motion_platform_state message
 *
 * @return [m] Z axis (heave) position, positive down.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field roll from motion_platform_state message
 *
 * @return [rad] Roll position, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field pitch from motion_platform_state message
 *
 * @return [rad] Pitch position, positive nose up.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  20);
}

/**
 * @brief Get field yaw from motion_platform_state message
 *
 * @return [rad] Yaw position, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Get field vel_x from motion_platform_state message
 *
 * @return [m/s] X axis (surge) velocity, positive forward.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_vel_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Get field vel_y from motion_platform_state message
 *
 * @return [m/s] Y axis (sway) velocity, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_vel_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field vel_z from motion_platform_state message
 *
 * @return [m/s] Z axis (heave) velocity, positive down.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_vel_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  36);
}

/**
 * @brief Get field vel_roll from motion_platform_state message
 *
 * @return [rad/s] Roll velocity, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_vel_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  40);
}

/**
 * @brief Get field vel_pitch from motion_platform_state message
 *
 * @return [rad/s] Pitch velocity, positive nose up.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_vel_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  44);
}

/**
 * @brief Get field vel_yaw from motion_platform_state message
 *
 * @return [rad/s] Yaw velocity, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_vel_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  48);
}

/**
 * @brief Get field acc_x from motion_platform_state message
 *
 * @return [m/s/s] X axis (surge) acceleration, positive forward.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_acc_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  52);
}

/**
 * @brief Get field acc_y from motion_platform_state message
 *
 * @return [m/s/s] Y axis (sway) acceleration, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_acc_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  56);
}

/**
 * @brief Get field acc_z from motion_platform_state message
 *
 * @return [m/s/s] Z axis (heave) acceleration, positive down.
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_acc_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  60);
}

/**
 * @brief Get field acc_roll from motion_platform_state message
 *
 * @return  Roll acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_acc_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  64);
}

/**
 * @brief Get field acc_pitch from motion_platform_state message
 *
 * @return  Pitch acceleration, positive nose up. Unit rad/s/s, currently not part of mavschema.xsd
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_acc_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  68);
}

/**
 * @brief Get field acc_yaw from motion_platform_state message
 *
 * @return  Yaw acceleration, positive right. Unit rad/s/s, currently not part of mavschema.xsd
 */
MAVLINK_WIP
static inline float mavlink_msg_motion_platform_state_get_acc_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  72);
}

/**
 * @brief Decode a motion_platform_state message into a struct
 *
 * @param msg The message to decode
 * @param motion_platform_state C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_motion_platform_state_decode(const mavlink_message_t* msg, mavlink_motion_platform_state_t* motion_platform_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    motion_platform_state->time_boot_ms = mavlink_msg_motion_platform_state_get_time_boot_ms(msg);
    motion_platform_state->x = mavlink_msg_motion_platform_state_get_x(msg);
    motion_platform_state->y = mavlink_msg_motion_platform_state_get_y(msg);
    motion_platform_state->z = mavlink_msg_motion_platform_state_get_z(msg);
    motion_platform_state->roll = mavlink_msg_motion_platform_state_get_roll(msg);
    motion_platform_state->pitch = mavlink_msg_motion_platform_state_get_pitch(msg);
    motion_platform_state->yaw = mavlink_msg_motion_platform_state_get_yaw(msg);
    motion_platform_state->vel_x = mavlink_msg_motion_platform_state_get_vel_x(msg);
    motion_platform_state->vel_y = mavlink_msg_motion_platform_state_get_vel_y(msg);
    motion_platform_state->vel_z = mavlink_msg_motion_platform_state_get_vel_z(msg);
    motion_platform_state->vel_roll = mavlink_msg_motion_platform_state_get_vel_roll(msg);
    motion_platform_state->vel_pitch = mavlink_msg_motion_platform_state_get_vel_pitch(msg);
    motion_platform_state->vel_yaw = mavlink_msg_motion_platform_state_get_vel_yaw(msg);
    motion_platform_state->acc_x = mavlink_msg_motion_platform_state_get_acc_x(msg);
    motion_platform_state->acc_y = mavlink_msg_motion_platform_state_get_acc_y(msg);
    motion_platform_state->acc_z = mavlink_msg_motion_platform_state_get_acc_z(msg);
    motion_platform_state->acc_roll = mavlink_msg_motion_platform_state_get_acc_roll(msg);
    motion_platform_state->acc_pitch = mavlink_msg_motion_platform_state_get_acc_pitch(msg);
    motion_platform_state->acc_yaw = mavlink_msg_motion_platform_state_get_acc_yaw(msg);
    motion_platform_state->health = mavlink_msg_motion_platform_state_get_health(msg);
    motion_platform_state->mode = mavlink_msg_motion_platform_state_get_mode(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN? msg->len : MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN;
        memset(motion_platform_state, 0, MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_LEN);
    memcpy(motion_platform_state, _MAV_PAYLOAD(msg), len);
#endif
}
