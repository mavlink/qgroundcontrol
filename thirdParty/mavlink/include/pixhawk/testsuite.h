/** @file
 *	@brief MAVLink comm protocol testsuite generated from pixhawk.xml
 *	@see http://qgroundcontrol.org/mavlink/
 */
#ifndef PIXHAWK_TESTSUITE_H
#define PIXHAWK_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL
static void mavlink_test_common(uint8_t, uint8_t);
static void mavlink_test_pixhawk(uint8_t, uint8_t);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id)
{
	mavlink_test_common(system_id, component_id);
	mavlink_test_pixhawk(system_id, component_id);
}
#endif

#include "../common/testsuite.h"


static void mavlink_test_set_cam_shutter(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_set_cam_shutter_t packet2, packet1 = {
		17.0,
	17443,
	17547,
	29,
	96,
	163,
	};
	if (sizeof(packet2) != 11) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_set_cam_shutter_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_set_cam_shutter_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_set_cam_shutter_pack(system_id, component_id, &msg , packet1.cam_no , packet1.cam_mode , packet1.trigger_pin , packet1.interval , packet1.exposure , packet1.gain );
	mavlink_msg_set_cam_shutter_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.cam_no , packet1.cam_mode , packet1.trigger_pin , packet1.interval , packet1.exposure , packet1.gain );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_set_cam_shutter_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.cam_no , packet1.cam_mode , packet1.trigger_pin , packet1.interval , packet1.exposure , packet1.gain );
	mavlink_msg_set_cam_shutter_send(MAVLINK_COMM_2 , packet1.cam_no , packet1.cam_mode , packet1.trigger_pin , packet1.interval , packet1.exposure , packet1.gain );
}

static void mavlink_test_image_triggered(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_image_triggered_t packet2, packet1 = {
		93372036854775807ULL,
	963497880,
	101.0,
	129.0,
	157.0,
	185.0,
	213.0,
	241.0,
	269.0,
	297.0,
	325.0,
	353.0,
	};
	if (sizeof(packet2) != 52) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_image_triggered_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_image_triggered_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_image_triggered_pack(system_id, component_id, &msg , packet1.timestamp , packet1.seq , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
	mavlink_msg_image_triggered_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.timestamp , packet1.seq , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_image_triggered_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.timestamp , packet1.seq , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
	mavlink_msg_image_triggered_send(MAVLINK_COMM_2 , packet1.timestamp , packet1.seq , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
}

static void mavlink_test_image_trigger_control(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_image_trigger_control_t packet2, packet1 = {
		5,
	};
	if (sizeof(packet2) != 1) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_image_trigger_control_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_image_trigger_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_image_trigger_control_pack(system_id, component_id, &msg , packet1.enable );
	mavlink_msg_image_trigger_control_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.enable );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_image_trigger_control_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.enable );
	mavlink_msg_image_trigger_control_send(MAVLINK_COMM_2 , packet1.enable );
}

