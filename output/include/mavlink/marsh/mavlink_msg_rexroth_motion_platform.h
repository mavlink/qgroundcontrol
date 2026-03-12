#pragma once
// MESSAGE REXROTH_MOTION_PLATFORM PACKING

#define MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM 52503


typedef struct __mavlink_rexroth_motion_platform_t {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 uint32_t frame_count; /*<  Number of message as sent by the Motion System.*/
 uint32_t motion_status; /*<  Motion Status variable as sent by the system.*/
 float actuator1; /*< [m] Current actuator 1 position.*/
 float actuator2; /*< [m] Current actuator 2 position.*/
 float actuator3; /*< [m] Current actuator 3 position.*/
 float actuator4; /*< [m] Current actuator 4 position.*/
 float actuator5; /*< [m] Current actuator 5 position.*/
 float actuator6; /*< [m] Current actuator 6 position.*/
 float platform_setpoint_x; /*< [m] X axis (surge) platform setpoint, positive forward.*/
 float platform_setpoint_y; /*< [m] Y axis (sway) platform setpoint, positive right.*/
 float platform_setpoint_z; /*< [m] Z axis (heave) platform setpoint, positive down.*/
 float platform_setpoint_roll; /*< [rad] Roll platform setpoint, positive right.*/
 float platform_setpoint_pitch; /*< [rad] Pitch platform setpoint, positive nose up.*/
 float platform_setpoint_yaw; /*< [rad] Yaw platform setpoint, positive right.*/
 float effect_setpoint_x; /*< [m] X axis (surge) special effect setpoint, positive forward.*/
 float effect_setpoint_y; /*< [m] Y axis (sway) special effect setpoint, positive right.*/
 float effect_setpoint_z; /*< [m] Z axis (heave) special effect setpoint, positive down.*/
 float effect_setpoint_roll; /*< [rad] Roll special effect setpoint, positive right.*/
 float effect_setpoint_pitch; /*< [rad] Pitch special effect setpoint, positive nose up.*/
 float effect_setpoint_yaw; /*< [rad] Yaw special effect setpoint, positive right.*/
 uint8_t error_code; /*<  Error code extracted from motion status.*/
} mavlink_rexroth_motion_platform_t;

#define MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN 85
#define MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN 85
#define MAVLINK_MSG_ID_52503_LEN 85
#define MAVLINK_MSG_ID_52503_MIN_LEN 85

#define MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC 96
#define MAVLINK_MSG_ID_52503_CRC 96



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_REXROTH_MOTION_PLATFORM { \
    52503, \
    "REXROTH_MOTION_PLATFORM", \
    22, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_rexroth_motion_platform_t, time_boot_ms) }, \
         { "frame_count", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_rexroth_motion_platform_t, frame_count) }, \
         { "motion_status", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_rexroth_motion_platform_t, motion_status) }, \
         { "error_code", NULL, MAVLINK_TYPE_UINT8_T, 0, 84, offsetof(mavlink_rexroth_motion_platform_t, error_code) }, \
         { "actuator1", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_rexroth_motion_platform_t, actuator1) }, \
         { "actuator2", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_rexroth_motion_platform_t, actuator2) }, \
         { "actuator3", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_rexroth_motion_platform_t, actuator3) }, \
         { "actuator4", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_rexroth_motion_platform_t, actuator4) }, \
         { "actuator5", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_rexroth_motion_platform_t, actuator5) }, \
         { "actuator6", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_rexroth_motion_platform_t, actuator6) }, \
         { "platform_setpoint_x", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_x) }, \
         { "platform_setpoint_y", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_y) }, \
         { "platform_setpoint_z", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_z) }, \
         { "platform_setpoint_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 48, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_roll) }, \
         { "platform_setpoint_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 52, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_pitch) }, \
         { "platform_setpoint_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 56, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_yaw) }, \
         { "effect_setpoint_x", NULL, MAVLINK_TYPE_FLOAT, 0, 60, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_x) }, \
         { "effect_setpoint_y", NULL, MAVLINK_TYPE_FLOAT, 0, 64, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_y) }, \
         { "effect_setpoint_z", NULL, MAVLINK_TYPE_FLOAT, 0, 68, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_z) }, \
         { "effect_setpoint_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 72, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_roll) }, \
         { "effect_setpoint_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 76, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_pitch) }, \
         { "effect_setpoint_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 80, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_yaw) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_REXROTH_MOTION_PLATFORM { \
    "REXROTH_MOTION_PLATFORM", \
    22, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_rexroth_motion_platform_t, time_boot_ms) }, \
         { "frame_count", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_rexroth_motion_platform_t, frame_count) }, \
         { "motion_status", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_rexroth_motion_platform_t, motion_status) }, \
         { "error_code", NULL, MAVLINK_TYPE_UINT8_T, 0, 84, offsetof(mavlink_rexroth_motion_platform_t, error_code) }, \
         { "actuator1", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_rexroth_motion_platform_t, actuator1) }, \
         { "actuator2", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_rexroth_motion_platform_t, actuator2) }, \
         { "actuator3", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_rexroth_motion_platform_t, actuator3) }, \
         { "actuator4", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_rexroth_motion_platform_t, actuator4) }, \
         { "actuator5", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_rexroth_motion_platform_t, actuator5) }, \
         { "actuator6", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_rexroth_motion_platform_t, actuator6) }, \
         { "platform_setpoint_x", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_x) }, \
         { "platform_setpoint_y", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_y) }, \
         { "platform_setpoint_z", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_z) }, \
         { "platform_setpoint_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 48, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_roll) }, \
         { "platform_setpoint_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 52, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_pitch) }, \
         { "platform_setpoint_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 56, offsetof(mavlink_rexroth_motion_platform_t, platform_setpoint_yaw) }, \
         { "effect_setpoint_x", NULL, MAVLINK_TYPE_FLOAT, 0, 60, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_x) }, \
         { "effect_setpoint_y", NULL, MAVLINK_TYPE_FLOAT, 0, 64, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_y) }, \
         { "effect_setpoint_z", NULL, MAVLINK_TYPE_FLOAT, 0, 68, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_z) }, \
         { "effect_setpoint_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 72, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_roll) }, \
         { "effect_setpoint_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 76, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_pitch) }, \
         { "effect_setpoint_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 80, offsetof(mavlink_rexroth_motion_platform_t, effect_setpoint_yaw) }, \
         } \
}
#endif

