/** @file
 *    @brief MAVLink comm protocol testsuite generated from development.xml
 *    @see https://mavlink.io/en/
 */
#pragma once
#ifndef DEVELOPMENT_TESTSUITE_H
#define DEVELOPMENT_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL
static void mavlink_test_common(uint8_t, uint8_t, mavlink_message_t *last_msg);
static void mavlink_test_development(uint8_t, uint8_t, mavlink_message_t *last_msg);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_common(system_id, component_id, last_msg);
    mavlink_test_development(system_id, component_id, last_msg);
}
#endif

#include "../common/testsuite.h"


static void mavlink_test_global_position(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_GLOBAL_POSITION >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_global_position_t packet_in = {
        93372036854775807ULL,963497880,963498088,129.0,157.0,185.0,213.0,101,168,235
    };
    mavlink_global_position_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_usec = packet_in.time_usec;
        packet1.lat = packet_in.lat;
        packet1.lon = packet_in.lon;
        packet1.alt = packet_in.alt;
        packet1.alt_ellipsoid = packet_in.alt_ellipsoid;
        packet1.eph = packet_in.eph;
        packet1.epv = packet_in.epv;
        packet1.id = packet_in.id;
        packet1.source = packet_in.source;
        packet1.flags = packet_in.flags;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_global_position_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_global_position_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_global_position_pack(system_id, component_id, &msg , packet1.id , packet1.time_usec , packet1.source , packet1.flags , packet1.lat , packet1.lon , packet1.alt , packet1.alt_ellipsoid , packet1.eph , packet1.epv );
    mavlink_msg_global_position_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_global_position_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.id , packet1.time_usec , packet1.source , packet1.flags , packet1.lat , packet1.lon , packet1.alt , packet1.alt_ellipsoid , packet1.eph , packet1.epv );
    mavlink_msg_global_position_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_global_position_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_global_position_send(MAVLINK_COMM_1 , packet1.id , packet1.time_usec , packet1.source , packet1.flags , packet1.lat , packet1.lon , packet1.alt , packet1.alt_ellipsoid , packet1.eph , packet1.epv );
    mavlink_msg_global_position_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("GLOBAL_POSITION") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_GLOBAL_POSITION) != NULL);
#endif
}

static void mavlink_test_set_velocity_limits(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_SET_VELOCITY_LIMITS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_set_velocity_limits_t packet_in = {
        17.0,45.0,73.0,41,108
    };
    mavlink_set_velocity_limits_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.horizontal_speed_limit = packet_in.horizontal_speed_limit;
        packet1.vertical_speed_limit = packet_in.vertical_speed_limit;
        packet1.yaw_rate_limit = packet_in.yaw_rate_limit;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_set_velocity_limits_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_set_velocity_limits_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_set_velocity_limits_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.horizontal_speed_limit , packet1.vertical_speed_limit , packet1.yaw_rate_limit );
    mavlink_msg_set_velocity_limits_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_set_velocity_limits_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.horizontal_speed_limit , packet1.vertical_speed_limit , packet1.yaw_rate_limit );
    mavlink_msg_set_velocity_limits_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_set_velocity_limits_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_set_velocity_limits_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.horizontal_speed_limit , packet1.vertical_speed_limit , packet1.yaw_rate_limit );
    mavlink_msg_set_velocity_limits_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("SET_VELOCITY_LIMITS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_SET_VELOCITY_LIMITS) != NULL);
#endif
}