static void mavlink_test_image_available(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_image_available_t packet2, packet1 = {
		93372036854775807ULL,
	93372036854776311ULL,
	93372036854776815ULL,
	963498712,
	963498920,
	963499128,
	963499336,
	297.0,
	325.0,
	353.0,
	381.0,
	409.0,
	437.0,
	465.0,
	493.0,
	521.0,
	549.0,
	577.0,
	21603,
	21707,
	21811,
	147,
	214,
	};
	if (sizeof(packet2) != 92) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_image_available_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_image_available_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_image_available_pack(system_id, component_id, &msg , packet1.cam_id , packet1.cam_no , packet1.timestamp , packet1.valid_until , packet1.img_seq , packet1.img_buf_index , packet1.width , packet1.height , packet1.depth , packet1.channels , packet1.key , packet1.exposure , packet1.gain , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
	mavlink_msg_image_available_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.cam_id , packet1.cam_no , packet1.timestamp , packet1.valid_until , packet1.img_seq , packet1.img_buf_index , packet1.width , packet1.height , packet1.depth , packet1.channels , packet1.key , packet1.exposure , packet1.gain , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_image_available_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.cam_id , packet1.cam_no , packet1.timestamp , packet1.valid_until , packet1.img_seq , packet1.img_buf_index , packet1.width , packet1.height , packet1.depth , packet1.channels , packet1.key , packet1.exposure , packet1.gain , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
	mavlink_msg_image_available_send(MAVLINK_COMM_2 , packet1.cam_id , packet1.cam_no , packet1.timestamp , packet1.valid_until , packet1.img_seq , packet1.img_buf_index , packet1.width , packet1.height , packet1.depth , packet1.channels , packet1.key , packet1.exposure , packet1.gain , packet1.roll , packet1.pitch , packet1.yaw , packet1.local_z , packet1.lat , packet1.lon , packet1.alt , packet1.ground_x , packet1.ground_y , packet1.ground_z );
}

static void mavlink_test_vision_position_estimate(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_vision_position_estimate_t packet2, packet1 = {
		93372036854775807ULL,
	73.0,
	101.0,
	129.0,
	157.0,
	185.0,
	213.0,
	};
	if (sizeof(packet2) != 32) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_vision_position_estimate_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_vision_position_estimate_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_vision_position_estimate_pack(system_id, component_id, &msg , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
	mavlink_msg_vision_position_estimate_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_vision_position_estimate_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
	mavlink_msg_vision_position_estimate_send(MAVLINK_COMM_2 , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
}

static void mavlink_test_vicon_position_estimate(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_vicon_position_estimate_t packet2, packet1 = {
		93372036854775807ULL,
	73.0,
	101.0,
	129.0,
	157.0,
	185.0,
	213.0,
	};
	if (sizeof(packet2) != 32) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_vicon_position_estimate_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_vicon_position_estimate_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_vicon_position_estimate_pack(system_id, component_id, &msg , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
	mavlink_msg_vicon_position_estimate_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_vicon_position_estimate_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
	mavlink_msg_vicon_position_estimate_send(MAVLINK_COMM_2 , packet1.usec , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
}

static void mavlink_test_vision_speed_estimate(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_vision_speed_estimate_t packet2, packet1 = {
		93372036854775807ULL,
	73.0,
	101.0,
	129.0,
	};
	if (sizeof(packet2) != 20) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_vision_speed_estimate_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_vision_speed_estimate_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_vision_speed_estimate_pack(system_id, component_id, &msg , packet1.usec , packet1.x , packet1.y , packet1.z );
	mavlink_msg_vision_speed_estimate_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.usec , packet1.x , packet1.y , packet1.z );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_vision_speed_estimate_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.usec , packet1.x , packet1.y , packet1.z );
	mavlink_msg_vision_speed_estimate_send(MAVLINK_COMM_2 , packet1.usec , packet1.x , packet1.y , packet1.z );
}

static void mavlink_test_position_control_setpoint_set(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_position_control_setpoint_set_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	18067,
	187,
	254,
	};
	if (sizeof(packet2) != 20) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_position_control_setpoint_set_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_position_control_setpoint_set_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_position_control_setpoint_set_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
	mavlink_msg_position_control_setpoint_set_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_position_control_setpoint_set_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.target_system , packet1.target_component , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
	mavlink_msg_position_control_setpoint_set_send(MAVLINK_COMM_2 , packet1.target_system , packet1.target_component , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
}

static void mavlink_test_position_control_offset_set(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_position_control_offset_set_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	53,
	120,
	};
	if (sizeof(packet2) != 18) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_position_control_offset_set_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_position_control_offset_set_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_position_control_offset_set_pack(system_id, component_id, &msg , packet1.target_system , packet1.target_component , packet1.x , packet1.y , packet1.z , packet1.yaw );
	mavlink_msg_position_control_offset_set_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system , packet1.target_component , packet1.x , packet1.y , packet1.z , packet1.yaw );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_position_control_offset_set_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.target_system , packet1.target_component , packet1.x , packet1.y , packet1.z , packet1.yaw );
	mavlink_msg_position_control_offset_set_send(MAVLINK_COMM_2 , packet1.target_system , packet1.target_component , packet1.x , packet1.y , packet1.z , packet1.yaw );
}

static void mavlink_test_position_control_setpoint(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_position_control_setpoint_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	18067,
	};
	if (sizeof(packet2) != 18) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_position_control_setpoint_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_position_control_setpoint_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_position_control_setpoint_pack(system_id, component_id, &msg , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
	mavlink_msg_position_control_setpoint_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_position_control_setpoint_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
	mavlink_msg_position_control_setpoint_send(MAVLINK_COMM_2 , packet1.id , packet1.x , packet1.y , packet1.z , packet1.yaw );
}

