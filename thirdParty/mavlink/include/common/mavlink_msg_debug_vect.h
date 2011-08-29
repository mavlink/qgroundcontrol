// MESSAGE DEBUG_VECT PACKING

#define MAVLINK_MSG_ID_DEBUG_VECT 251

typedef struct __mavlink_debug_vect_t
{
 uint64_t usec; ///< Timestamp
 float x; ///< x
 float y; ///< y
 float z; ///< z
 char name[10]; ///< Name
} mavlink_debug_vect_t;

#define MAVLINK_MSG_ID_DEBUG_VECT_LEN 30
#define MAVLINK_MSG_ID_251_LEN 30

#define MAVLINK_MSG_DEBUG_VECT_FIELD_NAME_LEN 10

#define MAVLINK_MESSAGE_INFO_DEBUG_VECT { \
	"DEBUG_VECT", \
	5, \
	{  { "usec", MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_debug_vect_t, usec) }, \
         { "x", MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_debug_vect_t, x) }, \
         { "y", MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_debug_vect_t, y) }, \
         { "z", MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_debug_vect_t, z) }, \
         { "name", MAVLINK_TYPE_CHAR, 10, 20, offsetof(mavlink_debug_vect_t, name) }, \
         } \
}


/**
 * @brief Pack a debug_vect message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param name Name
 * @param usec Timestamp
 * @param x x
 * @param y y
 * @param z z
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_vect_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       const char *name, uint64_t usec, float x, float y, float z)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG_VECT;

	put_uint64_t_by_index(msg, 0, usec); // Timestamp
	put_float_by_index(msg, 8, x); // x
	put_float_by_index(msg, 12, y); // y
	put_float_by_index(msg, 16, z); // z
	put_char_array_by_index(msg, 20, name, 10); // Name

	return mavlink_finalize_message(msg, system_id, component_id, 30, 15);
}

/**
 * @brief Pack a debug_vect message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param name Name
 * @param usec Timestamp
 * @param x x
 * @param y y
 * @param z z
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_vect_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           const char *name,uint64_t usec,float x,float y,float z)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG_VECT;

	put_uint64_t_by_index(msg, 0, usec); // Timestamp
	put_float_by_index(msg, 8, x); // x
	put_float_by_index(msg, 12, y); // y
	put_float_by_index(msg, 16, z); // z
	put_char_array_by_index(msg, 20, name, 10); // Name

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 30, 15);
}

/**
 * @brief Encode a debug_vect struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param debug_vect C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_debug_vect_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_debug_vect_t* debug_vect)
{
	return mavlink_msg_debug_vect_pack(system_id, component_id, msg, debug_vect->name, debug_vect->usec, debug_vect->x, debug_vect->y, debug_vect->z);
}

/**
 * @brief Send a debug_vect message
 * @param chan MAVLink channel to send the message
 *
 * @param name Name
 * @param usec Timestamp
 * @param x x
 * @param y y
 * @param z z
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_debug_vect_send(mavlink_channel_t chan, const char *name, uint64_t usec, float x, float y, float z)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 30);
	msg->msgid = MAVLINK_MSG_ID_DEBUG_VECT;

	put_uint64_t_by_index(msg, 0, usec); // Timestamp
	put_float_by_index(msg, 8, x); // x
	put_float_by_index(msg, 12, y); // y
	put_float_by_index(msg, 16, z); // z
	put_char_array_by_index(msg, 20, name, 10); // Name

	mavlink_finalize_message_chan_send(msg, chan, 30, 15);
}

#endif

// MESSAGE DEBUG_VECT UNPACKING


/**
 * @brief Get field name from debug_vect message
 *
 * @return Name
 */
static inline uint16_t mavlink_msg_debug_vect_get_name(const mavlink_message_t* msg, char *name)
{
	return MAVLINK_MSG_RETURN_char_array(msg, name, 10,  20);
}

/**
 * @brief Get field usec from debug_vect message
 *
 * @return Timestamp
 */
static inline uint64_t mavlink_msg_debug_vect_get_usec(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field x from debug_vect message
 *
 * @return x
 */
static inline float mavlink_msg_debug_vect_get_x(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  8);
}

/**
 * @brief Get field y from debug_vect message
 *
 * @return y
 */
static inline float mavlink_msg_debug_vect_get_y(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  12);
}

/**
 * @brief Get field z from debug_vect message
 *
 * @return z
 */
static inline float mavlink_msg_debug_vect_get_z(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  16);
}

/**
 * @brief Decode a debug_vect message into a struct
 *
 * @param msg The message to decode
 * @param debug_vect C-struct to decode the message contents into
 */
static inline void mavlink_msg_debug_vect_decode(const mavlink_message_t* msg, mavlink_debug_vect_t* debug_vect)
{
#if MAVLINK_NEED_BYTE_SWAP
	debug_vect->usec = mavlink_msg_debug_vect_get_usec(msg);
	debug_vect->x = mavlink_msg_debug_vect_get_x(msg);
	debug_vect->y = mavlink_msg_debug_vect_get_y(msg);
	debug_vect->z = mavlink_msg_debug_vect_get_z(msg);
	mavlink_msg_debug_vect_get_name(msg, debug_vect->name);
#else
	memcpy(debug_vect, MAVLINK_PAYLOAD(msg), 30);
#endif
}