/**
 * @brief Pack a rexroth_motion_platform message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param frame_count  Number of message as sent by the Motion System.
 * @param motion_status  Motion Status variable as sent by the system.
 * @param error_code  Error code extracted from motion status.
 * @param actuator1 [m] Current actuator 1 position.
 * @param actuator2 [m] Current actuator 2 position.
 * @param actuator3 [m] Current actuator 3 position.
 * @param actuator4 [m] Current actuator 4 position.
 * @param actuator5 [m] Current actuator 5 position.
 * @param actuator6 [m] Current actuator 6 position.
 * @param platform_setpoint_x [m] X axis (surge) platform setpoint, positive forward.
 * @param platform_setpoint_y [m] Y axis (sway) platform setpoint, positive right.
 * @param platform_setpoint_z [m] Z axis (heave) platform setpoint, positive down.
 * @param platform_setpoint_roll [rad] Roll platform setpoint, positive right.
 * @param platform_setpoint_pitch [rad] Pitch platform setpoint, positive nose up.
 * @param platform_setpoint_yaw [rad] Yaw platform setpoint, positive right.
 * @param effect_setpoint_x [m] X axis (surge) special effect setpoint, positive forward.
 * @param effect_setpoint_y [m] Y axis (sway) special effect setpoint, positive right.
 * @param effect_setpoint_z [m] Z axis (heave) special effect setpoint, positive down.
 * @param effect_setpoint_roll [rad] Roll special effect setpoint, positive right.
 * @param effect_setpoint_pitch [rad] Pitch special effect setpoint, positive nose up.
 * @param effect_setpoint_yaw [rad] Yaw special effect setpoint, positive right.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_rexroth_motion_platform_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t frame_count, uint32_t motion_status, uint8_t error_code, float actuator1, float actuator2, float actuator3, float actuator4, float actuator5, float actuator6, float platform_setpoint_x, float platform_setpoint_y, float platform_setpoint_z, float platform_setpoint_roll, float platform_setpoint_pitch, float platform_setpoint_yaw, float effect_setpoint_x, float effect_setpoint_y, float effect_setpoint_z, float effect_setpoint_roll, float effect_setpoint_pitch, float effect_setpoint_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, frame_count);
    _mav_put_uint32_t(buf, 8, motion_status);
    _mav_put_float(buf, 12, actuator1);
    _mav_put_float(buf, 16, actuator2);
    _mav_put_float(buf, 20, actuator3);
    _mav_put_float(buf, 24, actuator4);
    _mav_put_float(buf, 28, actuator5);
    _mav_put_float(buf, 32, actuator6);
    _mav_put_float(buf, 36, platform_setpoint_x);
    _mav_put_float(buf, 40, platform_setpoint_y);
    _mav_put_float(buf, 44, platform_setpoint_z);
    _mav_put_float(buf, 48, platform_setpoint_roll);
    _mav_put_float(buf, 52, platform_setpoint_pitch);
    _mav_put_float(buf, 56, platform_setpoint_yaw);
    _mav_put_float(buf, 60, effect_setpoint_x);
    _mav_put_float(buf, 64, effect_setpoint_y);
    _mav_put_float(buf, 68, effect_setpoint_z);
    _mav_put_float(buf, 72, effect_setpoint_roll);
    _mav_put_float(buf, 76, effect_setpoint_pitch);
    _mav_put_float(buf, 80, effect_setpoint_yaw);
    _mav_put_uint8_t(buf, 84, error_code);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#else
    mavlink_rexroth_motion_platform_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.frame_count = frame_count;
    packet.motion_status = motion_status;
    packet.actuator1 = actuator1;
    packet.actuator2 = actuator2;
    packet.actuator3 = actuator3;
    packet.actuator4 = actuator4;
    packet.actuator5 = actuator5;
    packet.actuator6 = actuator6;
    packet.platform_setpoint_x = platform_setpoint_x;
    packet.platform_setpoint_y = platform_setpoint_y;
    packet.platform_setpoint_z = platform_setpoint_z;
    packet.platform_setpoint_roll = platform_setpoint_roll;
    packet.platform_setpoint_pitch = platform_setpoint_pitch;
    packet.platform_setpoint_yaw = platform_setpoint_yaw;
    packet.effect_setpoint_x = effect_setpoint_x;
    packet.effect_setpoint_y = effect_setpoint_y;
    packet.effect_setpoint_z = effect_setpoint_z;
    packet.effect_setpoint_roll = effect_setpoint_roll;
    packet.effect_setpoint_pitch = effect_setpoint_pitch;
    packet.effect_setpoint_yaw = effect_setpoint_yaw;
    packet.error_code = error_code;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
}

/**
 * @brief Pack a rexroth_motion_platform message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param frame_count  Number of message as sent by the Motion System.
 * @param motion_status  Motion Status variable as sent by the system.
 * @param error_code  Error code extracted from motion status.
 * @param actuator1 [m] Current actuator 1 position.
 * @param actuator2 [m] Current actuator 2 position.
 * @param actuator3 [m] Current actuator 3 position.
 * @param actuator4 [m] Current actuator 4 position.
 * @param actuator5 [m] Current actuator 5 position.
 * @param actuator6 [m] Current actuator 6 position.
 * @param platform_setpoint_x [m] X axis (surge) platform setpoint, positive forward.
 * @param platform_setpoint_y [m] Y axis (sway) platform setpoint, positive right.
 * @param platform_setpoint_z [m] Z axis (heave) platform setpoint, positive down.
 * @param platform_setpoint_roll [rad] Roll platform setpoint, positive right.
 * @param platform_setpoint_pitch [rad] Pitch platform setpoint, positive nose up.
 * @param platform_setpoint_yaw [rad] Yaw platform setpoint, positive right.
 * @param effect_setpoint_x [m] X axis (surge) special effect setpoint, positive forward.
 * @param effect_setpoint_y [m] Y axis (sway) special effect setpoint, positive right.
 * @param effect_setpoint_z [m] Z axis (heave) special effect setpoint, positive down.
 * @param effect_setpoint_roll [rad] Roll special effect setpoint, positive right.
 * @param effect_setpoint_pitch [rad] Pitch special effect setpoint, positive nose up.
 * @param effect_setpoint_yaw [rad] Yaw special effect setpoint, positive right.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rexroth_motion_platform_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t frame_count, uint32_t motion_status, uint8_t error_code, float actuator1, float actuator2, float actuator3, float actuator4, float actuator5, float actuator6, float platform_setpoint_x, float platform_setpoint_y, float platform_setpoint_z, float platform_setpoint_roll, float platform_setpoint_pitch, float platform_setpoint_yaw, float effect_setpoint_x, float effect_setpoint_y, float effect_setpoint_z, float effect_setpoint_roll, float effect_setpoint_pitch, float effect_setpoint_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, frame_count);
    _mav_put_uint32_t(buf, 8, motion_status);
    _mav_put_float(buf, 12, actuator1);
    _mav_put_float(buf, 16, actuator2);
    _mav_put_float(buf, 20, actuator3);
    _mav_put_float(buf, 24, actuator4);
    _mav_put_float(buf, 28, actuator5);
    _mav_put_float(buf, 32, actuator6);
    _mav_put_float(buf, 36, platform_setpoint_x);
    _mav_put_float(buf, 40, platform_setpoint_y);
    _mav_put_float(buf, 44, platform_setpoint_z);
    _mav_put_float(buf, 48, platform_setpoint_roll);
    _mav_put_float(buf, 52, platform_setpoint_pitch);
    _mav_put_float(buf, 56, platform_setpoint_yaw);
    _mav_put_float(buf, 60, effect_setpoint_x);
    _mav_put_float(buf, 64, effect_setpoint_y);
    _mav_put_float(buf, 68, effect_setpoint_z);
    _mav_put_float(buf, 72, effect_setpoint_roll);
    _mav_put_float(buf, 76, effect_setpoint_pitch);
    _mav_put_float(buf, 80, effect_setpoint_yaw);
    _mav_put_uint8_t(buf, 84, error_code);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#else
    mavlink_rexroth_motion_platform_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.frame_count = frame_count;
    packet.motion_status = motion_status;
    packet.actuator1 = actuator1;
    packet.actuator2 = actuator2;
    packet.actuator3 = actuator3;
    packet.actuator4 = actuator4;
    packet.actuator5 = actuator5;
    packet.actuator6 = actuator6;
    packet.platform_setpoint_x = platform_setpoint_x;
    packet.platform_setpoint_y = platform_setpoint_y;
    packet.platform_setpoint_z = platform_setpoint_z;
    packet.platform_setpoint_roll = platform_setpoint_roll;
    packet.platform_setpoint_pitch = platform_setpoint_pitch;
    packet.platform_setpoint_yaw = platform_setpoint_yaw;
    packet.effect_setpoint_x = effect_setpoint_x;
    packet.effect_setpoint_y = effect_setpoint_y;
    packet.effect_setpoint_z = effect_setpoint_z;
    packet.effect_setpoint_roll = effect_setpoint_roll;
    packet.effect_setpoint_pitch = effect_setpoint_pitch;
    packet.effect_setpoint_yaw = effect_setpoint_yaw;
    packet.error_code = error_code;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#endif
}

/**
 * @brief Pack a rexroth_motion_platform message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param frame_count  Number of message as sent by the Motion System.
 * @param motion_status  Motion Status variable as sent by the system.
 * @param error_code  Error code extracted from motion status.
 * @param actuator1 [m] Current actuator 1 position.
 * @param actuator2 [m] Current actuator 2 position.
 * @param actuator3 [m] Current actuator 3 position.
 * @param actuator4 [m] Current actuator 4 position.
 * @param actuator5 [m] Current actuator 5 position.
 * @param actuator6 [m] Current actuator 6 position.
 * @param platform_setpoint_x [m] X axis (surge) platform setpoint, positive forward.
 * @param platform_setpoint_y [m] Y axis (sway) platform setpoint, positive right.
 * @param platform_setpoint_z [m] Z axis (heave) platform setpoint, positive down.
 * @param platform_setpoint_roll [rad] Roll platform setpoint, positive right.
 * @param platform_setpoint_pitch [rad] Pitch platform setpoint, positive nose up.
 * @param platform_setpoint_yaw [rad] Yaw platform setpoint, positive right.
 * @param effect_setpoint_x [m] X axis (surge) special effect setpoint, positive forward.
 * @param effect_setpoint_y [m] Y axis (sway) special effect setpoint, positive right.
 * @param effect_setpoint_z [m] Z axis (heave) special effect setpoint, positive down.
 * @param effect_setpoint_roll [rad] Roll special effect setpoint, positive right.
 * @param effect_setpoint_pitch [rad] Pitch special effect setpoint, positive nose up.
 * @param effect_setpoint_yaw [rad] Yaw special effect setpoint, positive right.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_rexroth_motion_platform_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint32_t frame_count,uint32_t motion_status,uint8_t error_code,float actuator1,float actuator2,float actuator3,float actuator4,float actuator5,float actuator6,float platform_setpoint_x,float platform_setpoint_y,float platform_setpoint_z,float platform_setpoint_roll,float platform_setpoint_pitch,float platform_setpoint_yaw,float effect_setpoint_x,float effect_setpoint_y,float effect_setpoint_z,float effect_setpoint_roll,float effect_setpoint_pitch,float effect_setpoint_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, frame_count);
    _mav_put_uint32_t(buf, 8, motion_status);
    _mav_put_float(buf, 12, actuator1);
    _mav_put_float(buf, 16, actuator2);
    _mav_put_float(buf, 20, actuator3);
    _mav_put_float(buf, 24, actuator4);
    _mav_put_float(buf, 28, actuator5);
    _mav_put_float(buf, 32, actuator6);
    _mav_put_float(buf, 36, platform_setpoint_x);
    _mav_put_float(buf, 40, platform_setpoint_y);
    _mav_put_float(buf, 44, platform_setpoint_z);
    _mav_put_float(buf, 48, platform_setpoint_roll);
    _mav_put_float(buf, 52, platform_setpoint_pitch);
    _mav_put_float(buf, 56, platform_setpoint_yaw);
    _mav_put_float(buf, 60, effect_setpoint_x);
    _mav_put_float(buf, 64, effect_setpoint_y);
    _mav_put_float(buf, 68, effect_setpoint_z);
    _mav_put_float(buf, 72, effect_setpoint_roll);
    _mav_put_float(buf, 76, effect_setpoint_pitch);
    _mav_put_float(buf, 80, effect_setpoint_yaw);
    _mav_put_uint8_t(buf, 84, error_code);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#else
    mavlink_rexroth_motion_platform_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.frame_count = frame_count;
    packet.motion_status = motion_status;
    packet.actuator1 = actuator1;
    packet.actuator2 = actuator2;
    packet.actuator3 = actuator3;
    packet.actuator4 = actuator4;
    packet.actuator5 = actuator5;
    packet.actuator6 = actuator6;
    packet.platform_setpoint_x = platform_setpoint_x;
    packet.platform_setpoint_y = platform_setpoint_y;
    packet.platform_setpoint_z = platform_setpoint_z;
    packet.platform_setpoint_roll = platform_setpoint_roll;
    packet.platform_setpoint_pitch = platform_setpoint_pitch;
    packet.platform_setpoint_yaw = platform_setpoint_yaw;
    packet.effect_setpoint_x = effect_setpoint_x;
    packet.effect_setpoint_y = effect_setpoint_y;
    packet.effect_setpoint_z = effect_setpoint_z;
    packet.effect_setpoint_roll = effect_setpoint_roll;
    packet.effect_setpoint_pitch = effect_setpoint_pitch;
    packet.effect_setpoint_yaw = effect_setpoint_yaw;
    packet.error_code = error_code;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
}

/**
 * @brief Encode a rexroth_motion_platform struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rexroth_motion_platform C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_rexroth_motion_platform_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rexroth_motion_platform_t* rexroth_motion_platform)
{
    return mavlink_msg_rexroth_motion_platform_pack(system_id, component_id, msg, rexroth_motion_platform->time_boot_ms, rexroth_motion_platform->frame_count, rexroth_motion_platform->motion_status, rexroth_motion_platform->error_code, rexroth_motion_platform->actuator1, rexroth_motion_platform->actuator2, rexroth_motion_platform->actuator3, rexroth_motion_platform->actuator4, rexroth_motion_platform->actuator5, rexroth_motion_platform->actuator6, rexroth_motion_platform->platform_setpoint_x, rexroth_motion_platform->platform_setpoint_y, rexroth_motion_platform->platform_setpoint_z, rexroth_motion_platform->platform_setpoint_roll, rexroth_motion_platform->platform_setpoint_pitch, rexroth_motion_platform->platform_setpoint_yaw, rexroth_motion_platform->effect_setpoint_x, rexroth_motion_platform->effect_setpoint_y, rexroth_motion_platform->effect_setpoint_z, rexroth_motion_platform->effect_setpoint_roll, rexroth_motion_platform->effect_setpoint_pitch, rexroth_motion_platform->effect_setpoint_yaw);
}

/**
 * @brief Encode a rexroth_motion_platform struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rexroth_motion_platform C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_rexroth_motion_platform_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rexroth_motion_platform_t* rexroth_motion_platform)
{
    return mavlink_msg_rexroth_motion_platform_pack_chan(system_id, component_id, chan, msg, rexroth_motion_platform->time_boot_ms, rexroth_motion_platform->frame_count, rexroth_motion_platform->motion_status, rexroth_motion_platform->error_code, rexroth_motion_platform->actuator1, rexroth_motion_platform->actuator2, rexroth_motion_platform->actuator3, rexroth_motion_platform->actuator4, rexroth_motion_platform->actuator5, rexroth_motion_platform->actuator6, rexroth_motion_platform->platform_setpoint_x, rexroth_motion_platform->platform_setpoint_y, rexroth_motion_platform->platform_setpoint_z, rexroth_motion_platform->platform_setpoint_roll, rexroth_motion_platform->platform_setpoint_pitch, rexroth_motion_platform->platform_setpoint_yaw, rexroth_motion_platform->effect_setpoint_x, rexroth_motion_platform->effect_setpoint_y, rexroth_motion_platform->effect_setpoint_z, rexroth_motion_platform->effect_setpoint_roll, rexroth_motion_platform->effect_setpoint_pitch, rexroth_motion_platform->effect_setpoint_yaw);
}

/**
 * @brief Encode a rexroth_motion_platform struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rexroth_motion_platform C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rexroth_motion_platform_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rexroth_motion_platform_t* rexroth_motion_platform)
{
    return mavlink_msg_rexroth_motion_platform_pack_status(system_id, component_id, _status, msg,  rexroth_motion_platform->time_boot_ms, rexroth_motion_platform->frame_count, rexroth_motion_platform->motion_status, rexroth_motion_platform->error_code, rexroth_motion_platform->actuator1, rexroth_motion_platform->actuator2, rexroth_motion_platform->actuator3, rexroth_motion_platform->actuator4, rexroth_motion_platform->actuator5, rexroth_motion_platform->actuator6, rexroth_motion_platform->platform_setpoint_x, rexroth_motion_platform->platform_setpoint_y, rexroth_motion_platform->platform_setpoint_z, rexroth_motion_platform->platform_setpoint_roll, rexroth_motion_platform->platform_setpoint_pitch, rexroth_motion_platform->platform_setpoint_yaw, rexroth_motion_platform->effect_setpoint_x, rexroth_motion_platform->effect_setpoint_y, rexroth_motion_platform->effect_setpoint_z, rexroth_motion_platform->effect_setpoint_roll, rexroth_motion_platform->effect_setpoint_pitch, rexroth_motion_platform->effect_setpoint_yaw);
}

/**
 * @brief Send a rexroth_motion_platform message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param frame_count  Number of message as sent by the Motion System.
 * @param motion_status  Motion Status variable as sent by the system.
 * @param error_code  Error code extracted from motion status.
 * @param actuator1 [m] Current actuator 1 position.
 * @param actuator2 [m] Current actuator 2 position.
 * @param actuator3 [m] Current actuator 3 position.
 * @param actuator4 [m] Current actuator 4 position.
 * @param actuator5 [m] Current actuator 5 position.
 * @param actuator6 [m] Current actuator 6 position.
 * @param platform_setpoint_x [m] X axis (surge) platform setpoint, positive forward.
 * @param platform_setpoint_y [m] Y axis (sway) platform setpoint, positive right.
 * @param platform_setpoint_z [m] Z axis (heave) platform setpoint, positive down.
 * @param platform_setpoint_roll [rad] Roll platform setpoint, positive right.
 * @param platform_setpoint_pitch [rad] Pitch platform setpoint, positive nose up.
 * @param platform_setpoint_yaw [rad] Yaw platform setpoint, positive right.
 * @param effect_setpoint_x [m] X axis (surge) special effect setpoint, positive forward.
 * @param effect_setpoint_y [m] Y axis (sway) special effect setpoint, positive right.
 * @param effect_setpoint_z [m] Z axis (heave) special effect setpoint, positive down.
 * @param effect_setpoint_roll [rad] Roll special effect setpoint, positive right.
 * @param effect_setpoint_pitch [rad] Pitch special effect setpoint, positive nose up.
 * @param effect_setpoint_yaw [rad] Yaw special effect setpoint, positive right.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_rexroth_motion_platform_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint32_t frame_count, uint32_t motion_status, uint8_t error_code, float actuator1, float actuator2, float actuator3, float actuator4, float actuator5, float actuator6, float platform_setpoint_x, float platform_setpoint_y, float platform_setpoint_z, float platform_setpoint_roll, float platform_setpoint_pitch, float platform_setpoint_yaw, float effect_setpoint_x, float effect_setpoint_y, float effect_setpoint_z, float effect_setpoint_roll, float effect_setpoint_pitch, float effect_setpoint_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, frame_count);
    _mav_put_uint32_t(buf, 8, motion_status);
    _mav_put_float(buf, 12, actuator1);
    _mav_put_float(buf, 16, actuator2);
    _mav_put_float(buf, 20, actuator3);
    _mav_put_float(buf, 24, actuator4);
    _mav_put_float(buf, 28, actuator5);
    _mav_put_float(buf, 32, actuator6);
    _mav_put_float(buf, 36, platform_setpoint_x);
    _mav_put_float(buf, 40, platform_setpoint_y);
    _mav_put_float(buf, 44, platform_setpoint_z);
    _mav_put_float(buf, 48, platform_setpoint_roll);
    _mav_put_float(buf, 52, platform_setpoint_pitch);
    _mav_put_float(buf, 56, platform_setpoint_yaw);
    _mav_put_float(buf, 60, effect_setpoint_x);
    _mav_put_float(buf, 64, effect_setpoint_y);
    _mav_put_float(buf, 68, effect_setpoint_z);
    _mav_put_float(buf, 72, effect_setpoint_roll);
    _mav_put_float(buf, 76, effect_setpoint_pitch);
    _mav_put_float(buf, 80, effect_setpoint_yaw);
    _mav_put_uint8_t(buf, 84, error_code);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM, buf, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
#else
    mavlink_rexroth_motion_platform_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.frame_count = frame_count;
    packet.motion_status = motion_status;
    packet.actuator1 = actuator1;
    packet.actuator2 = actuator2;
    packet.actuator3 = actuator3;
    packet.actuator4 = actuator4;
    packet.actuator5 = actuator5;
    packet.actuator6 = actuator6;
    packet.platform_setpoint_x = platform_setpoint_x;
    packet.platform_setpoint_y = platform_setpoint_y;
    packet.platform_setpoint_z = platform_setpoint_z;
    packet.platform_setpoint_roll = platform_setpoint_roll;
    packet.platform_setpoint_pitch = platform_setpoint_pitch;
    packet.platform_setpoint_yaw = platform_setpoint_yaw;
    packet.effect_setpoint_x = effect_setpoint_x;
    packet.effect_setpoint_y = effect_setpoint_y;
    packet.effect_setpoint_z = effect_setpoint_z;
    packet.effect_setpoint_roll = effect_setpoint_roll;
    packet.effect_setpoint_pitch = effect_setpoint_pitch;
    packet.effect_setpoint_yaw = effect_setpoint_yaw;
    packet.error_code = error_code;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM, (const char *)&packet, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
#endif
}

/**
 * @brief Send a rexroth_motion_platform message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_rexroth_motion_platform_send_struct(mavlink_channel_t chan, const mavlink_rexroth_motion_platform_t* rexroth_motion_platform)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rexroth_motion_platform_send(chan, rexroth_motion_platform->time_boot_ms, rexroth_motion_platform->frame_count, rexroth_motion_platform->motion_status, rexroth_motion_platform->error_code, rexroth_motion_platform->actuator1, rexroth_motion_platform->actuator2, rexroth_motion_platform->actuator3, rexroth_motion_platform->actuator4, rexroth_motion_platform->actuator5, rexroth_motion_platform->actuator6, rexroth_motion_platform->platform_setpoint_x, rexroth_motion_platform->platform_setpoint_y, rexroth_motion_platform->platform_setpoint_z, rexroth_motion_platform->platform_setpoint_roll, rexroth_motion_platform->platform_setpoint_pitch, rexroth_motion_platform->platform_setpoint_yaw, rexroth_motion_platform->effect_setpoint_x, rexroth_motion_platform->effect_setpoint_y, rexroth_motion_platform->effect_setpoint_z, rexroth_motion_platform->effect_setpoint_roll, rexroth_motion_platform->effect_setpoint_pitch, rexroth_motion_platform->effect_setpoint_yaw);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM, (const char *)rexroth_motion_platform, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
#endif
}

#if MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_rexroth_motion_platform_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint32_t frame_count, uint32_t motion_status, uint8_t error_code, float actuator1, float actuator2, float actuator3, float actuator4, float actuator5, float actuator6, float platform_setpoint_x, float platform_setpoint_y, float platform_setpoint_z, float platform_setpoint_roll, float platform_setpoint_pitch, float platform_setpoint_yaw, float effect_setpoint_x, float effect_setpoint_y, float effect_setpoint_z, float effect_setpoint_roll, float effect_setpoint_pitch, float effect_setpoint_yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, frame_count);
    _mav_put_uint32_t(buf, 8, motion_status);
    _mav_put_float(buf, 12, actuator1);
    _mav_put_float(buf, 16, actuator2);
    _mav_put_float(buf, 20, actuator3);
    _mav_put_float(buf, 24, actuator4);
    _mav_put_float(buf, 28, actuator5);
    _mav_put_float(buf, 32, actuator6);
    _mav_put_float(buf, 36, platform_setpoint_x);
    _mav_put_float(buf, 40, platform_setpoint_y);
    _mav_put_float(buf, 44, platform_setpoint_z);
    _mav_put_float(buf, 48, platform_setpoint_roll);
    _mav_put_float(buf, 52, platform_setpoint_pitch);
    _mav_put_float(buf, 56, platform_setpoint_yaw);
    _mav_put_float(buf, 60, effect_setpoint_x);
    _mav_put_float(buf, 64, effect_setpoint_y);
    _mav_put_float(buf, 68, effect_setpoint_z);
    _mav_put_float(buf, 72, effect_setpoint_roll);
    _mav_put_float(buf, 76, effect_setpoint_pitch);
    _mav_put_float(buf, 80, effect_setpoint_yaw);
    _mav_put_uint8_t(buf, 84, error_code);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM, buf, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
#else
    mavlink_rexroth_motion_platform_t *packet = (mavlink_rexroth_motion_platform_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->frame_count = frame_count;
    packet->motion_status = motion_status;
    packet->actuator1 = actuator1;
    packet->actuator2 = actuator2;
    packet->actuator3 = actuator3;
    packet->actuator4 = actuator4;
    packet->actuator5 = actuator5;
    packet->actuator6 = actuator6;
    packet->platform_setpoint_x = platform_setpoint_x;
    packet->platform_setpoint_y = platform_setpoint_y;
    packet->platform_setpoint_z = platform_setpoint_z;
    packet->platform_setpoint_roll = platform_setpoint_roll;
    packet->platform_setpoint_pitch = platform_setpoint_pitch;
    packet->platform_setpoint_yaw = platform_setpoint_yaw;
    packet->effect_setpoint_x = effect_setpoint_x;
    packet->effect_setpoint_y = effect_setpoint_y;
    packet->effect_setpoint_z = effect_setpoint_z;
    packet->effect_setpoint_roll = effect_setpoint_roll;
    packet->effect_setpoint_pitch = effect_setpoint_pitch;
    packet->effect_setpoint_yaw = effect_setpoint_yaw;
    packet->error_code = error_code;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM, (const char *)packet, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_CRC);
#endif
}
#endif

#endif

// MESSAGE REXROTH_MOTION_PLATFORM UNPACKING


/**
 * @brief Get field time_boot_ms from rexroth_motion_platform message
 *
 * @return [ms] Timestamp (time since system boot).
 */
