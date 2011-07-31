// MESSAGE WAYPOINT PACKING

#define MAVLINK_MSG_ID_WAYPOINT 39
#define MAVLINK_MSG_ID_WAYPOINT_LEN 36
#define MAVLINK_MSG_39_LEN 36

typedef struct __mavlink_waypoint_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	uint16_t seq; ///< Sequence
	uint8_t frame; ///< The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
	uint8_t command; ///< The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
	uint8_t current; ///< false:0, true:1
	uint8_t autocontinue; ///< autocontinue to next wp
	float param1; ///< PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
	float param2; ///< PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
	float param3; ///< PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
	float param4; ///< PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
	float x; ///< PARAM5 / local: x position, global: latitude
	float y; ///< PARAM6 / y position: global: longitude
	float z; ///< PARAM7 / z position: global: altitude

} mavlink_waypoint_t;

/**
 * @brief Pack a waypoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param seq Sequence
 * @param frame The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
 * @param command The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
 * @param current false:0, true:1
 * @param autocontinue autocontinue to next wp
 * @param param1 PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
 * @param param2 PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
 * @param param3 PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
 * @param param4 PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
 * @param x PARAM5 / local: x position, global: latitude
 * @param y PARAM6 / y position: global: longitude
 * @param z PARAM7 / z position: global: altitude
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint16_t seq, uint8_t frame, uint8_t command, uint8_t current, uint8_t autocontinue, float param1, float param2, float param3, float param4, float x, float y, float z)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->seq = seq; // uint16_t:Sequence
	p->frame = frame; // uint8_t:The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
	p->command = command; // uint8_t:The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
	p->current = current; // uint8_t:false:0, true:1
	p->autocontinue = autocontinue; // uint8_t:autocontinue to next wp
	p->param1 = param1; // float:PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
	p->param2 = param2; // float:PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
	p->param3 = param3; // float:PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
	p->param4 = param4; // float:PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
	p->x = x; // float:PARAM5 / local: x position, global: latitude
	p->y = y; // float:PARAM6 / y position: global: longitude
	p->z = z; // float:PARAM7 / z position: global: altitude

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WAYPOINT_LEN);
}

/**
 * @brief Pack a waypoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param seq Sequence
 * @param frame The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
 * @param command The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
 * @param current false:0, true:1
 * @param autocontinue autocontinue to next wp
 * @param param1 PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
 * @param param2 PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
 * @param param3 PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
 * @param param4 PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
 * @param x PARAM5 / local: x position, global: latitude
 * @param y PARAM6 / y position: global: longitude
 * @param z PARAM7 / z position: global: altitude
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint16_t seq, uint8_t frame, uint8_t command, uint8_t current, uint8_t autocontinue, float param1, float param2, float param3, float param4, float x, float y, float z)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->seq = seq; // uint16_t:Sequence
	p->frame = frame; // uint8_t:The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
	p->command = command; // uint8_t:The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
	p->current = current; // uint8_t:false:0, true:1
	p->autocontinue = autocontinue; // uint8_t:autocontinue to next wp
	p->param1 = param1; // float:PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
	p->param2 = param2; // float:PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
	p->param3 = param3; // float:PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
	p->param4 = param4; // float:PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
	p->x = x; // float:PARAM5 / local: x position, global: latitude
	p->y = y; // float:PARAM6 / y position: global: longitude
	p->z = z; // float:PARAM7 / z position: global: altitude

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WAYPOINT_LEN);
}

/**
 * @brief Encode a waypoint struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param waypoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_waypoint_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_waypoint_t* waypoint)
{
	return mavlink_msg_waypoint_pack(system_id, component_id, msg, waypoint->target_system, waypoint->target_component, waypoint->seq, waypoint->frame, waypoint->command, waypoint->current, waypoint->autocontinue, waypoint->param1, waypoint->param2, waypoint->param3, waypoint->param4, waypoint->x, waypoint->y, waypoint->z);
}

/**
 * @brief Send a waypoint message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param seq Sequence
 * @param frame The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
 * @param command The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
 * @param current false:0, true:1
 * @param autocontinue autocontinue to next wp
 * @param param1 PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
 * @param param2 PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
 * @param param3 PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
 * @param param4 PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
 * @param x PARAM5 / local: x position, global: latitude
 * @param y PARAM6 / y position: global: longitude
 * @param z PARAM7 / z position: global: altitude
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_waypoint_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t seq, uint8_t frame, uint8_t command, uint8_t current, uint8_t autocontinue, float param1, float param2, float param3, float param4, float x, float y, float z)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg.payload[0];

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->seq = seq; // uint16_t:Sequence
	p->frame = frame; // uint8_t:The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
	p->command = command; // uint8_t:The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
	p->current = current; // uint8_t:false:0, true:1
	p->autocontinue = autocontinue; // uint8_t:autocontinue to next wp
	p->param1 = param1; // float:PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
	p->param2 = param2; // float:PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
	p->param3 = param3; // float:PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
	p->param4 = param4; // float:PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
	p->x = x; // float:PARAM5 / local: x position, global: latitude
	p->y = y; // float:PARAM6 / y position: global: longitude
	p->z = z; // float:PARAM7 / z position: global: altitude

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_WAYPOINT_LEN;
	msg.msgid = MAVLINK_MSG_ID_WAYPOINT;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_waypoint_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t seq, uint8_t frame, uint8_t command, uint8_t current, uint8_t autocontinue, float param1, float param2, float param3, float param4, float x, float y, float z)
{
	mavlink_header_t hdr;
	mavlink_waypoint_t payload;
	uint16_t checksum;
	mavlink_waypoint_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->seq = seq; // uint16_t:Sequence
	p->frame = frame; // uint8_t:The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
	p->command = command; // uint8_t:The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
	p->current = current; // uint8_t:false:0, true:1
	p->autocontinue = autocontinue; // uint8_t:autocontinue to next wp
	p->param1 = param1; // float:PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
	p->param2 = param2; // float:PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
	p->param3 = param3; // float:PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
	p->param4 = param4; // float:PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
	p->x = x; // float:PARAM5 / local: x position, global: latitude
	p->y = y; // float:PARAM6 / y position: global: longitude
	p->z = z; // float:PARAM7 / z position: global: altitude

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WAYPOINT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WAYPOINT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&checksum);
	checksum = crc_calculate_mem((uint8_t *)&hdr.len, &checksum, MAVLINK_CORE_HEADER_LEN);
	checksum = crc_calculate_mem((uint8_t *)&payload, &checksum, hdr.len );
	hdr.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	hdr.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_mem(chan, (uint8_t *)&payload, hdr.len);
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck_a, MAVLINK_NUM_CHECKSUM_BYTES);
}

#endif
// MESSAGE WAYPOINT UNPACKING

/**
 * @brief Get field target_system from waypoint message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_waypoint_get_target_system(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from waypoint message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_waypoint_get_target_component(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field seq from waypoint message
 *
 * @return Sequence
 */