static void mavlink_test_marker(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_marker_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	129.0,
	157.0,
	18483,
	};
	if (sizeof(packet2) != 26) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_marker_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_marker_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_marker_pack(system_id, component_id, &msg , packet1.id , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
	mavlink_msg_marker_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.id , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_marker_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.id , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
	mavlink_msg_marker_send(MAVLINK_COMM_2 , packet1.id , packet1.x , packet1.y , packet1.z , packet1.roll , packet1.pitch , packet1.yaw );
}

static void mavlink_test_raw_aux(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_raw_aux_t packet2, packet1 = {
		963497464,
	17443,
	17547,
	17651,
	17755,
	17859,
	17963,
	};
	if (sizeof(packet2) != 16) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_raw_aux_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_raw_aux_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_raw_aux_pack(system_id, component_id, &msg , packet1.adc1 , packet1.adc2 , packet1.adc3 , packet1.adc4 , packet1.vbat , packet1.temp , packet1.baro );
	mavlink_msg_raw_aux_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.adc1 , packet1.adc2 , packet1.adc3 , packet1.adc4 , packet1.vbat , packet1.temp , packet1.baro );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_raw_aux_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.adc1 , packet1.adc2 , packet1.adc3 , packet1.adc4 , packet1.vbat , packet1.temp , packet1.baro );
	mavlink_msg_raw_aux_send(MAVLINK_COMM_2 , packet1.adc1 , packet1.adc2 , packet1.adc3 , packet1.adc4 , packet1.vbat , packet1.temp , packet1.baro );
}

static void mavlink_test_watchdog_heartbeat(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_watchdog_heartbeat_t packet2, packet1 = {
		17235,
	17339,
	};
	if (sizeof(packet2) != 4) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_watchdog_heartbeat_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_watchdog_heartbeat_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_watchdog_heartbeat_pack(system_id, component_id, &msg , packet1.watchdog_id , packet1.process_count );
	mavlink_msg_watchdog_heartbeat_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.watchdog_id , packet1.process_count );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_watchdog_heartbeat_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.watchdog_id , packet1.process_count );
	mavlink_msg_watchdog_heartbeat_send(MAVLINK_COMM_2 , packet1.watchdog_id , packet1.process_count );
}

static void mavlink_test_watchdog_process_info(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_watchdog_process_info_t packet2, packet1 = {
		963497464,
	17443,
	17547,
	"IJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC",
	"EFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRST",
	};
	if (sizeof(packet2) != 255) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_watchdog_process_info_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_watchdog_process_info_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_watchdog_process_info_pack(system_id, component_id, &msg , packet1.watchdog_id , packet1.process_id , packet1.name , packet1.arguments , packet1.timeout );
	mavlink_msg_watchdog_process_info_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.watchdog_id , packet1.process_id , packet1.name , packet1.arguments , packet1.timeout );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_watchdog_process_info_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.watchdog_id , packet1.process_id , packet1.name , packet1.arguments , packet1.timeout );
	mavlink_msg_watchdog_process_info_send(MAVLINK_COMM_2 , packet1.watchdog_id , packet1.process_id , packet1.name , packet1.arguments , packet1.timeout );
}

static void mavlink_test_watchdog_process_status(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_watchdog_process_status_t packet2, packet1 = {
		963497464,
	17443,
	17547,
	17651,
	163,
	230,
	};
	if (sizeof(packet2) != 12) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_watchdog_process_status_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_watchdog_process_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_watchdog_process_status_pack(system_id, component_id, &msg , packet1.watchdog_id , packet1.process_id , packet1.state , packet1.muted , packet1.pid , packet1.crashes );
	mavlink_msg_watchdog_process_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.watchdog_id , packet1.process_id , packet1.state , packet1.muted , packet1.pid , packet1.crashes );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_watchdog_process_status_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.watchdog_id , packet1.process_id , packet1.state , packet1.muted , packet1.pid , packet1.crashes );
	mavlink_msg_watchdog_process_status_send(MAVLINK_COMM_2 , packet1.watchdog_id , packet1.process_id , packet1.state , packet1.muted , packet1.pid , packet1.crashes );
}

