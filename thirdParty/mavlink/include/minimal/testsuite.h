/** @file
 *	@brief MAVLink comm protocol testsuite generated from minimal.xml
 *	@see http://qgroundcontrol.org/mavlink/
 */
#ifndef MINIMAL_TESTSUITE_H
#define MINIMAL_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL

static void mavlink_test_minimal(uint8_t, uint8_t);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id)
{

	mavlink_test_minimal(system_id, component_id);
}
#endif




static void mavlink_test_heartbeat(uint8_t system_id, uint8_t component_id)
{
	mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
	mavlink_heartbeat_t packet2, packet1 = {
		5,
	72,
	139,
	206,
	17,
	84,
	151,
	2,
	};
	if (sizeof(packet2) != 8) {
		packet2 = packet1; // cope with alignment within the packet
	}
	mavlink_msg_heartbeat_encode(system_id, component_id, &msg, &packet1);
	mavlink_msg_heartbeat_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
	mavlink_msg_heartbeat_pack(system_id, component_id, &msg , packet1.type , packet1.autopilot , packet1.mode , packet1.nav_mode , packet1.status , packet1.safety_status , packet1.link_status );
	mavlink_msg_heartbeat_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.type , packet1.autopilot , packet1.mode , packet1.nav_mode , packet1.status , packet1.safety_status , packet1.link_status );
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
        	comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
	mavlink_msg_heartbeat_pack_chan_send(MAVLINK_COMM_1, &msg , packet1.type , packet1.autopilot , packet1.mode , packet1.nav_mode , packet1.status , packet1.safety_status , packet1.link_status );
	mavlink_msg_heartbeat_send(MAVLINK_COMM_2 , packet1.type , packet1.autopilot , packet1.mode , packet1.nav_mode , packet1.status , packet1.safety_status , packet1.link_status );
}

static void mavlink_test_minimal(uint8_t system_id, uint8_t component_id)
{
	mavlink_test_heartbeat(system_id, component_id);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MINIMAL_TESTSUITE_H
