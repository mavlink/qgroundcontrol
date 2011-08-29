// MESSAGE ENCAPSULATED_DATA PACKING

#define MAVLINK_MSG_ID_ENCAPSULATED_DATA 194

typedef struct __mavlink_encapsulated_data_t
{
 uint16_t seqnr; ///< sequence number (starting with 0 on every transmission)
 uint8_t data[253]; ///< image data bytes
} mavlink_encapsulated_data_t;

#define MAVLINK_MSG_ID_ENCAPSULATED_DATA_LEN 255
#define MAVLINK_MSG_ID_194_LEN 255

#define MAVLINK_MSG_ENCAPSULATED_DATA_FIELD_DATA_LEN 253

#define MAVLINK_MESSAGE_INFO_ENCAPSULATED_DATA { \
	"ENCAPSULATED_DATA", \
	2, \
	{  { "seqnr", MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_encapsulated_data_t, seqnr) }, \
         { "data", MAVLINK_TYPE_UINT8_T, 253, 2, offsetof(mavlink_encapsulated_data_t, data) }, \
         } \
}


/**
 * @brief Pack a encapsulated_data message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param seqnr sequence number (starting with 0 on every transmission)
 * @param data image data bytes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_encapsulated_data_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint16_t seqnr, const uint8_t *data)
{
	msg->msgid = MAVLINK_MSG_ID_ENCAPSULATED_DATA;

	put_uint16_t_by_index(msg, 0, seqnr); // sequence number (starting with 0 on every transmission)
	put_uint8_t_array_by_index(msg, 2, data, 253); // image data bytes

	return mavlink_finalize_message(msg, system_id, component_id, 255, 223);
}

/**
 * @brief Pack a encapsulated_data message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param seqnr sequence number (starting with 0 on every transmission)
 * @param data image data bytes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_encapsulated_data_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint16_t seqnr,const uint8_t *data)
{
	msg->msgid = MAVLINK_MSG_ID_ENCAPSULATED_DATA;

	put_uint16_t_by_index(msg, 0, seqnr); // sequence number (starting with 0 on every transmission)
	put_uint8_t_array_by_index(msg, 2, data, 253); // image data bytes

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 255, 223);
}

/**
 * @brief Encode a encapsulated_data struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param encapsulated_data C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_encapsulated_data_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_encapsulated_data_t* encapsulated_data)
{
	return mavlink_msg_encapsulated_data_pack(system_id, component_id, msg, encapsulated_data->seqnr, encapsulated_data->data);
}

/**
 * @brief Send a encapsulated_data message
 * @param chan MAVLink channel to send the message
 *
 * @param seqnr sequence number (starting with 0 on every transmission)
 * @param data image data bytes
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_encapsulated_data_send(mavlink_channel_t chan, uint16_t seqnr, const uint8_t *data)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 255);
	msg->msgid = MAVLINK_MSG_ID_ENCAPSULATED_DATA;

	put_uint16_t_by_index(msg, 0, seqnr); // sequence number (starting with 0 on every transmission)
	put_uint8_t_array_by_index(msg, 2, data, 253); // image data bytes

	mavlink_finalize_message_chan_send(msg, chan, 255, 223);
}

#endif

// MESSAGE ENCAPSULATED_DATA UNPACKING


/**
 * @brief Get field seqnr from encapsulated_data message
 *
 * @return sequence number (starting with 0 on every transmission)
 */
static inline uint16_t mavlink_msg_encapsulated_data_get_seqnr(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field data from encapsulated_data message
 *
 * @return image data bytes
 */
static inline uint16_t mavlink_msg_encapsulated_data_get_data(const mavlink_message_t* msg, uint8_t *data)
{
	return MAVLINK_MSG_RETURN_uint8_t_array(msg, data, 253,  2);
}

/**
 * @brief Decode a encapsulated_data message into a struct
 *
 * @param msg The message to decode
 * @param encapsulated_data C-struct to decode the message contents into
 */
static inline void mavlink_msg_encapsulated_data_decode(const mavlink_message_t* msg, mavlink_encapsulated_data_t* encapsulated_data)
{
#if MAVLINK_NEED_BYTE_SWAP
	encapsulated_data->seqnr = mavlink_msg_encapsulated_data_get_seqnr(msg);
	mavlink_msg_encapsulated_data_get_data(msg, encapsulated_data->data);
#else
	memcpy(encapsulated_data, MAVLINK_PAYLOAD(msg), 255);
#endif
}
