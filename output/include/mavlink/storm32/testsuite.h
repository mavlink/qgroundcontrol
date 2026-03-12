/** @file
 *    @brief MAVLink comm protocol testsuite generated from storm32.xml
 *    @see https://mavlink.io/en/
 */
#pragma once
#ifndef STORM32_TESTSUITE_H
#define STORM32_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL
static void mavlink_test_ardupilotmega(uint8_t, uint8_t, mavlink_message_t *last_msg);
static void mavlink_test_storm32(uint8_t, uint8_t, mavlink_message_t *last_msg);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_ardupilotmega(system_id, component_id, last_msg);
    mavlink_test_storm32(system_id, component_id, last_msg);
}
#endif

#include "../ardupilotmega/testsuite.h"


static void mavlink_test_storm32_gimbal_manager_information(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_INFORMATION >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_storm32_gimbal_manager_information_t packet_in = {
        963497464,963497672,73.0,101.0,129.0,157.0,185.0,213.0,101
    };
    mavlink_storm32_gimbal_manager_information_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.device_cap_flags = packet_in.device_cap_flags;
        packet1.manager_cap_flags = packet_in.manager_cap_flags;
        packet1.roll_min = packet_in.roll_min;
        packet1.roll_max = packet_in.roll_max;
        packet1.pitch_min = packet_in.pitch_min;
        packet1.pitch_max = packet_in.pitch_max;
        packet1.yaw_min = packet_in.yaw_min;
        packet1.yaw_max = packet_in.yaw_max;
        packet1.gimbal_id = packet_in.gimbal_id;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_INFORMATION_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_INFORMATION_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_information_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_storm32_gimbal_manager_information_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_information_pack(system_id, component_id, &msg , packet1.gimbal_id , packet1.device_cap_flags , packet1.manager_cap_flags , packet1.roll_min , packet1.roll_max , packet1.pitch_min , packet1.pitch_max , packet1.yaw_min , packet1.yaw_max );
    mavlink_msg_storm32_gimbal_manager_information_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_information_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.gimbal_id , packet1.device_cap_flags , packet1.manager_cap_flags , packet1.roll_min , packet1.roll_max , packet1.pitch_min , packet1.pitch_max , packet1.yaw_min , packet1.yaw_max );
    mavlink_msg_storm32_gimbal_manager_information_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_storm32_gimbal_manager_information_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_information_send(MAVLINK_COMM_1 , packet1.gimbal_id , packet1.device_cap_flags , packet1.manager_cap_flags , packet1.roll_min , packet1.roll_max , packet1.pitch_min , packet1.pitch_max , packet1.yaw_min , packet1.yaw_max );
    mavlink_msg_storm32_gimbal_manager_information_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("STORM32_GIMBAL_MANAGER_INFORMATION") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_INFORMATION) != NULL);
#endif
}

static void mavlink_test_storm32_gimbal_manager_status(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_STATUS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_storm32_gimbal_manager_status_t packet_in = {
        17235,17339,17,84,151
    };
    mavlink_storm32_gimbal_manager_status_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.device_flags = packet_in.device_flags;
        packet1.manager_flags = packet_in.manager_flags;
        packet1.gimbal_id = packet_in.gimbal_id;
        packet1.supervisor = packet_in.supervisor;
        packet1.profile = packet_in.profile;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_STATUS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_STATUS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_status_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_storm32_gimbal_manager_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_status_pack(system_id, component_id, &msg , packet1.gimbal_id , packet1.supervisor , packet1.device_flags , packet1.manager_flags , packet1.profile );
    mavlink_msg_storm32_gimbal_manager_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.gimbal_id , packet1.supervisor , packet1.device_flags , packet1.manager_flags , packet1.profile );
    mavlink_msg_storm32_gimbal_manager_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_storm32_gimbal_manager_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_status_send(MAVLINK_COMM_1 , packet1.gimbal_id , packet1.supervisor , packet1.device_flags , packet1.manager_flags , packet1.profile );
    mavlink_msg_storm32_gimbal_manager_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("STORM32_GIMBAL_MANAGER_STATUS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_STATUS) != NULL);
#endif
}

