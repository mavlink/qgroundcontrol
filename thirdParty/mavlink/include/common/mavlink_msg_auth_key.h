// MESSAGE AUTH_KEY PACKING

#define MAVLINK_MSG_ID_AUTH_KEY 7

typedef struct __mavlink_auth_key_t
{
 char key[32]; ///< key
} mavlink_auth_key_t;

#define MAVLINK_MSG_ID_AUTH_KEY_LEN 32
#define MAVLINK_MSG_ID_7_LEN 32

#define MAVLINK_MSG_AUTH_KEY_FIELD_KEY_LEN 32

#define MAVLINK_MESSAGE_INFO_AUTH_KEY { \
	"AUTH_KEY", \
	1, \
	{  { "key", MAVLINK_TYPE_CHAR, 32, 0, offsetof(mavlink_auth_key_t, key) }, \
         } \
}


/**
 * @brief Pack a auth_key message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param key key
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_auth_key_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       const char *key)
{
	msg->msgid = MAVLINK_MSG_ID_AUTH_KEY;

	put_char_array_by_index(msg, 0, key, 32); // key

	return mavlink_finalize_message(msg, system_id, component_id, 32, 119);
}

/**
 * @brief Pack a auth_key message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param key key
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_auth_key_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           const char *key)
{
	msg->msgid = MAVLINK_MSG_ID_AUTH_KEY;

	put_char_array_by_index(msg, 0, key, 32); // key

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 32, 119);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a auth_key message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param key key
 */
static inline void mavlink_msg_auth_key_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           const char *key)
{
	msg->msgid = MAVLINK_MSG_ID_AUTH_KEY;

	put_char_array_by_index(msg, 0, key, 32); // key

	mavlink_finalize_message_chan_send(msg, chan, 32, 119);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a auth_key struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param auth_key C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_auth_key_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_auth_key_t* auth_key)
{
	return mavlink_msg_auth_key_pack(system_id, component_id, msg, auth_key->key);
}

/**
 * @brief Send a auth_key message
 * @param chan MAVLink channel to send the message
 *
 * @param key key
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_auth_key_send(mavlink_channel_t chan, const char *key)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 32);
	mavlink_msg_auth_key_pack_chan_send(chan, msg, key);
}

#endif

// MESSAGE AUTH_KEY UNPACKING


/**
 * @brief Get field key from auth_key message
 *
 * @return key
 */
static inline uint16_t mavlink_msg_auth_key_get_key(const mavlink_message_t* msg, char *key)
{
	return MAVLINK_MSG_RETURN_char_array(msg, key, 32,  0);
}

/**
 * @brief Decode a auth_key message into a struct
 *
 * @param msg The message to decode
 * @param auth_key C-struct to decode the message contents into
 */
static inline void mavlink_msg_auth_key_decode(const mavlink_message_t* msg, mavlink_auth_key_t* auth_key)
{
#if MAVLINK_NEED_BYTE_SWAP
	mavlink_msg_auth_key_get_key(msg, auth_key->key);
#else
	memcpy(auth_key, MAVLINK_PAYLOAD(msg), 32);
#endif
}