static void mavlink_test_watchdog_command(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_watchdog_command_t packet2, packet1 = {
		17235,
	17339,
	17,
	84,
	};
	if (sizeof(packet2) != 6) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_watchdog_command_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_watchdog_command_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_watchdog_command_pack(system_id, component_id, &msg , packet1.target_system_id , packet1.watchdog_id , packet1.process_id , packet1.command_id );
	mavlink_msg_watchdog_command_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target_system_id , packet1.watchdog_id , packet1.process_id , packet1.command_id );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_watchdog_command_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.target_system_id , packet1.watchdog_id , packet1.process_id , packet1.command_id );
	mavlink_msg_watchdog_command_send(MAVLINK_COMM_2 , packet1.target_system_id , packet1.watchdog_id , packet1.process_id , packet1.command_id );
}

static void mavlink_test_pattern_detected(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_pattern_detected_t packet2, packet1 = {
		17.0,
	17,
	"FGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ",
	128,
	};
	if (sizeof(packet2) != 106) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_pattern_detected_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_pattern_detected_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_pattern_detected_pack(system_id, component_id, &msg , packet1.type , packet1.confidence , packet1.file , packet1.detected );
	mavlink_msg_pattern_detected_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.type , packet1.confidence , packet1.file , packet1.detected );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_pattern_detected_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.type , packet1.confidence , packet1.file , packet1.detected );
	mavlink_msg_pattern_detected_send(MAVLINK_COMM_2 , packet1.type , packet1.confidence , packet1.file , packet1.detected );
}

static void mavlink_test_point_of_interest(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_point_of_interest_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	17859,
	175,
	242,
	53,
	"RSTUVWXYZABCDEFGHIJKLMNOP",
	};
	if (sizeof(packet2) != 43) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_point_of_interest_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_point_of_interest_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_point_of_interest_pack(system_id, component_id, &msg , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.x , packet1.y , packet1.z , packet1.name );
	mavlink_msg_point_of_interest_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.x , packet1.y , packet1.z , packet1.name );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_point_of_interest_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.x , packet1.y , packet1.z , packet1.name );
	mavlink_msg_point_of_interest_send(MAVLINK_COMM_2 , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.x , packet1.y , packet1.z , packet1.name );
}

static void mavlink_test_point_of_interest_connection(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_point_of_interest_connection_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	129.0,
	157.0,
	18483,
	211,
	22,
	89,
	"DEFGHIJKLMNOPQRSTUVWXYZAB",
	};
	if (sizeof(packet2) != 55) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_point_of_interest_connection_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_point_of_interest_connection_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_point_of_interest_connection_pack(system_id, component_id, &msg , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.xp1 , packet1.yp1 , packet1.zp1 , packet1.xp2 , packet1.yp2 , packet1.zp2 , packet1.name );
	mavlink_msg_point_of_interest_connection_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.xp1 , packet1.yp1 , packet1.zp1 , packet1.xp2 , packet1.yp2 , packet1.zp2 , packet1.name );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_point_of_interest_connection_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.xp1 , packet1.yp1 , packet1.zp1 , packet1.xp2 , packet1.yp2 , packet1.zp2 , packet1.name );
	mavlink_msg_point_of_interest_connection_send(MAVLINK_COMM_2 , packet1.type , packet1.color , packet1.coordinate_system , packet1.timeout , packet1.xp1 , packet1.yp1 , packet1.zp1 , packet1.xp2 , packet1.yp2 , packet1.zp2 , packet1.name );
}

static void mavlink_test_data_transmission_handshake(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_data_transmission_handshake_t packet2, packet1 = {
		963497464,
	17,
	84,
	151,
	218,
	};
	if (sizeof(packet2) != 8) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_data_transmission_handshake_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_data_transmission_handshake_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_data_transmission_handshake_pack(system_id, component_id, &msg , packet1.type , packet1.size , packet1.packets , packet1.payload , packet1.jpg_quality );
	mavlink_msg_data_transmission_handshake_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.type , packet1.size , packet1.packets , packet1.payload , packet1.jpg_quality );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_data_transmission_handshake_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.type , packet1.size , packet1.packets , packet1.payload , packet1.jpg_quality );
	mavlink_msg_data_transmission_handshake_send(MAVLINK_COMM_2 , packet1.type , packet1.size , packet1.packets , packet1.payload , packet1.jpg_quality );
}

