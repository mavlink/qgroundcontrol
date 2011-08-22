// MESSAGE HEARTBEAT PACKING

#define MAVLINK_MSG_ID_HEARTBEAT 0
#define MAVLINK_MSG_ID_HEARTBEAT_LEN 8
#define MAVLINK_MSG_0_LEN 8
#define MAVLINK_MSG_ID_HEARTBEAT_KEY 0xA
#define MAVLINK_MSG_0_KEY 0xA

typedef struct __mavlink_heartbeat_t 
{
	uint8_t type;	///< Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	uint8_t autopilot;	///< Autopilot type / class. defined in MAV_CLASS ENUM
	uint8_t system_mode;	///< System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	uint8_t flight_mode;	///< Navigation mode, see MAV_FLIGHT_MODE ENUM
	uint8_t system_status;	///< System status flag, see MAV_STATUS ENUM
	uint8_t safety_status;	///< System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	uint8_t link_status;	///< Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
	uint8_t mavlink_version;	///< MAVLink version

} mavlink_heartbeat_t;

/**
 * @brief Pack a heartbeat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param type Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
 * @param autopilot Autopilot type / class. defined in MAV_CLASS ENUM
 * @param system_mode System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 * @param flight_mode Navigation mode, see MAV_FLIGHT_MODE ENUM
 * @param system_status System status flag, see MAV_STATUS ENUM
 * @param safety_status System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
 * @param link_status Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_heartbeat_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t type, uint8_t autopilot, uint8_t system_mode, uint8_t flight_mode, uint8_t system_status, uint8_t safety_status, uint8_t link_status)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HEARTBEAT;

	p->type = type;	// uint8_t:Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	p->autopilot = autopilot;	// uint8_t:Autopilot type / class. defined in MAV_CLASS ENUM
	p->system_mode = system_mode;	// uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	p->flight_mode = flight_mode;	// uint8_t:Navigation mode, see MAV_FLIGHT_MODE ENUM
	p->system_status = system_status;	// uint8_t:System status flag, see MAV_STATUS ENUM
	p->safety_status = safety_status;	// uint8_t:System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	p->link_status = link_status;	// uint8_t:Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN

	p->mavlink_version = MAVLINK_VERSION;	// uint8_t_mavlink_version:MAVLink version
	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HEARTBEAT_LEN);
}

/**
 * @brief Pack a heartbeat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param type Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
 * @param autopilot Autopilot type / class. defined in MAV_CLASS ENUM
 * @param system_mode System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 * @param flight_mode Navigation mode, see MAV_FLIGHT_MODE ENUM
 * @param system_status System status flag, see MAV_STATUS ENUM
 * @param safety_status System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
 * @param link_status Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_heartbeat_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t type, uint8_t autopilot, uint8_t system_mode, uint8_t flight_mode, uint8_t system_status, uint8_t safety_status, uint8_t link_status)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HEARTBEAT;

	p->type = type;	// uint8_t:Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	p->autopilot = autopilot;	// uint8_t:Autopilot type / class. defined in MAV_CLASS ENUM
	p->system_mode = system_mode;	// uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	p->flight_mode = flight_mode;	// uint8_t:Navigation mode, see MAV_FLIGHT_MODE ENUM
	p->system_status = system_status;	// uint8_t:System status flag, see MAV_STATUS ENUM
	p->safety_status = safety_status;	// uint8_t:System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	p->link_status = link_status;	// uint8_t:Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN

	p->mavlink_version = MAVLINK_VERSION;	// uint8_t_mavlink_version:MAVLink version
	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HEARTBEAT_LEN);
}

/**
 * @brief Encode a heartbeat struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param heartbeat C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_heartbeat_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_heartbeat_t* heartbeat)
{
	return mavlink_msg_heartbeat_pack(system_id, component_id, msg, heartbeat->type, heartbeat->autopilot, heartbeat->system_mode, heartbeat->flight_mode, heartbeat->system_status, heartbeat->safety_status, heartbeat->link_status);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a heartbeat message
 * @param chan MAVLink channel to send the message
 *
 * @param type Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
 * @param autopilot Autopilot type / class. defined in MAV_CLASS ENUM
 * @param system_mode System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 * @param flight_mode Navigation mode, see MAV_FLIGHT_MODE ENUM
 * @param system_status System status flag, see MAV_STATUS ENUM
 * @param safety_status System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
 * @param link_status Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
 */