static void mavlink_test_velocity_limits(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_VELOCITY_LIMITS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_velocity_limits_t packet_in = {
        17.0,45.0,73.0
    };
    mavlink_velocity_limits_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.horizontal_speed_limit = packet_in.horizontal_speed_limit;
        packet1.vertical_speed_limit = packet_in.vertical_speed_limit;
        packet1.yaw_rate_limit = packet_in.yaw_rate_limit;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_VELOCITY_LIMITS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_VELOCITY_LIMITS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_velocity_limits_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_velocity_limits_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_velocity_limits_pack(system_id, component_id, &msg , packet1.horizontal_speed_limit , packet1.vertical_speed_limit , packet1.yaw_rate_limit );
    mavlink_msg_velocity_limits_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_velocity_limits_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.horizontal_speed_limit , packet1.vertical_speed_limit , packet1.yaw_rate_limit );
    mavlink_msg_velocity_limits_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_velocity_limits_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_velocity_limits_send(MAVLINK_COMM_1 , packet1.horizontal_speed_limit , packet1.vertical_speed_limit , packet1.yaw_rate_limit );
    mavlink_msg_velocity_limits_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("VELOCITY_LIMITS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_VELOCITY_LIMITS) != NULL);
#endif
}

static void mavlink_test_battery_status_v2(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_BATTERY_STATUS_V2 >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_battery_status_v2_t packet_in = {
        17.0,45.0,73.0,101.0,963498296,18275,199,10
    };
    mavlink_battery_status_v2_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.voltage = packet_in.voltage;
        packet1.current = packet_in.current;
        packet1.capacity_consumed = packet_in.capacity_consumed;
        packet1.capacity_remaining = packet_in.capacity_remaining;
        packet1.status_flags = packet_in.status_flags;
        packet1.temperature = packet_in.temperature;
        packet1.id = packet_in.id;
        packet1.percent_remaining = packet_in.percent_remaining;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_battery_status_v2_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_battery_status_v2_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_battery_status_v2_pack(system_id, component_id, &msg , packet1.id , packet1.temperature , packet1.voltage , packet1.current , packet1.capacity_consumed , packet1.capacity_remaining , packet1.percent_remaining , packet1.status_flags );
    mavlink_msg_battery_status_v2_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_battery_status_v2_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.id , packet1.temperature , packet1.voltage , packet1.current , packet1.capacity_consumed , packet1.capacity_remaining , packet1.percent_remaining , packet1.status_flags );
    mavlink_msg_battery_status_v2_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_battery_status_v2_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_battery_status_v2_send(MAVLINK_COMM_1 , packet1.id , packet1.temperature , packet1.voltage , packet1.current , packet1.capacity_consumed , packet1.capacity_remaining , packet1.percent_remaining , packet1.status_flags );
    mavlink_msg_battery_status_v2_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("BATTERY_STATUS_V2") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_BATTERY_STATUS_V2) != NULL);
#endif
}

static void mavlink_test_group_start(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_GROUP_START >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_group_start_t packet_in = {
        93372036854775807ULL,963497880,963498088
    };
    mavlink_group_start_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_usec = packet_in.time_usec;
        packet1.group_id = packet_in.group_id;
        packet1.mission_checksum = packet_in.mission_checksum;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_GROUP_START_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_GROUP_START_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_start_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_group_start_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_start_pack(system_id, component_id, &msg , packet1.group_id , packet1.mission_checksum , packet1.time_usec );
    mavlink_msg_group_start_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_start_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.group_id , packet1.mission_checksum , packet1.time_usec );
    mavlink_msg_group_start_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_group_start_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_start_send(MAVLINK_COMM_1 , packet1.group_id , packet1.mission_checksum , packet1.time_usec );
    mavlink_msg_group_start_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("GROUP_START") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_GROUP_START) != NULL);
#endif
}

static void mavlink_test_group_end(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_GROUP_END >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_group_end_t packet_in = {
        93372036854775807ULL,963497880,963498088
    };
    mavlink_group_end_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_usec = packet_in.time_usec;
        packet1.group_id = packet_in.group_id;
        packet1.mission_checksum = packet_in.mission_checksum;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_GROUP_END_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_GROUP_END_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_end_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_group_end_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_end_pack(system_id, component_id, &msg , packet1.group_id , packet1.mission_checksum , packet1.time_usec );
    mavlink_msg_group_end_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_end_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.group_id , packet1.mission_checksum , packet1.time_usec );
    mavlink_msg_group_end_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_group_end_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_group_end_send(MAVLINK_COMM_1 , packet1.group_id , packet1.mission_checksum , packet1.time_usec );
    mavlink_msg_group_end_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("GROUP_END") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_GROUP_END) != NULL);