static void mavlink_test_encapsulated_data(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_encapsulated_data_t packet2, packet1 = {
		17235,
	{ 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135 },
	};
	if (sizeof(packet2) != 255) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_encapsulated_data_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_encapsulated_data_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_encapsulated_data_pack(system_id, component_id, &msg , packet1.seqnr , packet1.data );
	mavlink_msg_encapsulated_data_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.seqnr , packet1.data );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_encapsulated_data_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.seqnr , packet1.data );
	mavlink_msg_encapsulated_data_send(MAVLINK_COMM_2 , packet1.seqnr , packet1.data );
}

static void mavlink_test_brief_feature(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_brief_feature_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	18067,
	18171,
	65,
	{ 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163 },
	};
	if (sizeof(packet2) != 53) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_brief_feature_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_brief_feature_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_brief_feature_pack(system_id, component_id, &msg , packet1.x , packet1.y , packet1.z , packet1.orientation_assignment , packet1.size , packet1.orientation , packet1.descriptor , packet1.response );
	mavlink_msg_brief_feature_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.x , packet1.y , packet1.z , packet1.orientation_assignment , packet1.size , packet1.orientation , packet1.descriptor , packet1.response );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_brief_feature_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.x , packet1.y , packet1.z , packet1.orientation_assignment , packet1.size , packet1.orientation , packet1.descriptor , packet1.response );
	mavlink_msg_brief_feature_send(MAVLINK_COMM_2 , packet1.x , packet1.y , packet1.z , packet1.orientation_assignment , packet1.size , packet1.orientation , packet1.descriptor , packet1.response );
}

static void mavlink_test_attitude_control(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_attitude_control_t packet2, packet1 = {
		17.0,
	45.0,
	73.0,
	101.0,
	53,
	120,
	187,
	254,
	65,
	};
	if (sizeof(packet2) != 21) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_attitude_control_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_attitude_control_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_attitude_control_pack(system_id, component_id, &msg , packet1.target , packet1.roll , packet1.pitch , packet1.yaw , packet1.thrust , packet1.roll_manual , packet1.pitch_manual , packet1.yaw_manual , packet1.thrust_manual );
	mavlink_msg_attitude_control_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.target , packet1.roll , packet1.pitch , packet1.yaw , packet1.thrust , packet1.roll_manual , packet1.pitch_manual , packet1.yaw_manual , packet1.thrust_manual );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_attitude_control_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.target , packet1.roll , packet1.pitch , packet1.yaw , packet1.thrust , packet1.roll_manual , packet1.pitch_manual , packet1.yaw_manual , packet1.thrust_manual );
	mavlink_msg_attitude_control_send(MAVLINK_COMM_2 , packet1.target , packet1.roll , packet1.pitch , packet1.yaw , packet1.thrust , packet1.roll_manual , packet1.pitch_manual , packet1.yaw_manual , packet1.thrust_manual );
}

static void mavlink_test_pixhawk(uint8_t system_id, uint8_t component_id)
{
	mavlink_test_set_cam_shutter(system_id, component_id);
	mavlink_test_image_triggered(system_id, component_id);
	mavlink_test_image_trigger_control(system_id, component_id);
	mavlink_test_image_available(system_id, component_id);
	mavlink_test_vision_position_estimate(system_id, component_id);
	mavlink_test_vicon_position_estimate(system_id, component_id);
	mavlink_test_vision_speed_estimate(system_id, component_id);
	mavlink_test_position_control_setpoint_set(system_id, component_id);
	mavlink_test_position_control_offset_set(system_id, component_id);
	mavlink_test_position_control_setpoint(system_id, component_id);
	mavlink_test_marker(system_id, component_id);
	mavlink_test_raw_aux(system_id, component_id);
	mavlink_test_watchdog_heartbeat(system_id, component_id);
	mavlink_test_watchdog_process_info(system_id, component_id);
	mavlink_test_watchdog_process_status(system_id, component_id);
	mavlink_test_watchdog_command(system_id, component_id);
	mavlink_test_pattern_detected(system_id, component_id);
	mavlink_test_point_of_interest(system_id, component_id);
	mavlink_test_point_of_interest_connection(system_id, component_id);
	mavlink_test_data_transmission_handshake(system_id, component_id);
	mavlink_test_encapsulated_data(system_id, component_id);
	mavlink_test_brief_feature(system_id, component_id);
	mavlink_test_attitude_control(system_id, component_id);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // PIXHAWK_TESTSUITE_H