static inline void mavlink_msg_heartbeat_send(mavlink_channel_t chan, uint8_t type, uint8_t autopilot, uint8_t system_mode, uint8_t flight_mode, uint8_t system_status, uint8_t safety_status, uint8_t link_status)
{
	mavlink_header_t hdr;
	mavlink_heartbeat_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_HEARTBEAT_LEN )
	payload.type = type;	// uint8_t:Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	payload.autopilot = autopilot;	// uint8_t:Autopilot type / class. defined in MAV_CLASS ENUM
	payload.system_mode = system_mode;	// uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	payload.flight_mode = flight_mode;	// uint8_t:Navigation mode, see MAV_FLIGHT_MODE ENUM
	payload.system_status = system_status;	// uint8_t:System status flag, see MAV_STATUS ENUM
	payload.safety_status = safety_status;	// uint8_t:System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	payload.link_status = link_status;	// uint8_t:Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN

	payload.mavlink_version = MAVLINK_VERSION;	// uint8_t_mavlink_version:MAVLink version
	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_HEARTBEAT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_HEARTBEAT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xA, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE HEARTBEAT UNPACKING

/**
 * @brief Get field type from heartbeat message
 *
 * @return Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
 */
static inline uint8_t mavlink_msg_heartbeat_get_type(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->type);
}

/**
 * @brief Get field autopilot from heartbeat message
 *
 * @return Autopilot type / class. defined in MAV_CLASS ENUM
 */
static inline uint8_t mavlink_msg_heartbeat_get_autopilot(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->autopilot);
}

/**
 * @brief Get field system_mode from heartbeat message
 *
 * @return System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 */
static inline uint8_t mavlink_msg_heartbeat_get_system_mode(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->system_mode);
}

/**
 * @brief Get field flight_mode from heartbeat message
 *
 * @return Navigation mode, see MAV_FLIGHT_MODE ENUM
 */
static inline uint8_t mavlink_msg_heartbeat_get_flight_mode(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->flight_mode);
}

/**
 * @brief Get field system_status from heartbeat message
 *
 * @return System status flag, see MAV_STATUS ENUM
 */
static inline uint8_t mavlink_msg_heartbeat_get_system_status(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->system_status);
}

/**
 * @brief Get field safety_status from heartbeat message
 *
 * @return System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
 */
static inline uint8_t mavlink_msg_heartbeat_get_safety_status(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->safety_status);
}

/**
 * @brief Get field link_status from heartbeat message
 *
 * @return Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
 */
static inline uint8_t mavlink_msg_heartbeat_get_link_status(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->link_status);
}

/**
 * @brief Get field mavlink_version from heartbeat message
 *
 * @return MAVLink version
 */
static inline uint8_t mavlink_msg_heartbeat_get_mavlink_version(const mavlink_message_t* msg)
{
	mavlink_heartbeat_t *p = (mavlink_heartbeat_t *)&msg->payload[0];
	return (uint8_t)(p->mavlink_version);
}

/**
 * @brief Decode a heartbeat message into a struct
 *
 * @param msg The message to decode
 * @param heartbeat C-struct to decode the message contents into
 */
static inline void mavlink_msg_heartbeat_decode(const mavlink_message_t* msg, mavlink_heartbeat_t* heartbeat)
{
	memcpy( heartbeat, msg->payload, sizeof(mavlink_heartbeat_t));
}
