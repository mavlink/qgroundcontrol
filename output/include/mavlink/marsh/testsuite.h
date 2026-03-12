/** @file
 *    @brief MAVLink comm protocol testsuite generated from marsh.xml
 *    @see https://mavlink.io/en/
 */
#pragma once
#ifndef MARSH_TESTSUITE_H
#define MARSH_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL
static void mavlink_test_common(uint8_t, uint8_t, mavlink_message_t *last_msg);
static void mavlink_test_marsh(uint8_t, uint8_t, mavlink_message_t *last_msg);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_common(system_id, component_id, last_msg);
    mavlink_test_marsh(system_id, component_id, last_msg);
}
#endif

#include "../common/testsuite.h"


static void mavlink_test_control_loading_axis(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_CONTROL_LOADING_AXIS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_control_loading_axis_t packet_in = {
        963497464,45.0,73.0,101.0,53
    };
    mavlink_control_loading_axis_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.position = packet_in.position;
        packet1.velocity = packet_in.velocity;
        packet1.force = packet_in.force;
        packet1.axis = packet_in.axis;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_loading_axis_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_control_loading_axis_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_loading_axis_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.axis , packet1.position , packet1.velocity , packet1.force );
    mavlink_msg_control_loading_axis_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_loading_axis_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.axis , packet1.position , packet1.velocity , packet1.force );
    mavlink_msg_control_loading_axis_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_control_loading_axis_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_loading_axis_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.axis , packet1.position , packet1.velocity , packet1.force );
    mavlink_msg_control_loading_axis_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("CONTROL_LOADING_AXIS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_CONTROL_LOADING_AXIS) != NULL);
#endif
}

static void mavlink_test_motion_platform_state(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_MOTION_PLATFORM_STATE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_motion_platform_state_t packet_in = {
        963497464,45.0,73.0,101.0,129.0,157.0,185.0,213.0,241.0,269.0,297.0,325.0,353.0,381.0,409.0,437.0,465.0,493.0,521.0,233,44
    };
    mavlink_motion_platform_state_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.x = packet_in.x;
        packet1.y = packet_in.y;
        packet1.z = packet_in.z;
        packet1.roll = packet_in.roll;
        packet1.pitch = packet_in.pitch;
        packet1.yaw = packet_in.yaw;
        packet1.vel_x = packet_in.vel_x;
        packet1.vel_y = packet_in.vel_y;
        packet1.vel_z = packet_in.vel_z;
        packet1.vel_roll = packet_in.vel_roll;
        packet1.vel_pitch = packet_in.vel_pitch;
        packet1.vel_yaw = packet_in.vel_yaw;
        packet1.acc_x = packet_in.acc_x;
        packet1.acc_y = packet_in.acc_y;
        packet1.acc_z = packet_in.acc_z;
        packet1.acc_roll = packet_in.acc_roll;
        packet1.acc_pitch = packet_in.acc_pitch;
        packet1.acc_yaw = packet_in.acc_yaw;
        packet1.health = packet_in.health;
        packet1.mode = packet_in.mode;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_MOTION_PLATFORM_STATE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_platform_state_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_motion_platform_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_platform_state_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.health , packet1.mode , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw , packet1.vel_x , packet1.vel_y , packet1.vel_z , packet1.vel_roll , packet1.vel_pitch , packet1.vel_yaw , packet1.acc_x , packet1.acc_y , packet1.acc_z , packet1.acc_roll , packet1.acc_pitch , packet1.acc_yaw );
    mavlink_msg_motion_platform_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_platform_state_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.health , packet1.mode , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw , packet1.vel_x , packet1.vel_y , packet1.vel_z , packet1.vel_roll , packet1.vel_pitch , packet1.vel_yaw , packet1.acc_x , packet1.acc_y , packet1.acc_z , packet1.acc_roll , packet1.acc_pitch , packet1.acc_yaw );
    mavlink_msg_motion_platform_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_motion_platform_state_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_platform_state_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.health , packet1.mode , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw , packet1.vel_x , packet1.vel_y , packet1.vel_z , packet1.vel_roll , packet1.vel_pitch , packet1.vel_yaw , packet1.acc_x , packet1.acc_y , packet1.acc_z , packet1.acc_roll , packet1.acc_pitch , packet1.acc_yaw );
    mavlink_msg_motion_platform_state_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("MOTION_PLATFORM_STATE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_MOTION_PLATFORM_STATE) != NULL);
#endif
}