#endif
}

static void mavlink_test_radio_rc_channels(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_RADIO_RC_CHANNELS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_radio_rc_channels_t packet_in = {
        963497464,17443,151,218,29,{ 17703, 17704, 17705, 17706, 17707, 17708, 17709, 17710, 17711, 17712, 17713, 17714, 17715, 17716, 17717, 17718, 17719, 17720, 17721, 17722, 17723, 17724, 17725, 17726, 17727, 17728, 17729, 17730, 17731, 17732, 17733, 17734 }
    };
    mavlink_radio_rc_channels_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_last_update_ms = packet_in.time_last_update_ms;
        packet1.flags = packet_in.flags;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.count = packet_in.count;
        
        mav_array_memcpy(packet1.channels, packet_in.channels, sizeof(int16_t)*32);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_RADIO_RC_CHANNELS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_RADIO_RC_CHANNELS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_radio_rc_channels_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_radio_rc_channels_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_radio_rc_channels_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.time_last_update_ms , packet1.flags , packet1.count , packet1.channels );
    mavlink_msg_radio_rc_channels_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_radio_rc_channels_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.time_last_update_ms , packet1.flags , packet1.count , packet1.channels );
    mavlink_msg_radio_rc_channels_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_radio_rc_channels_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_radio_rc_channels_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.time_last_update_ms , packet1.flags , packet1.count , packet1.channels );
    mavlink_msg_radio_rc_channels_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("RADIO_RC_CHANNELS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_RADIO_RC_CHANNELS) != NULL);
#endif
}

static void mavlink_test_gnss_integrity(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_GNSS_INTEGRITY >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_gnss_integrity_t packet_in = {
        963497464,17443,17547,29,96,163,230,41,108,175,242,53
    };
    mavlink_gnss_integrity_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.system_errors = packet_in.system_errors;
        packet1.raim_hfom = packet_in.raim_hfom;
        packet1.raim_vfom = packet_in.raim_vfom;
        packet1.id = packet_in.id;
        packet1.authentication_state = packet_in.authentication_state;
        packet1.jamming_state = packet_in.jamming_state;
        packet1.spoofing_state = packet_in.spoofing_state;
        packet1.raim_state = packet_in.raim_state;
        packet1.corrections_quality = packet_in.corrections_quality;
        packet1.system_status_summary = packet_in.system_status_summary;
        packet1.gnss_signal_quality = packet_in.gnss_signal_quality;
        packet1.post_processing_quality = packet_in.post_processing_quality;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_GNSS_INTEGRITY_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_GNSS_INTEGRITY_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_gnss_integrity_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_gnss_integrity_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_gnss_integrity_pack(system_id, component_id, &msg , packet1.id , packet1.system_errors , packet1.authentication_state , packet1.jamming_state , packet1.spoofing_state , packet1.raim_state , packet1.raim_hfom , packet1.raim_vfom , packet1.corrections_quality , packet1.system_status_summary , packet1.gnss_signal_quality , packet1.post_processing_quality );
    mavlink_msg_gnss_integrity_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_gnss_integrity_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.id , packet1.system_errors , packet1.authentication_state , packet1.jamming_state , packet1.spoofing_state , packet1.raim_state , packet1.raim_hfom , packet1.raim_vfom , packet1.corrections_quality , packet1.system_status_summary , packet1.gnss_signal_quality , packet1.post_processing_quality );
    mavlink_msg_gnss_integrity_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_gnss_integrity_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_gnss_integrity_send(MAVLINK_COMM_1 , packet1.id , packet1.system_errors , packet1.authentication_state , packet1.jamming_state , packet1.spoofing_state , packet1.raim_state , packet1.raim_hfom , packet1.raim_vfom , packet1.corrections_quality , packet1.system_status_summary , packet1.gnss_signal_quality , packet1.post_processing_quality );
    mavlink_msg_gnss_integrity_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("GNSS_INTEGRITY") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_GNSS_INTEGRITY) != NULL);
