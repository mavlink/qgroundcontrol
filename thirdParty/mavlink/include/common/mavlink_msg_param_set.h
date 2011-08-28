// MESSAGE PARAM_SET PACKING

#define MAVLINK_MSG_ID_PARAM_SET 23

typedef struct __mavlink_param_set_t
{
 float param_value; ///< Onboard parameter value
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
 char param_id[16]; ///< Onboard parameter id
 uint8_t param_type; ///< Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
} mavlink_param_set_t;

#define MAVLINK_MSG_ID_PARAM_SET_LEN 23
#define MAVLINK_MSG_ID_23_LEN 23

#define MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN 16

#define MAVLINK_MESSAGE_INFO_PARAM_SET { \
	"PARAM_SET", \
	5, \
	{  { "param_value", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_param_set_t, param_value) }, \
         { "target_system", MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_param_set_t, target_system) }, \
         { "target_component", MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_param_set_t, target_component) }, \
         { "param_id", MAVLINK_TYPE_CHAR, 16, 6, offsetof(mavlink_param_set_t, param_id) }, \
         { "param_type", MAVLINK_TYPE_UINT8_T, 0, 22, offsetof(mavlink_param_set_t, param_type) }, \
         } \
}


/**
 * @brief Pack a param_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_set_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, const char *param_id, float param_value, uint8_t param_type)
{
	msg->msgid = MAVLINK_MSG_ID_PARAM_SET;

	put_float_by_index(msg, 0, param_value); // Onboard parameter value
	put_uint8_t_by_index(msg, 4, target_system); // System ID
	put_uint8_t_by_index(msg, 5, target_component); // Component ID
	put_char_array_by_index(msg, 6, param_id, 16); // Onboard parameter id
	put_uint8_t_by_index(msg, 22, param_type); // Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

	return mavlink_finalize_message(msg, system_id, component_id, 23, 168);
}

/**
 * @brief Pack a param_set message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_set_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,const char *param_id,float param_value,uint8_t param_type)
{
	msg->msgid = MAVLINK_MSG_ID_PARAM_SET;

	put_float_by_index(msg, 0, param_value); // Onboard parameter value
	put_uint8_t_by_index(msg, 4, target_system); // System ID
	put_uint8_t_by_index(msg, 5, target_component); // Component ID
	put_char_array_by_index(msg, 6, param_id, 16); // Onboard parameter id
	put_uint8_t_by_index(msg, 22, param_type); // Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 23, 168);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a param_set message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 */
static inline void mavlink_msg_param_set_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,const char *param_id,float param_value,uint8_t param_type)
{
	msg->msgid = MAVLINK_MSG_ID_PARAM_SET;

	put_float_by_index(msg, 0, param_value); // Onboard parameter value
	put_uint8_t_by_index(msg, 4, target_system); // System ID
	put_uint8_t_by_index(msg, 5, target_component); // Component ID
	put_char_array_by_index(msg, 6, param_id, 16); // Onboard parameter id
	put_uint8_t_by_index(msg, 22, param_type); // Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

	mavlink_finalize_message_chan_send(msg, chan, 23, 168);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a param_set struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_set_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_set_t* param_set)
{
	return mavlink_msg_param_set_pack(system_id, component_id, msg, param_set->target_system, param_set->target_component, param_set->param_id, param_set->param_value, param_set->param_type);
}

/**
 * @brief Send a param_set message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_param_set_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, const char *param_id, float param_value, uint8_t param_type)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 23);
	mavlink_msg_param_set_pack_chan_send(chan, msg, target_system, target_component, param_id, param_value, param_type);
}

#endif

// MESSAGE PARAM_SET UNPACKING


/**
 * @brief Get field target_system from param_set message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_param_set_get_target_system(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field target_component from param_set message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_param_set_get_target_component(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field param_id from param_set message
 *
 * @return Onboard parameter id
 */
static inline uint16_t mavlink_msg_param_set_get_param_id(const mavlink_message_t* msg, char *param_id)
{
	return MAVLINK_MSG_RETURN_char_array(msg, param_id, 16,  6);
}

/**
 * @brief Get field param_value from param_set message
 *
 * @return Onboard parameter value
 */
static inline float mavlink_msg_param_set_get_param_value(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
}

/**
 * @brief Get field param_type from param_set message
 *
 * @return Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 */
static inline uint8_t mavlink_msg_param_set_get_param_type(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  22);
}

/**
 * @brief Decode a param_set message into a struct
 *
 * @param msg The message to decode
 * @param param_set C-struct to decode the message contents into
 */
static inline void mavlink_msg_param_set_decode(const mavlink_message_t* msg, mavlink_param_set_t* param_set)
{
#if MAVLINK_NEED_BYTE_SWAP
	param_set->param_value = mavlink_msg_param_set_get_param_value(msg);
	param_set->target_system = mavlink_msg_param_set_get_target_system(msg);
	param_set->target_component = mavlink_msg_param_set_get_target_component(msg);
	mavlink_msg_param_set_get_param_id(msg, param_set->param_id);
	param_set->param_type = mavlink_msg_param_set_get_param_type(msg);
#else
	memcpy(param_set, MAVLINK_PAYLOAD(msg), 23);
#endif
}
