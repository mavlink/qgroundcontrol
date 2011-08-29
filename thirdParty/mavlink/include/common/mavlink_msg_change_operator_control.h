// MESSAGE CHANGE_OPERATOR_CONTROL PACKING

#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL 5

typedef struct __mavlink_change_operator_control_t
{
 uint8_t target_system; ///< System the GCS requests control for
 uint8_t control_request; ///< 0: request control of this MAV, 1: Release control of this MAV
 uint8_t version; ///< 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 char passkey[25]; ///< Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
} mavlink_change_operator_control_t;

#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_LEN 28
#define MAVLINK_MSG_ID_5_LEN 28

#define MAVLINK_MSG_CHANGE_OPERATOR_CONTROL_FIELD_PASSKEY_LEN 25

#define MAVLINK_MESSAGE_INFO_CHANGE_OPERATOR_CONTROL { \
	"CHANGE_OPERATOR_CONTROL", \
	4, \
	{  { "target_system", MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_change_operator_control_t, target_system) }, \
         { "control_request", MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_change_operator_control_t, control_request) }, \
         { "version", MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_change_operator_control_t, version) }, \
         { "passkey", MAVLINK_TYPE_CHAR, 25, 3, offsetof(mavlink_change_operator_control_t, passkey) }, \
         } \
}


/**
 * @brief Pack a change_operator_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System the GCS requests control for
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param version 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 * @param passkey Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t control_request, uint8_t version, const char *passkey)
{
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL;

	put_uint8_t_by_index(msg, 0, target_system); // System the GCS requests control for
	put_uint8_t_by_index(msg, 1, control_request); // 0: request control of this MAV, 1: Release control of this MAV
	put_uint8_t_by_index(msg, 2, version); // 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	put_char_array_by_index(msg, 3, passkey, 25); // Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

	return mavlink_finalize_message(msg, system_id, component_id, 28, 217);
}

/**
 * @brief Pack a change_operator_control message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System the GCS requests control for
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param version 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 * @param passkey Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t control_request,uint8_t version,const char *passkey)
{
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL;

	put_uint8_t_by_index(msg, 0, target_system); // System the GCS requests control for
	put_uint8_t_by_index(msg, 1, control_request); // 0: request control of this MAV, 1: Release control of this MAV
	put_uint8_t_by_index(msg, 2, version); // 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	put_char_array_by_index(msg, 3, passkey, 25); // Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 28, 217);
}

/**
 * @brief Encode a change_operator_control struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param change_operator_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_change_operator_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_change_operator_control_t* change_operator_control)
{
	return mavlink_msg_change_operator_control_pack(system_id, component_id, msg, change_operator_control->target_system, change_operator_control->control_request, change_operator_control->version, change_operator_control->passkey);
}

/**
 * @brief Send a change_operator_control message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System the GCS requests control for
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param version 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 * @param passkey Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_change_operator_control_send(mavlink_channel_t chan, uint8_t target_system, uint8_t control_request, uint8_t version, const char *passkey)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 28);
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL;

	put_uint8_t_by_index(msg, 0, target_system); // System the GCS requests control for
	put_uint8_t_by_index(msg, 1, control_request); // 0: request control of this MAV, 1: Release control of this MAV
	put_uint8_t_by_index(msg, 2, version); // 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	put_char_array_by_index(msg, 3, passkey, 25); // Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

	mavlink_finalize_message_chan_send(msg, chan, 28, 217);
}

#endif

// MESSAGE CHANGE_OPERATOR_CONTROL UNPACKING


/**
 * @brief Get field target_system from change_operator_control message
 *
 * @return System the GCS requests control for
 */
static inline uint8_t mavlink_msg_change_operator_control_get_target_system(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field control_request from change_operator_control message
 *
 * @return 0: request control of this MAV, 1: Release control of this MAV
 */
static inline uint8_t mavlink_msg_change_operator_control_get_control_request(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field version from change_operator_control message
 *
 * @return 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 */
static inline uint8_t mavlink_msg_change_operator_control_get_version(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field passkey from change_operator_control message
 *
 * @return Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 */
static inline uint16_t mavlink_msg_change_operator_control_get_passkey(const mavlink_message_t* msg, char *passkey)
{
	return MAVLINK_MSG_RETURN_char_array(msg, passkey, 25,  3);
}

/**
 * @brief Decode a change_operator_control message into a struct
 *
 * @param msg The message to decode
 * @param change_operator_control C-struct to decode the message contents into
 */
static inline void mavlink_msg_change_operator_control_decode(const mavlink_message_t* msg, mavlink_change_operator_control_t* change_operator_control)
{
#if MAVLINK_NEED_BYTE_SWAP
	change_operator_control->target_system = mavlink_msg_change_operator_control_get_target_system(msg);
	change_operator_control->control_request = mavlink_msg_change_operator_control_get_control_request(msg);
	change_operator_control->version = mavlink_msg_change_operator_control_get_version(msg);
	mavlink_msg_change_operator_control_get_passkey(msg, change_operator_control->passkey);
#else
	memcpy(change_operator_control, MAVLINK_PAYLOAD(msg), 28);
#endif
}