#endif
}

static void mavlink_test_target_absolute(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_TARGET_ABSOLUTE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_target_absolute_t packet_in = {
        93372036854775807ULL,963497880,963498088,129.0,{ 157.0, 158.0, 159.0 },{ 241.0, 242.0, 243.0 },{ 325.0, 326.0, 327.0, 328.0 },{ 437.0, 438.0, 439.0 },{ 521.0, 522.0 },{ 577.0, 578.0, 579.0 },{ 661.0, 662.0, 663.0 },61,128
    };
    mavlink_target_absolute_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.timestamp = packet_in.timestamp;
        packet1.lat = packet_in.lat;
        packet1.lon = packet_in.lon;
        packet1.alt = packet_in.alt;
        packet1.id = packet_in.id;
        packet1.sensor_capabilities = packet_in.sensor_capabilities;
        
        mav_array_memcpy(packet1.vel, packet_in.vel, sizeof(float)*3);
        mav_array_memcpy(packet1.acc, packet_in.acc, sizeof(float)*3);
        mav_array_memcpy(packet1.q_target, packet_in.q_target, sizeof(float)*4);
        mav_array_memcpy(packet1.rates, packet_in.rates, sizeof(float)*3);
        mav_array_memcpy(packet1.position_std, packet_in.position_std, sizeof(float)*2);
        mav_array_memcpy(packet1.vel_std, packet_in.vel_std, sizeof(float)*3);
        mav_array_memcpy(packet1.acc_std, packet_in.acc_std, sizeof(float)*3);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_absolute_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_target_absolute_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_absolute_pack(system_id, component_id, &msg , packet1.timestamp , packet1.id , packet1.sensor_capabilities , packet1.lat , packet1.lon , packet1.alt , packet1.vel , packet1.acc , packet1.q_target , packet1.rates , packet1.position_std , packet1.vel_std , packet1.acc_std );
    mavlink_msg_target_absolute_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_absolute_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.timestamp , packet1.id , packet1.sensor_capabilities , packet1.lat , packet1.lon , packet1.alt , packet1.vel , packet1.acc , packet1.q_target , packet1.rates , packet1.position_std , packet1.vel_std , packet1.acc_std );
    mavlink_msg_target_absolute_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_target_absolute_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_absolute_send(MAVLINK_COMM_1 , packet1.timestamp , packet1.id , packet1.sensor_capabilities , packet1.lat , packet1.lon , packet1.alt , packet1.vel , packet1.acc , packet1.q_target , packet1.rates , packet1.position_std , packet1.vel_std , packet1.acc_std );
    mavlink_msg_target_absolute_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("TARGET_ABSOLUTE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_TARGET_ABSOLUTE) != NULL);
#endif
}

static void mavlink_test_target_relative(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_TARGET_RELATIVE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_target_relative_t packet_in = {
        93372036854775807ULL,73.0,101.0,129.0,{ 157.0, 158.0, 159.0 },241.0,{ 269.0, 270.0, 271.0, 272.0 },{ 381.0, 382.0, 383.0, 384.0 },209,20,87
    };
    mavlink_target_relative_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.timestamp = packet_in.timestamp;
        packet1.x = packet_in.x;
        packet1.y = packet_in.y;
        packet1.z = packet_in.z;
        packet1.yaw_std = packet_in.yaw_std;
        packet1.id = packet_in.id;
        packet1.frame = packet_in.frame;
        packet1.type = packet_in.type;
        
        mav_array_memcpy(packet1.pos_std, packet_in.pos_std, sizeof(float)*3);
        mav_array_memcpy(packet1.q_target, packet_in.q_target, sizeof(float)*4);
        mav_array_memcpy(packet1.q_sensor, packet_in.q_sensor, sizeof(float)*4);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_relative_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_target_relative_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_relative_pack(system_id, component_id, &msg , packet1.timestamp , packet1.id , packet1.frame , packet1.x , packet1.y , packet1.z , packet1.pos_std , packet1.yaw_std , packet1.q_target , packet1.q_sensor , packet1.type );
    mavlink_msg_target_relative_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_relative_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.timestamp , packet1.id , packet1.frame , packet1.x , packet1.y , packet1.z , packet1.pos_std , packet1.yaw_std , packet1.q_target , packet1.q_sensor , packet1.type );
    mavlink_msg_target_relative_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_target_relative_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_target_relative_send(MAVLINK_COMM_1 , packet1.timestamp , packet1.id , packet1.frame , packet1.x , packet1.y , packet1.z , packet1.pos_std , packet1.yaw_std , packet1.q_target , packet1.q_sensor , packet1.type );
    mavlink_msg_target_relative_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("TARGET_RELATIVE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_TARGET_RELATIVE) != NULL);
#endif
}

