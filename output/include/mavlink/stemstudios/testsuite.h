/** @file
 *    @brief MAVLink comm protocol testsuite generated from stemstudios.xml
 *    @see https://mavlink.io/en/
 */
#pragma once
#ifndef STEMSTUDIOS_TESTSUITE_H
#define STEMSTUDIOS_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL
static void mavlink_test_common(uint8_t, uint8_t, mavlink_message_t *last_msg);
static void mavlink_test_stemstudios(uint8_t, uint8_t, mavlink_message_t *last_msg);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_common(system_id, component_id, last_msg);
    mavlink_test_stemstudios(system_id, component_id, last_msg);
}
#endif

#include "../common/testsuite.h"


static void mavlink_test_led_strip_config(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_LED_STRIP_CONFIG >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_led_strip_config_t packet_in = {
        { 963497464, 963497465, 963497466, 963497467, 963497468, 963497469, 963497470, 963497471 },101,168,235,46,113,180
    };
    mavlink_led_strip_config_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.mode = packet_in.mode;
        packet1.index = packet_in.index;
        packet1.length = packet_in.length;
        packet1.id = packet_in.id;
        
        mav_array_memcpy(packet1.colors, packet_in.colors, sizeof(uint32_t)*8);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_config_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_led_strip_config_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_config_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.mode , packet1.index , packet1.length , packet1.id , packet1.colors );
    mavlink_msg_led_strip_config_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_config_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.mode , packet1.index , packet1.length , packet1.id , packet1.colors );
    mavlink_msg_led_strip_config_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_led_strip_config_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_config_send(MAVLINK_COMM_1 , packet1.target_system , packet1.target_component , packet1.mode , packet1.index , packet1.length , packet1.id , packet1.colors );
    mavlink_msg_led_strip_config_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("LED_STRIP_CONFIG") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_LED_STRIP_CONFIG) != NULL);
#endif
}

static void mavlink_test_led_strip_state(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_LED_STRIP_STATE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_led_strip_state_t packet_in = {
        { 963497464, 963497465, 963497466, 963497467, 963497468, 963497469, 963497470, 963497471 },101,168,235,46
    };
    mavlink_led_strip_state_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.length = packet_in.length;
        packet1.index = packet_in.index;
        packet1.id = packet_in.id;
        packet1.following_flight_mode = packet_in.following_flight_mode;
        
        mav_array_memcpy(packet1.colors, packet_in.colors, sizeof(uint32_t)*8);
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_state_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_led_strip_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_state_pack(system_id, component_id, &msg , packet1.length , packet1.index , packet1.id , packet1.following_flight_mode , packet1.colors );
    mavlink_msg_led_strip_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_state_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.length , packet1.index , packet1.id , packet1.following_flight_mode , packet1.colors );
    mavlink_msg_led_strip_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_led_strip_state_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_led_strip_state_send(MAVLINK_COMM_1 , packet1.length , packet1.index , packet1.id , packet1.following_flight_mode , packet1.colors );
    mavlink_msg_led_strip_state_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("LED_STRIP_STATE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_LED_STRIP_STATE) != NULL);
#endif
}

static void mavlink_test_stemstudios(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_led_strip_config(system_id, component_id, last_msg);
    mavlink_test_led_strip_state(system_id, component_id, last_msg);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // STEMSTUDIOS_TESTSUITE_H