MAVLINK_WIP
static inline uint32_t mavlink_msg_rexroth_motion_platform_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field frame_count from rexroth_motion_platform message
 *
 * @return  Number of message as sent by the Motion System.
 */
MAVLINK_WIP
static inline uint32_t mavlink_msg_rexroth_motion_platform_get_frame_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field motion_status from rexroth_motion_platform message
 *
 * @return  Motion Status variable as sent by the system.
 */
MAVLINK_WIP
static inline uint32_t mavlink_msg_rexroth_motion_platform_get_motion_status(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  8);
}

/**
 * @brief Get field error_code from rexroth_motion_platform message
 *
 * @return  Error code extracted from motion status.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_rexroth_motion_platform_get_error_code(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  84);
}

/**
 * @brief Get field actuator1 from rexroth_motion_platform message
 *
 * @return [m] Current actuator 1 position.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_actuator1(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field actuator2 from rexroth_motion_platform message
 *
 * @return [m] Current actuator 2 position.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_actuator2(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field actuator3 from rexroth_motion_platform message
 *
 * @return [m] Current actuator 3 position.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_actuator3(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  20);
}

/**
 * @brief Get field actuator4 from rexroth_motion_platform message
 *
 * @return [m] Current actuator 4 position.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_actuator4(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Get field actuator5 from rexroth_motion_platform message
 *
 * @return [m] Current actuator 5 position.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_actuator5(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Get field actuator6 from rexroth_motion_platform message
 *
 * @return [m] Current actuator 6 position.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_actuator6(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field platform_setpoint_x from rexroth_motion_platform message
 *
 * @return [m] X axis (surge) platform setpoint, positive forward.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_platform_setpoint_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  36);
}

/**
 * @brief Get field platform_setpoint_y from rexroth_motion_platform message
 *
 * @return [m] Y axis (sway) platform setpoint, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_platform_setpoint_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  40);
}

/**
 * @brief Get field platform_setpoint_z from rexroth_motion_platform message
 *
 * @return [m] Z axis (heave) platform setpoint, positive down.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_platform_setpoint_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  44);
}

/**
 * @brief Get field platform_setpoint_roll from rexroth_motion_platform message
 *
 * @return [rad] Roll platform setpoint, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_platform_setpoint_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  48);
}

/**
 * @brief Get field platform_setpoint_pitch from rexroth_motion_platform message
 *
 * @return [rad] Pitch platform setpoint, positive nose up.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_platform_setpoint_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  52);
}

/**
 * @brief Get field platform_setpoint_yaw from rexroth_motion_platform message
 *
 * @return [rad] Yaw platform setpoint, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_platform_setpoint_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  56);
}

/**
 * @brief Get field effect_setpoint_x from rexroth_motion_platform message
 *
 * @return [m] X axis (surge) special effect setpoint, positive forward.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_effect_setpoint_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  60);
}

/**
 * @brief Get field effect_setpoint_y from rexroth_motion_platform message
 *
 * @return [m] Y axis (sway) special effect setpoint, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_effect_setpoint_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  64);
}

/**
 * @brief Get field effect_setpoint_z from rexroth_motion_platform message
 *
 * @return [m] Z axis (heave) special effect setpoint, positive down.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_effect_setpoint_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  68);
}

/**
 * @brief Get field effect_setpoint_roll from rexroth_motion_platform message
 *
 * @return [rad] Roll special effect setpoint, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_effect_setpoint_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  72);
}

/**
 * @brief Get field effect_setpoint_pitch from rexroth_motion_platform message
 *
 * @return [rad] Pitch special effect setpoint, positive nose up.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_effect_setpoint_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  76);
}

/**
 * @brief Get field effect_setpoint_yaw from rexroth_motion_platform message
 *
 * @return [rad] Yaw special effect setpoint, positive right.
 */
