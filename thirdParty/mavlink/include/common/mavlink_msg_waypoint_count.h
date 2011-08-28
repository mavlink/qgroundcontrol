// MESSAGE WAYPOINT_COUNT PACKING

#define MAVLINK_MSG_ID_WAYPOINT_COUNT 44

typedef struct __mavlink_waypoint_count_t
{
 uint16_t count; ///< Number of Waypoints in the Sequence
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_waypoint_count_t;

#define MAVLINK_MSG_ID_WAYPOINT_COUNT_LEN 4
#define MAVLINK_MSG_ID_44_LEN 4



#define MAVLINK_MESSAGE_INFO_WAYPOINT_COUNT { \
	"WAYPOINT_COUNT", \
	3, \
	{  { "count", MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_waypoint_count_t, count) }, \
         { "target_system", MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_waypoint_count_t, target_system) }, \
         { "target_component", MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_waypoint_count_t, target_component) }, \
         } \
}


/**
 * @brief Pack a waypoint_count message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param count Number of Waypoints in the Sequence
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_count_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, uint16_t count)
{
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_COUNT;

	put_uint16_t_by_index(msg, 0, count); // Number of Waypoints in the Sequence
	put_uint8_t_by_index(msg, 2, target_system); // System ID
	put_uint8_t_by_index(msg, 3, target_component); // Component ID

	return mavlink_finalize_message(msg, system_id, component_id, 4, 8);
}

/**
 * @brief Pack a waypoint_count message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param count Number of Waypoints in the Sequence
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_count_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,uint16_t count)
{
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_COUNT;

	put_uint16_t_by_index(msg, 0, count); // Number of Waypoints in the Sequence
	put_uint8_t_by_index(msg, 2, target_system); // System ID
	put_uint8_t_by_index(msg, 3, target_component); // Component ID

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 4, 8);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a waypoint_count message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param count Number of Waypoints in the Sequence
 */
static inline void mavlink_msg_waypoint_count_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,uint16_t count)
{
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_COUNT;

	put_uint16_t_by_index(msg, 0, count); // Number of Waypoints in the Sequence
	put_uint8_t_by_index(msg, 2, target_system); // System ID
	put_uint8_t_by_index(msg, 3, target_component); // Component ID

	mavlink_finalize_message_chan_send(msg, chan, 4, 8);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a waypoint_count struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param waypoint_count C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_waypoint_count_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_waypoint_count_t* waypoint_count)
{
	return mavlink_msg_waypoint_count_pack(system_id, component_id, msg, waypoint_count->target_system, waypoint_count->target_component, waypoint_count->count);
}

/**
 * @brief Send a waypoint_count message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param count Number of Waypoints in the Sequence
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_waypoint_count_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t count)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 4);
	mavlink_msg_waypoint_count_pack_chan_send(chan, msg, target_system, target_component, count);
}

#endif

// MESSAGE WAYPOINT_COUNT UNPACKING


/**
 * @brief Get field target_system from waypoint_count message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_waypoint_count_get_target_system(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field target_component from waypoint_count message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_waypoint_count_get_target_component(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field count from waypoint_count message
 *
 * @return Number of Waypoints in the Sequence
 */
static inline uint16_t mavlink_msg_waypoint_count_get_count(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Decode a waypoint_count message into a struct
 *
 * @param msg The message to decode
 * @param waypoint_count C-struct to decode the message contents into
 */
static inline void mavlink_msg_waypoint_count_decode(const mavlink_message_t* msg, mavlink_waypoint_count_t* waypoint_count)
{
#if MAVLINK_NEED_BYTE_SWAP
	waypoint_count->count = mavlink_msg_waypoint_count_get_count(msg);
	waypoint_count->target_system = mavlink_msg_waypoint_count_get_target_system(msg);
	waypoint_count->target_component = mavlink_msg_waypoint_count_get_target_component(msg);
#else
	memcpy(waypoint_count, MAVLINK_PAYLOAD(msg), 4);
#endif
}