static void mavlink_test_storm32_gimbal_manager_control(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_storm32_gimbal_manager_control_t packet_in = {
        { 17.0, 18.0, 19.0, 20.0 },129.0,157.0,185.0,18691,18795,101,168,235,46
    };
    mavlink_storm32_gimbal_manager_control_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.angular_velocity_x = packet_in.angular_velocity_x;
        packet1.angular_velocity_y = packet_in.angular_velocity_y;
        packet1.angular_velocity_z = packet_in.angular_velocity_z;
        packet1.device_flags = packet_in.device_flags;
        packet1.manager_flags = packet_in.manager_flags;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.gimbal_id = packet_in.gimbal_id;
        packet1.client = packet_in.client;
        
        mav_array_memcpy(packet1.q, packet_in.q, sizeof(float)*4);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_storm32_gimbal_manager_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.device_flags , packet1.manager_flags , packet1.q , packet1.angular_velocity_x , packet1.angular_velocity_y , packet1.angular_velocity_z );
    mavlink_msg_storm32_gimbal_manager_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.device_flags , packet1.manager_flags , packet1.q , packet1.angular_velocity_x , packet1.angular_velocity_y , packet1.angular_velocity_z );
    mavlink_msg_storm32_gimbal_manager_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_storm32_gimbal_manager_control_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.device_flags , packet1.manager_flags , packet1.q , packet1.angular_velocity_x , packet1.angular_velocity_y , packet1.angular_velocity_z );
    mavlink_msg_storm32_gimbal_manager_control_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("STORM32_GIMBAL_MANAGER_CONTROL") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL) != NULL);
#endif
}

static void mavlink_test_storm32_gimbal_manager_control_pitchyaw(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL_PITCHYAW >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_storm32_gimbal_manager_control_pitchyaw_t packet_in = {
        17.0,45.0,73.0,101.0,18067,18171,65,132,199,10
    };
    mavlink_storm32_gimbal_manager_control_pitchyaw_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.pitch = packet_in.pitch;
        packet1.yaw = packet_in.yaw;
        packet1.pitch_rate = packet_in.pitch_rate;
        packet1.yaw_rate = packet_in.yaw_rate;
        packet1.device_flags = packet_in.device_flags;
        packet1.manager_flags = packet_in.manager_flags;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.gimbal_id = packet_in.gimbal_id;
        packet1.client = packet_in.client;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL_PITCHYAW_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL_PITCHYAW_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.device_flags , packet1.manager_flags , packet1.pitch , packet1.yaw , packet1.pitch_rate , packet1.yaw_rate );
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.device_flags , packet1.manager_flags , packet1.pitch , packet1.yaw , packet1.pitch_rate , packet1.yaw_rate );
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.device_flags , packet1.manager_flags , packet1.pitch , packet1.yaw , packet1.pitch_rate , packet1.yaw_rate );
    mavlink_msg_storm32_gimbal_manager_control_pitchyaw_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("STORM32_GIMBAL_MANAGER_CONTROL_PITCHYAW") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CONTROL_PITCHYAW) != NULL);
#endif
}

static void mavlink_test_storm32_gimbal_manager_correct_roll(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CORRECT_ROLL >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_storm32_gimbal_manager_correct_roll_t packet_in = {
        17.0,17,84,151,218
    };
    mavlink_storm32_gimbal_manager_correct_roll_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.roll = packet_in.roll;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.gimbal_id = packet_in.gimbal_id;
        packet1.client = packet_in.client;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CORRECT_ROLL_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CORRECT_ROLL_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_correct_roll_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_storm32_gimbal_manager_correct_roll_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_correct_roll_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.roll );
    mavlink_msg_storm32_gimbal_manager_correct_roll_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_correct_roll_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.roll );
    mavlink_msg_storm32_gimbal_manager_correct_roll_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_storm32_gimbal_manager_correct_roll_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_storm32_gimbal_manager_correct_roll_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.gimbal_id , packet1.client , packet1.roll );
    mavlink_msg_storm32_gimbal_manager_correct_roll_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("STORM32_GIMBAL_MANAGER_CORRECT_ROLL") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_STORM32_GIMBAL_MANAGER_CORRECT_ROLL) != NULL);
#endif
}

