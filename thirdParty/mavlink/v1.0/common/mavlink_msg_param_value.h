// MESSAGE PARAM_VALUE PACKING

#define MAVLINK_MSG_ID_PARAM_VALUE 22

typedef struct __mavlink_param_value_t
{
 float param_value; ///< Onboard parameter value
 uint16_t param_count; ///< Total number of onboard parameters
 uint16_t param_index; ///< Index of this onboard parameter
 char param_id[16]; ///< Onboard parameter id
 uint8_t param_type; ///< Onboard parameter type: see MAV_VAR enum
} mavlink_param_value_t;

#define MAVLINK_MSG_ID_PARAM_VALUE_LEN 25
#define MAVLINK_MSG_ID_22_LEN 25

#define MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN 16

#define MAVLINK_MESSAGE_INFO_PARAM_VALUE { \
	"PARAM_VALUE", \
	5, \
	{  { "param_value", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_param_value_t, param_value) }, \
         { "param_count", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_param_value_t, param_count) }, \
         { "param_index", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_param_value_t, param_index) }, \
         { "param_id", NULL, MAVLINK_TYPE_CHAR, 16, 8, offsetof(mavlink_param_value_t, param_id) }, \
         { "param_type", NULL, MAVLINK_TYPE_UINT8_T, 0, 24, offsetof(mavlink_param_value_t, param_type) }, \
         } \
}


/**
 * @brief Pack a param_value message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: see MAV_VAR enum
 * @param param_count Total number of onboard parameters
 * @param param_index Index of this onboard parameter
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_value_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       const char *param_id, float param_value, uint8_t param_type, uint16_t param_count, uint16_t param_index)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[25];
	_mav_put_float(buf, 0, param_value);
	_mav_put_uint16_t(buf, 4, param_count);
	_mav_put_uint16_t(buf, 6, param_index);
	_mav_put_uint8_t(buf, 24, param_type);
	_mav_put_char_array(buf, 8, param_id, 16);
        memcpy(_MAV_PAYLOAD(msg), buf, 25);
#else
	mavlink_param_value_t packet;
	packet.param_value = param_value;
	packet.param_count = param_count;
	packet.param_index = param_index;
	packet.param_type = param_type;
	mav_array_memcpy(packet.param_id, param_id, sizeof(char)*16);
        memcpy(_MAV_PAYLOAD(msg), &packet, 25);
#endif

	msg->msgid = MAVLINK_MSG_ID_PARAM_VALUE;
	return mavlink_finalize_message(msg, system_id, component_id, 25, 220);
}

/**
 * @brief Pack a param_value message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: see MAV_VAR enum
 * @param param_count Total number of onboard parameters
 * @param param_index Index of this onboard parameter
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_value_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           const char *param_id,float param_value,uint8_t param_type,uint16_t param_count,uint16_t param_index)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[25];
	_mav_put_float(buf, 0, param_value);
	_mav_put_uint16_t(buf, 4, param_count);
	_mav_put_uint16_t(buf, 6, param_index);
	_mav_put_uint8_t(buf, 24, param_type);
	_mav_put_char_array(buf, 8, param_id, 16);
        memcpy(_MAV_PAYLOAD(msg), buf, 25);
#else
	mavlink_param_value_t packet;
	packet.param_value = param_value;
	packet.param_count = param_count;
	packet.param_index = param_index;
	packet.param_type = param_type;
	mav_array_memcpy(packet.param_id, param_id, sizeof(char)*16);
        memcpy(_MAV_PAYLOAD(msg), &packet, 25);
#endif

	msg->msgid = MAVLINK_MSG_ID_PARAM_VALUE;
	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 25, 220);
}

/**
 * @brief Encode a param_value struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_value C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_value_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_value_t* param_value)
{
	return mavlink_msg_param_value_pack(system_id, component_id, msg, param_value->param_id, param_value->param_value, param_value->param_type, param_value->param_count, param_value->param_index);
}

/**
 * @brief Send a param_value message
 * @param chan MAVLink channel to send the message
 *
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: see MAV_VAR enum
 * @param param_count Total number of onboard parameters
 * @param param_index Index of this onboard parameter
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_param_value_send(mavlink_channel_t chan, const char *param_id, float param_value, uint8_t param_type, uint16_t param_count, uint16_t param_index)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[25];
	_mav_put_float(buf, 0, param_value);
	_mav_put_uint16_t(buf, 4, param_count);
	_mav_put_uint16_t(buf, 6, param_index);
	_mav_put_uint8_t(buf, 24, param_type);
	_mav_put_char_array(buf, 8, param_id, 16);
	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE, buf, 25, 220);
#else
	mavlink_param_value_t packet;
	packet.param_value = param_value;
	packet.param_count = param_count;
	packet.param_index = param_index;
	packet.param_type = param_type;
	mav_array_memcpy(packet.param_id, param_id, sizeof(char)*16);
	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE, (const char *)&packet, 25, 220);
#endif
}

#endif

// MESSAGE PARAM_VALUE UNPACKING


/**
 * @brief Get field param_id from param_value message
 *
 * @return Onboard parameter id
 */
static inline uint16_t mavlink_msg_param_value_get_param_id(const mavlink_message_t* msg, char *param_id)
{
	return _MAV_RETURN_char_array(msg, param_id, 16,  8);
}

/**
 * @brief Get field param_value from param_value message
 *
 * @return Onboard parameter value
 */
static inline float mavlink_msg_param_value_get_param_value(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field param_type from param_value message
 *
 * @return Onboard parameter type: see MAV_VAR enum
 */
static inline uint8_t mavlink_msg_param_value_get_param_type(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  24);
}

/**
 * @brief Get field param_count from param_value message
 *
 * @return Total number of onboard parameters
 */
static inline uint16_t mavlink_msg_param_value_get_param_count(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field param_index from param_value message
 *
 * @return Index of this onboard parameter
 */
static inline uint16_t mavlink_msg_param_value_get_param_index(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Decode a param_value message into a struct
 *
 * @param msg The message to decode
 * @param param_value C-struct to decode the message contents into
 */
static inline void mavlink_msg_param_value_decode(const mavlink_message_t* msg, mavlink_param_value_t* param_value)
{
#if MAVLINK_NEED_BYTE_SWAP
	param_value->param_value = mavlink_msg_param_value_get_param_value(msg);
	param_value->param_count = mavlink_msg_param_value_get_param_count(msg);
	param_value->param_index = mavlink_msg_param_value_get_param_index(msg);
	mavlink_msg_param_value_get_param_id(msg, param_value->param_id);
	param_value->param_type = mavlink_msg_param_value_get_param_type(msg);
#else
	memcpy(param_value, _MAV_PAYLOAD(msg), 25);
#endif
}