static void mavlink_test_rexroth_motion_platform(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_rexroth_motion_platform_t packet_in = {
        963497464,963497672,963497880,101.0,129.0,157.0,185.0,213.0,241.0,269.0,297.0,325.0,353.0,381.0,409.0,437.0,465.0,493.0,521.0,549.0,577.0,1
    };
    mavlink_rexroth_motion_platform_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.frame_count = packet_in.frame_count;
        packet1.motion_status = packet_in.motion_status;
        packet1.actuator1 = packet_in.actuator1;
        packet1.actuator2 = packet_in.actuator2;
        packet1.actuator3 = packet_in.actuator3;
        packet1.actuator4 = packet_in.actuator4;
        packet1.actuator5 = packet_in.actuator5;
        packet1.actuator6 = packet_in.actuator6;
        packet1.platform_setpoint_x = packet_in.platform_setpoint_x;
        packet1.platform_setpoint_y = packet_in.platform_setpoint_y;
        packet1.platform_setpoint_z = packet_in.platform_setpoint_z;
        packet1.platform_setpoint_roll = packet_in.platform_setpoint_roll;
        packet1.platform_setpoint_pitch = packet_in.platform_setpoint_pitch;
        packet1.platform_setpoint_yaw = packet_in.platform_setpoint_yaw;
        packet1.effect_setpoint_x = packet_in.effect_setpoint_x;
        packet1.effect_setpoint_y = packet_in.effect_setpoint_y;
        packet1.effect_setpoint_z = packet_in.effect_setpoint_z;
        packet1.effect_setpoint_roll = packet_in.effect_setpoint_roll;
        packet1.effect_setpoint_pitch = packet_in.effect_setpoint_pitch;
        packet1.effect_setpoint_yaw = packet_in.effect_setpoint_yaw;
        packet1.error_code = packet_in.error_code;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_rexroth_motion_platform_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_rexroth_motion_platform_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_rexroth_motion_platform_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.frame_count , packet1.motion_status , packet1.error_code , packet1.actuator1 , packet1.actuator2 , packet1.actuator3 , packet1.actuator4 , packet1.actuator5 , packet1.actuator6 , packet1.platform_setpoint_x , packet1.platform_setpoint_y , packet1.platform_setpoint_z , packet1.platform_setpoint_roll , packet1.platform_setpoint_pitch , packet1.platform_setpoint_yaw , packet1.effect_setpoint_x , packet1.effect_setpoint_y , packet1.effect_setpoint_z , packet1.effect_setpoint_roll , packet1.effect_setpoint_pitch , packet1.effect_setpoint_yaw );
    mavlink_msg_rexroth_motion_platform_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_rexroth_motion_platform_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.frame_count , packet1.motion_status , packet1.error_code , packet1.actuator1 , packet1.actuator2 , packet1.actuator3 , packet1.actuator4 , packet1.actuator5 , packet1.actuator6 , packet1.platform_setpoint_x , packet1.platform_setpoint_y , packet1.platform_setpoint_z , packet1.platform_setpoint_roll , packet1.platform_setpoint_pitch , packet1.platform_setpoint_yaw , packet1.effect_setpoint_x , packet1.effect_setpoint_y , packet1.effect_setpoint_z , packet1.effect_setpoint_roll , packet1.effect_setpoint_pitch , packet1.effect_setpoint_yaw );
    mavlink_msg_rexroth_motion_platform_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_rexroth_motion_platform_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_rexroth_motion_platform_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.frame_count , packet1.motion_status , packet1.error_code , packet1.actuator1 , packet1.actuator2 , packet1.actuator3 , packet1.actuator4 , packet1.actuator5 , packet1.actuator6 , packet1.platform_setpoint_x , packet1.platform_setpoint_y , packet1.platform_setpoint_z , packet1.platform_setpoint_roll , packet1.platform_setpoint_pitch , packet1.platform_setpoint_yaw , packet1.effect_setpoint_x , packet1.effect_setpoint_y , packet1.effect_setpoint_z , packet1.effect_setpoint_roll , packet1.effect_setpoint_pitch , packet1.effect_setpoint_yaw );
    mavlink_msg_rexroth_motion_platform_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("REXROTH_MOTION_PLATFORM") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_REXROTH_MOTION_PLATFORM) != NULL);
#endif
}