static void mavlink_test_qshot_status(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_QSHOT_STATUS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_qshot_status_t packet_in = {
        17235,17339
    };
    mavlink_qshot_status_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.mode = packet_in.mode;
        packet1.shot_state = packet_in.shot_state;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_QSHOT_STATUS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_QSHOT_STATUS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_qshot_status_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_qshot_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_qshot_status_pack(system_id, component_id, &msg , packet1.mode , packet1.shot_state );
    mavlink_msg_qshot_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_qshot_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.mode , packet1.shot_state );
    mavlink_msg_qshot_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_qshot_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_qshot_status_send(MAVLINK_COMM_1 , packet1.mode , packet1.shot_state );
    mavlink_msg_qshot_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("QSHOT_STATUS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_QSHOT_STATUS) != NULL);
#endif
}

static void mavlink_test_autopilot_state_for_gimbal_device_ext(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_autopilot_state_for_gimbal_device_ext_t packet_in = {
        93372036854775807ULL,73.0,101.0,129.0,65,132
    };
    mavlink_autopilot_state_for_gimbal_device_ext_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_us = packet_in.time_boot_us;
        packet1.wind_x = packet_in.wind_x;
        packet1.wind_y = packet_in.wind_y;
        packet1.wind_correction_angle = packet_in.wind_correction_angle;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_autopilot_state_for_gimbal_device_ext_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_autopilot_state_for_gimbal_device_ext_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_autopilot_state_for_gimbal_device_ext_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.time_boot_us , packet1.wind_x , packet1.wind_y , packet1.wind_correction_angle );
    mavlink_msg_autopilot_state_for_gimbal_device_ext_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_autopilot_state_for_gimbal_device_ext_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.time_boot_us , packet1.wind_x , packet1.wind_y , packet1.wind_correction_angle );
    mavlink_msg_autopilot_state_for_gimbal_device_ext_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_autopilot_state_for_gimbal_device_ext_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_autopilot_state_for_gimbal_device_ext_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.time_boot_us , packet1.wind_x , packet1.wind_y , packet1.wind_correction_angle );
    mavlink_msg_autopilot_state_for_gimbal_device_ext_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT) != NULL);
#endif
}

static void mavlink_test_frsky_passthrough_array(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_frsky_passthrough_array_t packet_in = {
        963497464,17,{ 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67 }
    };
    mavlink_frsky_passthrough_array_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.count = packet_in.count;
        
        mav_array_memcpy(packet1.packet_buf, packet_in.packet_buf, sizeof(uint8_t)*240);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_frsky_passthrough_array_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_frsky_passthrough_array_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_frsky_passthrough_array_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.count , packet1.packet_buf );
    mavlink_msg_frsky_passthrough_array_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_frsky_passthrough_array_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.count , packet1.packet_buf );
    mavlink_msg_frsky_passthrough_array_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_frsky_passthrough_array_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_frsky_passthrough_array_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.count , packet1.packet_buf );
    mavlink_msg_frsky_passthrough_array_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("FRSKY_PASSTHROUGH_ARRAY") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY) != NULL);
#endif
}

static void mavlink_test_param_value_array(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_PARAM_VALUE_ARRAY >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_param_value_array_t packet_in = {
        17235,17339,17443,151,{ 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209 }
    };
    mavlink_param_value_array_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.param_count = packet_in.param_count;
        packet1.param_index_first = packet_in.param_index_first;
        packet1.flags = packet_in.flags;
        packet1.param_array_len = packet_in.param_array_len;
        
        mav_array_memcpy(packet1.packet_buf, packet_in.packet_buf, sizeof(uint8_t)*248);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_param_value_array_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_param_value_array_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_param_value_array_pack(system_id, component_id, &msg , packet1.param_count , packet1.param_index_first , packet1.param_array_len , packet1.flags , packet1.packet_buf );
    mavlink_msg_param_value_array_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_param_value_array_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.param_count , packet1.param_index_first , packet1.param_array_len , packet1.flags , packet1.packet_buf );
    mavlink_msg_param_value_array_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_param_value_array_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_param_value_array_send(MAVLINK_COMM_1 , packet1.param_count , packet1.param_index_first , packet1.param_array_len , packet1.flags , packet1.packet_buf );
    mavlink_msg_param_value_array_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("PARAM_VALUE_ARRAY") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_PARAM_VALUE_ARRAY) != NULL);
