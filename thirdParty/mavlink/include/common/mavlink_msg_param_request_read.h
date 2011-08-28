// MESSAGE PARAM_REQUEST_READ PACKING

#define MAVLINK_MSG_ID_PARAM_REQUEST_READ 20

typedef struct __mavlink_param_request_read_t
{
 int16_t param_index; ///< Parameter index. Send -1 to use the param ID field as identifier
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
 char param_id[16]; ///< Onboard parameter id
} mavlink_param_request_read_t;

#define MAVLINK_MSG_ID_PARAM_REQUEST_READ_LEN 20
#define MAVLINK_MSG_ID_20_LEN 20

#define MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN 16

#define MAVLINK_MESSAGE_INFO_PARAM_REQUEST_READ { \
	"PARAM_REQUEST_READ", \
	4, \
	{  { "param_index", MAVLINK_TYPE_INT16_T, 0, 0, offsetof(mavlink_param_request_read_t, param_index) }, \
         { "target_system", MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_param_request_read_t, target_system) }, \
         { "target_component", MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_param_request_read_t, target_component) }, \
         { "param_id", MAVLINK_TYPE_CHAR, 16, 4, offsetof(mavlink_param_request_read_t, param_id) }, \
         } \
}


/**
 * @brief Pack a param_request_read message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_request_read_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, const char *param_id, int16_t param_index)
{
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;

	put_int16_t_by_index(msg, 0, param_index); // Parameter index. Send -1 to use the param ID field as identifier
	put_uint8_t_by_index(msg, 2, target_system); // System ID
	put_uint8_t_by_index(msg, 3, target_component); // Component ID
	put_char_array_by_index(msg, 4, param_id, 16); // Onboard parameter id

	return mavlink_finalize_message(msg, system_id, component_id, 20, 214);
}

/**
 * @brief Pack a param_request_read message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_request_read_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,const char *param_id,int16_t param_index)
{
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;

	put_int16_t_by_index(msg, 0, param_index); // Parameter index. Send -1 to use the param ID field as identifier
	put_uint8_t_by_index(msg, 2, target_system); // System ID
	put_uint8_t_by_index(msg, 3, target_component); // Component ID
	put_char_array_by_index(msg, 4, param_id, 16); // Onboard parameter id

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 20, 214);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a param_request_read message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 */
static inline void mavlink_msg_param_request_read_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,const char *param_id,int16_t param_index)
{
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;

	put_int16_t_by_index(msg, 0, param_index); // Parameter index. Send -1 to use the param ID field as identifier
	put_uint8_t_by_index(msg, 2, target_system); // System ID
	put_uint8_t_by_index(msg, 3, target_component); // Component ID
	put_char_array_by_index(msg, 4, param_id, 16); // Onboard parameter id

	mavlink_finalize_message_chan_send(msg, chan, 20, 214);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a param_request_read struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_request_read C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_request_read_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_request_read_t* param_request_read)
{
	return mavlink_msg_param_request_read_pack(system_id, component_id, msg, param_request_read->target_system, param_request_read->target_component, param_request_read->param_id, param_request_read->param_index);
}

/**
 * @brief Send a param_request_read message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_param_request_read_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, const char *param_id, int16_t param_index)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 20);
	mavlink_msg_param_request_read_pack_chan_send(chan, msg, target_system, target_component, param_id, param_index);
}

#endif

// MESSAGE PARAM_REQUEST_READ UNPACKING


/**
 * @brief Get field target_system from param_request_read message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_param_request_read_get_target_system(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field target_component from param_request_read message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_param_request_read_get_target_component(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field param_id from param_request_read message
 *
 * @return Onboard parameter id
 */
static inline uint16_t mavlink_msg_param_request_read_get_param_id(const mavlink_message_t* msg, char *param_id)
{
	return MAVLINK_MSG_RETURN_char_array(msg, param_id, 16,  4);
}

/**
 * @brief Get field param_index from param_request_read message
 *
 * @return Parameter index. Send -1 to use the param ID field as identifier
 */
static inline int16_t mavlink_msg_param_request_read_get_param_index(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  0);
}

/**
 * @brief Decode a param_request_read message into a struct
 *
 * @param msg The message to decode
 * @param param_request_read C-struct to decode the message contents into
 */
static inline void mavlink_msg_param_request_read_decode(const mavlink_message_t* msg, mavlink_param_request_read_t* param_request_read)
{
#if MAVLINK_NEED_BYTE_SWAP
	param_request_read->param_index = mavlink_msg_param_request_read_get_param_index(msg);
	param_request_read->target_system = mavlink_msg_param_request_read_get_target_system(msg);
	param_request_read->target_component = mavlink_msg_param_request_read_get_target_component(msg);
	mavlink_msg_param_request_read_get_param_id(msg, param_request_read->param_id);
#else
	memcpy(param_request_read, MAVLINK_PAYLOAD(msg), 20);
#endif
}
