// MESSAGE HEARTBEAT PACKING

#define MAVLINK_MSG_ID_HEARTBEAT 0

typedef struct __mavlink_heartbeat_t
{
 uint8_t type; ///< Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
 uint8_t autopilot; ///< Autopilot type / class. defined in MAV_CLASS ENUM
 uint8_t system_mode; ///< System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 uint8_t flight_mode; ///< Navigation mode, see MAV_FLIGHT_MODE ENUM
 uint8_t system_status; ///< System status flag, see MAV_STATUS ENUM
 uint8_t safety_status; ///< System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
 uint8_t link_status; ///< Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
 uint8_t mavlink_version; ///< MAVLink version
} mavlink_heartbeat_t;

#define MAVLINK_MSG_ID_HEARTBEAT_LEN 8
#define MAVLINK_MSG_ID_0_LEN 8



#define MAVLINK_MESSAGE_INFO_HEARTBEAT { \
	"HEARTBEAT", \
	8, \
	{  { "type", MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_heartbeat_t, type) }, \
         { "autopilot", MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_heartbeat_t, autopilot) }, \
         { "system_mode", MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_heartbeat_t, system_mode) }, \
         { "flight_mode", MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_heartbeat_t, flight_mode) }, \
         { "system_status", MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_heartbeat_t, system_status) }, \
         { "safety_status", MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_heartbeat_t, safety_status) }, \
         { "link_status", MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_heartbeat_t, link_status) }, \
         { "mavlink_version", MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_heartbeat_t, mavlink_version) }, \
         } \
}


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
static inline uint16_t mavlink_msg_heartbeat_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t type, uint8_t autopilot, uint8_t system_mode, uint8_t flight_mode, uint8_t system_status, uint8_t safety_status, uint8_t link_status)
{
	msg->msgid = MAVLINK_MSG_ID_HEARTBEAT;

	put_uint8_t_by_index(msg, 0, type); // Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	put_uint8_t_by_index(msg, 1, autopilot); // Autopilot type / class. defined in MAV_CLASS ENUM
	put_uint8_t_by_index(msg, 2, system_mode); // System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	put_uint8_t_by_index(msg, 3, flight_mode); // Navigation mode, see MAV_FLIGHT_MODE ENUM
	put_uint8_t_by_index(msg, 4, system_status); // System status flag, see MAV_STATUS ENUM
	put_uint8_t_by_index(msg, 5, safety_status); // System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	put_uint8_t_by_index(msg, 6, link_status); // Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
	put_uint8_t_by_index(msg, 7, 3); // MAVLink version

	return mavlink_finalize_message(msg, system_id, component_id, 8, 153);
}

/**
 * @brief Pack a heartbeat message on a channel
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
static inline uint16_t mavlink_msg_heartbeat_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t type,uint8_t autopilot,uint8_t system_mode,uint8_t flight_mode,uint8_t system_status,uint8_t safety_status,uint8_t link_status)
{
	msg->msgid = MAVLINK_MSG_ID_HEARTBEAT;

	put_uint8_t_by_index(msg, 0, type); // Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	put_uint8_t_by_index(msg, 1, autopilot); // Autopilot type / class. defined in MAV_CLASS ENUM
	put_uint8_t_by_index(msg, 2, system_mode); // System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	put_uint8_t_by_index(msg, 3, flight_mode); // Navigation mode, see MAV_FLIGHT_MODE ENUM
	put_uint8_t_by_index(msg, 4, system_status); // System status flag, see MAV_STATUS ENUM
	put_uint8_t_by_index(msg, 5, safety_status); // System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	put_uint8_t_by_index(msg, 6, link_status); // Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
	put_uint8_t_by_index(msg, 7, 3); // MAVLink version

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 8, 153);
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
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_heartbeat_send(mavlink_channel_t chan, uint8_t type, uint8_t autopilot, uint8_t system_mode, uint8_t flight_mode, uint8_t system_status, uint8_t safety_status, uint8_t link_status)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 8);
	msg->msgid = MAVLINK_MSG_ID_HEARTBEAT;

	put_uint8_t_by_index(msg, 0, type); // Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
	put_uint8_t_by_index(msg, 1, autopilot); // Autopilot type / class. defined in MAV_CLASS ENUM
	put_uint8_t_by_index(msg, 2, system_mode); // System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	put_uint8_t_by_index(msg, 3, flight_mode); // Navigation mode, see MAV_FLIGHT_MODE ENUM
	put_uint8_t_by_index(msg, 4, system_status); // System status flag, see MAV_STATUS ENUM
	put_uint8_t_by_index(msg, 5, safety_status); // System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
	put_uint8_t_by_index(msg, 6, link_status); // Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
	put_uint8_t_by_index(msg, 7, 3); // MAVLink version

	mavlink_finalize_message_chan_send(msg, chan, 8, 153);
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
	return MAVLINK_MSG_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field autopilot from heartbeat message
 *
 * @return Autopilot type / class. defined in MAV_CLASS ENUM
 */
static inline uint8_t mavlink_msg_heartbeat_get_autopilot(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field system_mode from heartbeat message
 *
 * @return System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 */
static inline uint8_t mavlink_msg_heartbeat_get_system_mode(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field flight_mode from heartbeat message
 *
 * @return Navigation mode, see MAV_FLIGHT_MODE ENUM
 */
static inline uint8_t mavlink_msg_heartbeat_get_flight_mode(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field system_status from heartbeat message
 *
 * @return System status flag, see MAV_STATUS ENUM
 */
static inline uint8_t mavlink_msg_heartbeat_get_system_status(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field safety_status from heartbeat message
 *
 * @return System safety lock state, see MAV_SAFETY enum. Also indicates HIL operation
 */
static inline uint8_t mavlink_msg_heartbeat_get_safety_status(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field link_status from heartbeat message
 *
 * @return Bitmask showing which links are ok / enabled. 0 for disabled/non functional, 1: enabled Indices: 0: RC, 1: UART1, 2: UART2, 3: UART3, 4: UART4, 5: UART5, 6: I2C, 7: CAN
 */
static inline uint8_t mavlink_msg_heartbeat_get_link_status(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Get field mavlink_version from heartbeat message
 *
 * @return MAVLink version
 */
static inline uint8_t mavlink_msg_heartbeat_get_mavlink_version(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  7);
}

/**
 * @brief Decode a heartbeat message into a struct
 *
 * @param msg The message to decode
 * @param heartbeat C-struct to decode the message contents into
 */
static inline void mavlink_msg_heartbeat_decode(const mavlink_message_t* msg, mavlink_heartbeat_t* heartbeat)
{
#if MAVLINK_NEED_BYTE_SWAP
	heartbeat->type = mavlink_msg_heartbeat_get_type(msg);
	heartbeat->autopilot = mavlink_msg_heartbeat_get_autopilot(msg);
	heartbeat->system_mode = mavlink_msg_heartbeat_get_system_mode(msg);
	heartbeat->flight_mode = mavlink_msg_heartbeat_get_flight_mode(msg);
	heartbeat->system_status = mavlink_msg_heartbeat_get_system_status(msg);
	heartbeat->safety_status = mavlink_msg_heartbeat_get_safety_status(msg);
	heartbeat->link_status = mavlink_msg_heartbeat_get_link_status(msg);
	heartbeat->mavlink_version = mavlink_msg_heartbeat_get_mavlink_version(msg);
#else
	memcpy(heartbeat, MAVLINK_PAYLOAD(msg), 8);
#endif
}