#endif
}

static void mavlink_test_mlrs_radio_link_stats(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_mlrs_radio_link_stats_t packet_in = {
        17235,139,206,17,84,151,218,29,96,163,230,41,108,175,122.0,150.0
    };
    mavlink_mlrs_radio_link_stats_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.flags = packet_in.flags;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.rx_LQ_rc = packet_in.rx_LQ_rc;
        packet1.rx_LQ_ser = packet_in.rx_LQ_ser;
        packet1.rx_rssi1 = packet_in.rx_rssi1;
        packet1.rx_snr1 = packet_in.rx_snr1;
        packet1.tx_LQ_ser = packet_in.tx_LQ_ser;
        packet1.tx_rssi1 = packet_in.tx_rssi1;
        packet1.tx_snr1 = packet_in.tx_snr1;
        packet1.rx_rssi2 = packet_in.rx_rssi2;
        packet1.rx_snr2 = packet_in.rx_snr2;
        packet1.tx_rssi2 = packet_in.tx_rssi2;
        packet1.tx_snr2 = packet_in.tx_snr2;
        packet1.frequency1 = packet_in.frequency1;
        packet1.frequency2 = packet_in.frequency2;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_stats_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_mlrs_radio_link_stats_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_stats_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.flags , packet1.rx_LQ_rc , packet1.rx_LQ_ser , packet1.rx_rssi1 , packet1.rx_snr1 , packet1.tx_LQ_ser , packet1.tx_rssi1 , packet1.tx_snr1 , packet1.rx_rssi2 , packet1.rx_snr2 , packet1.tx_rssi2 , packet1.tx_snr2 , packet1.frequency1 , packet1.frequency2 );
    mavlink_msg_mlrs_radio_link_stats_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_stats_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.flags , packet1.rx_LQ_rc , packet1.rx_LQ_ser , packet1.rx_rssi1 , packet1.rx_snr1 , packet1.tx_LQ_ser , packet1.tx_rssi1 , packet1.tx_snr1 , packet1.rx_rssi2 , packet1.rx_snr2 , packet1.tx_rssi2 , packet1.tx_snr2 , packet1.frequency1 , packet1.frequency2 );
    mavlink_msg_mlrs_radio_link_stats_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_mlrs_radio_link_stats_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_stats_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.flags , packet1.rx_LQ_rc , packet1.rx_LQ_ser , packet1.rx_rssi1 , packet1.rx_snr1 , packet1.tx_LQ_ser , packet1.tx_rssi1 , packet1.tx_snr1 , packet1.rx_rssi2 , packet1.rx_snr2 , packet1.tx_rssi2 , packet1.tx_snr2 , packet1.frequency1 , packet1.frequency2 );
    mavlink_msg_mlrs_radio_link_stats_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("MLRS_RADIO_LINK_STATS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS) != NULL);
#endif
}