MAVLINK_WIP
static inline float mavlink_msg_rexroth_motion_platform_get_effect_setpoint_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  80);
}

/**
 * @brief Decode a rexroth_motion_platform message into a struct
 *
 * @param msg The message to decode
 * @param rexroth_motion_platform C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_rexroth_motion_platform_decode(const mavlink_message_t* msg, mavlink_rexroth_motion_platform_t* rexroth_motion_platform)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rexroth_motion_platform->time_boot_ms = mavlink_msg_rexroth_motion_platform_get_time_boot_ms(msg);
    rexroth_motion_platform->frame_count = mavlink_msg_rexroth_motion_platform_get_frame_count(msg);
    rexroth_motion_platform->motion_status = mavlink_msg_rexroth_motion_platform_get_motion_status(msg);
    rexroth_motion_platform->actuator1 = mavlink_msg_rexroth_motion_platform_get_actuator1(msg);
    rexroth_motion_platform->actuator2 = mavlink_msg_rexroth_motion_platform_get_actuator2(msg);
    rexroth_motion_platform->actuator3 = mavlink_msg_rexroth_motion_platform_get_actuator3(msg);
    rexroth_motion_platform->actuator4 = mavlink_msg_rexroth_motion_platform_get_actuator4(msg);
    rexroth_motion_platform->actuator5 = mavlink_msg_rexroth_motion_platform_get_actuator5(msg);
    rexroth_motion_platform->actuator6 = mavlink_msg_rexroth_motion_platform_get_actuator6(msg);
    rexroth_motion_platform->platform_setpoint_x = mavlink_msg_rexroth_motion_platform_get_platform_setpoint_x(msg);
    rexroth_motion_platform->platform_setpoint_y = mavlink_msg_rexroth_motion_platform_get_platform_setpoint_y(msg);
    rexroth_motion_platform->platform_setpoint_z = mavlink_msg_rexroth_motion_platform_get_platform_setpoint_z(msg);
    rexroth_motion_platform->platform_setpoint_roll = mavlink_msg_rexroth_motion_platform_get_platform_setpoint_roll(msg);
    rexroth_motion_platform->platform_setpoint_pitch = mavlink_msg_rexroth_motion_platform_get_platform_setpoint_pitch(msg);
    rexroth_motion_platform->platform_setpoint_yaw = mavlink_msg_rexroth_motion_platform_get_platform_setpoint_yaw(msg);
    rexroth_motion_platform->effect_setpoint_x = mavlink_msg_rexroth_motion_platform_get_effect_setpoint_x(msg);
    rexroth_motion_platform->effect_setpoint_y = mavlink_msg_rexroth_motion_platform_get_effect_setpoint_y(msg);
    rexroth_motion_platform->effect_setpoint_z = mavlink_msg_rexroth_motion_platform_get_effect_setpoint_z(msg);
    rexroth_motion_platform->effect_setpoint_roll = mavlink_msg_rexroth_motion_platform_get_effect_setpoint_roll(msg);
    rexroth_motion_platform->effect_setpoint_pitch = mavlink_msg_rexroth_motion_platform_get_effect_setpoint_pitch(msg);
    rexroth_motion_platform->effect_setpoint_yaw = mavlink_msg_rexroth_motion_platform_get_effect_setpoint_yaw(msg);
    rexroth_motion_platform->error_code = mavlink_msg_rexroth_motion_platform_get_error_code(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN? msg->len : MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN;
        memset(rexroth_motion_platform, 0, MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_LEN);
    memcpy(rexroth_motion_platform, _MAV_PAYLOAD(msg), len);
#endif
}