static inline uint16_t mavlink_msg_waypoint_get_seq(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint16_t)(p->seq);
}

/**
 * @brief Get field frame from waypoint message
 *
 * @return The coordinate system of the waypoint. see MAV_FRAME in mavlink_types.h
 */
static inline uint8_t mavlink_msg_waypoint_get_frame(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint8_t)(p->frame);
}

/**
 * @brief Get field command from waypoint message
 *
 * @return The scheduled action for the waypoint. see MAV_COMMAND in common.xml MAVLink specs
 */
static inline uint8_t mavlink_msg_waypoint_get_command(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint8_t)(p->command);
}

/**
 * @brief Get field current from waypoint message
 *
 * @return false:0, true:1
 */
static inline uint8_t mavlink_msg_waypoint_get_current(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint8_t)(p->current);
}

/**
 * @brief Get field autocontinue from waypoint message
 *
 * @return autocontinue to next wp
 */
static inline uint8_t mavlink_msg_waypoint_get_autocontinue(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (uint8_t)(p->autocontinue);
}

/**
 * @brief Get field param1 from waypoint message
 *
 * @return PARAM1 / For NAV command waypoints: Radius in which the waypoint is accepted as reached, in meters
 */
static inline float mavlink_msg_waypoint_get_param1(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->param1);
}

/**
 * @brief Get field param2 from waypoint message
 *
 * @return PARAM2 / For NAV command waypoints: Time that the MAV should stay inside the PARAM1 radius before advancing, in milliseconds
 */
static inline float mavlink_msg_waypoint_get_param2(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->param2);
}

/**
 * @brief Get field param3 from waypoint message
 *
 * @return PARAM3 / For LOITER command waypoints: Orbit to circle around the waypoint, in meters. If positive the orbit direction should be clockwise, if negative the orbit direction should be counter-clockwise.
 */
static inline float mavlink_msg_waypoint_get_param3(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->param3);
}

/**
 * @brief Get field param4 from waypoint message
 *
 * @return PARAM4 / For NAV and LOITER command waypoints: Yaw orientation in degrees, [0..360] 0 = NORTH
 */
static inline float mavlink_msg_waypoint_get_param4(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->param4);
}

/**
 * @brief Get field x from waypoint message
 *
 * @return PARAM5 / local: x position, global: latitude
 */
static inline float mavlink_msg_waypoint_get_x(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from waypoint message
 *
 * @return PARAM6 / y position: global: longitude
 */
static inline float mavlink_msg_waypoint_get_y(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from waypoint message
 *
 * @return PARAM7 / z position: global: altitude
 */
static inline float mavlink_msg_waypoint_get_z(const mavlink_message_t* msg)
{
	mavlink_waypoint_t *p = (mavlink_waypoint_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Decode a waypoint message into a struct
 *
 * @param msg The message to decode
 * @param waypoint C-struct to decode the message contents into
 */
static inline void mavlink_msg_waypoint_decode(const mavlink_message_t* msg, mavlink_waypoint_t* waypoint)
{
	memcpy( waypoint, msg->payload, sizeof(mavlink_waypoint_t));
}