static void mavlink_test_mlrs_radio_link_information(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_mlrs_radio_link_information_t packet_in = {
        17235,17339,17443,17547,29,96,163,230,41,108,"OPQRS","UVWXY",211,22
    };
    mavlink_mlrs_radio_link_information_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.tx_frame_rate = packet_in.tx_frame_rate;
        packet1.rx_frame_rate = packet_in.rx_frame_rate;
        packet1.tx_ser_data_rate = packet_in.tx_ser_data_rate;
        packet1.rx_ser_data_rate = packet_in.rx_ser_data_rate;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.type = packet_in.type;
        packet1.mode = packet_in.mode;
        packet1.tx_power = packet_in.tx_power;
        packet1.rx_power = packet_in.rx_power;
        packet1.tx_receive_sensitivity = packet_in.tx_receive_sensitivity;
        packet1.rx_receive_sensitivity = packet_in.rx_receive_sensitivity;
        
        mav_array_memcpy(packet1.mode_str, packet_in.mode_str, sizeof(char)*6);
        mav_array_memcpy(packet1.band_str, packet_in.band_str, sizeof(char)*6);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_information_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_mlrs_radio_link_information_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_information_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.type , packet1.mode , packet1.tx_power , packet1.rx_power , packet1.tx_frame_rate , packet1.rx_frame_rate , packet1.mode_str , packet1.band_str , packet1.tx_ser_data_rate , packet1.rx_ser_data_rate , packet1.tx_receive_sensitivity , packet1.rx_receive_sensitivity );
    mavlink_msg_mlrs_radio_link_information_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_information_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.type , packet1.mode , packet1.tx_power , packet1.rx_power , packet1.tx_frame_rate , packet1.rx_frame_rate , packet1.mode_str , packet1.band_str , packet1.tx_ser_data_rate , packet1.rx_ser_data_rate , packet1.tx_receive_sensitivity , packet1.rx_receive_sensitivity );
    mavlink_msg_mlrs_radio_link_information_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_mlrs_radio_link_information_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_information_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.type , packet1.mode , packet1.tx_power , packet1.rx_power , packet1.tx_frame_rate , packet1.rx_frame_rate , packet1.mode_str , packet1.band_str , packet1.tx_ser_data_rate , packet1.rx_ser_data_rate , packet1.tx_receive_sensitivity , packet1.rx_receive_sensitivity );
    mavlink_msg_mlrs_radio_link_information_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("MLRS_RADIO_LINK_INFORMATION") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION) != NULL);
#endif
}

static void mavlink_test_mlrs_radio_link_flow_control(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_mlrs_radio_link_flow_control_t packet_in = {
        17235,17339,17,84,151
    };
    mavlink_mlrs_radio_link_flow_control_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.tx_ser_rate = packet_in.tx_ser_rate;
        packet1.rx_ser_rate = packet_in.rx_ser_rate;
        packet1.tx_used_ser_bandwidth = packet_in.tx_used_ser_bandwidth;
        packet1.rx_used_ser_bandwidth = packet_in.rx_used_ser_bandwidth;
        packet1.txbuf = packet_in.txbuf;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_flow_control_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_mlrs_radio_link_flow_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_flow_control_pack(system_id, component_id, &msg , packet1.tx_ser_rate , packet1.rx_ser_rate , packet1.tx_used_ser_bandwidth , packet1.rx_used_ser_bandwidth , packet1.txbuf );
    mavlink_msg_mlrs_radio_link_flow_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_flow_control_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.tx_ser_rate , packet1.rx_ser_rate , packet1.tx_used_ser_bandwidth , packet1.rx_used_ser_bandwidth , packet1.txbuf );
    mavlink_msg_mlrs_radio_link_flow_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_mlrs_radio_link_flow_control_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_mlrs_radio_link_flow_control_send(MAVLINK_COMM_1 , packet1.tx_ser_rate , packet1.rx_ser_rate , packet1.tx_used_ser_bandwidth , packet1.rx_used_ser_bandwidth , packet1.txbuf );
    mavlink_msg_mlrs_radio_link_flow_control_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("MLRS_RADIO_LINK_FLOW_CONTROL") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL) != NULL);
#endif
}

static void mavlink_test_storm32(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_storm32_gimbal_manager_information(system_id, component_id, last_msg);
    mavlink_test_storm32_gimbal_manager_status(system_id, component_id, last_msg);
    mavlink_test_storm32_gimbal_manager_control(system_id, component_id, last_msg);
    mavlink_test_storm32_gimbal_manager_control_pitchyaw(system_id, component_id, last_msg);
    mavlink_test_storm32_gimbal_manager_correct_roll(system_id, component_id, last_msg);
    mavlink_test_qshot_status(system_id, component_id, last_msg);
    mavlink_test_autopilot_state_for_gimbal_device_ext(system_id, component_id, last_msg);
    mavlink_test_frsky_passthrough_array(system_id, component_id, last_msg);
    mavlink_test_param_value_array(system_id, component_id, last_msg);
    mavlink_test_mlrs_radio_link_stats(system_id, component_id, last_msg);
    mavlink_test_mlrs_radio_link_information(system_id, component_id, last_msg);
    mavlink_test_mlrs_radio_link_flow_control(system_id, component_id, last_msg);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // STORM32_TESTSUITE_H