static void mavlink_test_motion_cue_extra(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_MOTION_CUE_EXTRA >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_motion_cue_extra_t packet_in = {
        963497464,45.0,73.0,101.0,129.0,157.0,185.0
    };
    mavlink_motion_cue_extra_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.vel_roll = packet_in.vel_roll;
        packet1.vel_pitch = packet_in.vel_pitch;
        packet1.vel_yaw = packet_in.vel_yaw;
        packet1.acc_x = packet_in.acc_x;
        packet1.acc_y = packet_in.acc_y;
        packet1.acc_z = packet_in.acc_z;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_cue_extra_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_motion_cue_extra_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_cue_extra_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.vel_roll , packet1.vel_pitch , packet1.vel_yaw , packet1.acc_x , packet1.acc_y , packet1.acc_z );
    mavlink_msg_motion_cue_extra_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_cue_extra_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.vel_roll , packet1.vel_pitch , packet1.vel_yaw , packet1.acc_x , packet1.acc_y , packet1.acc_z );
    mavlink_msg_motion_cue_extra_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_motion_cue_extra_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_motion_cue_extra_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.vel_roll , packet1.vel_pitch , packet1.vel_yaw , packet1.acc_x , packet1.acc_y , packet1.acc_z );
    mavlink_msg_motion_cue_extra_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("MOTION_CUE_EXTRA") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_MOTION_CUE_EXTRA) != NULL);
#endif
}

static void mavlink_test_eye_tracking_data(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_EYE_TRACKING_DATA >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_eye_tracking_data_t packet_in = {
        93372036854775807ULL,73.0,101.0,129.0,157.0,185.0,213.0,241.0,269.0,297.0,325.0,149,216
    };
    mavlink_eye_tracking_data_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_usec = packet_in.time_usec;
        packet1.gaze_origin_x = packet_in.gaze_origin_x;
        packet1.gaze_origin_y = packet_in.gaze_origin_y;
        packet1.gaze_origin_z = packet_in.gaze_origin_z;
        packet1.gaze_direction_x = packet_in.gaze_direction_x;
        packet1.gaze_direction_y = packet_in.gaze_direction_y;
        packet1.gaze_direction_z = packet_in.gaze_direction_z;
        packet1.video_gaze_x = packet_in.video_gaze_x;
        packet1.video_gaze_y = packet_in.video_gaze_y;
        packet1.surface_gaze_x = packet_in.surface_gaze_x;
        packet1.surface_gaze_y = packet_in.surface_gaze_y;
        packet1.sensor_id = packet_in.sensor_id;
        packet1.surface_id = packet_in.surface_id;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_eye_tracking_data_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_eye_tracking_data_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_eye_tracking_data_pack(system_id, component_id, &msg , packet1.time_usec , packet1.sensor_id , packet1.gaze_origin_x , packet1.gaze_origin_y , packet1.gaze_origin_z , packet1.gaze_direction_x , packet1.gaze_direction_y , packet1.gaze_direction_z , packet1.video_gaze_x , packet1.video_gaze_y , packet1.surface_id , packet1.surface_gaze_x , packet1.surface_gaze_y );
    mavlink_msg_eye_tracking_data_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_eye_tracking_data_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_usec , packet1.sensor_id , packet1.gaze_origin_x , packet1.gaze_origin_y , packet1.gaze_origin_z , packet1.gaze_direction_x , packet1.gaze_direction_y , packet1.gaze_direction_z , packet1.video_gaze_x , packet1.video_gaze_y , packet1.surface_id , packet1.surface_gaze_x , packet1.surface_gaze_y );
    mavlink_msg_eye_tracking_data_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_eye_tracking_data_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_eye_tracking_data_send(MAVLINK_COMM_1 , packet1.time_usec , packet1.sensor_id , packet1.gaze_origin_x , packet1.gaze_origin_y , packet1.gaze_origin_z , packet1.gaze_direction_x , packet1.gaze_direction_y , packet1.gaze_direction_z , packet1.video_gaze_x , packet1.video_gaze_y , packet1.surface_id , packet1.surface_gaze_x , packet1.surface_gaze_y );
    mavlink_msg_eye_tracking_data_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("EYE_TRACKING_DATA") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_EYE_TRACKING_DATA) != NULL);
#endif
}

static void mavlink_test_marsh(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_control_loading_axis(system_id, component_id, last_msg);
    mavlink_test_motion_platform_state(system_id, component_id, last_msg);
    mavlink_test_rexroth_motion_platform(system_id, component_id, last_msg);
    mavlink_test_motion_cue_extra(system_id, component_id, last_msg);
    mavlink_test_eye_tracking_data(system_id, component_id, last_msg);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MARSH_TESTSUITE_H