static void mavlink_test_control_status(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_CONTROL_STATUS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_control_status_t packet_in = {
        5,72
    };
    mavlink_control_status_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.sysid_in_control = packet_in.sysid_in_control;
        packet1.flags = packet_in.flags;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_CONTROL_STATUS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_CONTROL_STATUS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_status_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_control_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_status_pack(system_id, component_id, &msg , packet1.sysid_in_control , packet1.flags );
    mavlink_msg_control_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.sysid_in_control , packet1.flags );
    mavlink_msg_control_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_control_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_status_send(MAVLINK_COMM_1 , packet1.sysid_in_control , packet1.flags );
    mavlink_msg_control_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("CONTROL_STATUS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_CONTROL_STATUS) != NULL);
#endif
}

static void mavlink_test_esc_eeprom(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_ESC_EEPROM >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_esc_eeprom_t packet_in = {
        { 963497464, 963497465, 963497466, 963497467, 963497468, 963497469 },77,144,211,22,89,156,223,{ 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225 }
    };
    mavlink_esc_eeprom_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.firmware = packet_in.firmware;
        packet1.msg_index = packet_in.msg_index;
        packet1.msg_count = packet_in.msg_count;
        packet1.esc_index = packet_in.esc_index;
        packet1.length = packet_in.length;
        
        mav_array_memcpy(packet1.write_mask, packet_in.write_mask, sizeof(uint32_t)*6);
        mav_array_memcpy(packet1.data, packet_in.data, sizeof(uint8_t)*192);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_esc_eeprom_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_esc_eeprom_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_esc_eeprom_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.firmware , packet1.msg_index , packet1.msg_count , packet1.esc_index , packet1.write_mask , packet1.length , packet1.data );
    mavlink_msg_esc_eeprom_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_esc_eeprom_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.firmware , packet1.msg_index , packet1.msg_count , packet1.esc_index , packet1.write_mask , packet1.length , packet1.data );
    mavlink_msg_esc_eeprom_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_esc_eeprom_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_esc_eeprom_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.firmware , packet1.msg_index , packet1.msg_count , packet1.esc_index , packet1.write_mask , packet1.length , packet1.data );
    mavlink_msg_esc_eeprom_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("ESC_EEPROM") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_ESC_EEPROM) != NULL);
#endif
}

static void mavlink_test_development(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_global_position(system_id, component_id, last_msg);
    mavlink_test_set_velocity_limits(system_id, component_id, last_msg);
    mavlink_test_velocity_limits(system_id, component_id, last_msg);
    mavlink_test_battery_status_v2(system_id, component_id, last_msg);
    mavlink_test_group_start(system_id, component_id, last_msg);
    mavlink_test_group_end(system_id, component_id, last_msg);
    mavlink_test_radio_rc_channels(system_id, component_id, last_msg);
    mavlink_test_gnss_integrity(system_id, component_id, last_msg);
    mavlink_test_target_absolute(system_id, component_id, last_msg);
    mavlink_test_target_relative(system_id, component_id, last_msg);
    mavlink_test_control_status(system_id, component_id, last_msg);
    mavlink_test_esc_eeprom(system_id, component_id, last_msg);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // DEVELOPMENT_TESTSUITE_H
