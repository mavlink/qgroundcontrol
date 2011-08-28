// MESSAGE DATA_STREAM PACKING

#define MAVLINK_MSG_ID_DATA_STREAM 67

typedef struct __mavlink_data_stream_t
{
 uint16_t message_rate; ///< The requested interval between two messages of this type
 uint8_t stream_id; ///< The ID of the requested data stream
 uint8_t on_off; ///< 1 stream is enabled, 0 stream is stopped.
} mavlink_data_stream_t;

#define MAVLINK_MSG_ID_DATA_STREAM_LEN 4
#define MAVLINK_MSG_ID_67_LEN 4



#define MAVLINK_MESSAGE_INFO_DATA_STREAM { \
	"DATA_STREAM", \
	3, \
	{  { "message_rate", MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_data_stream_t, message_rate) }, \
         { "stream_id", MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_data_stream_t, stream_id) }, \
         { "on_off", MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_data_stream_t, on_off) }, \
         } \
}


/**
 * @brief Pack a data_stream message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param stream_id The ID of the requested data stream
 * @param message_rate The requested interval between two messages of this type
 * @param on_off 1 stream is enabled, 0 stream is stopped.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_data_stream_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t stream_id, uint16_t message_rate, uint8_t on_off)
{
	msg->msgid = MAVLINK_MSG_ID_DATA_STREAM;

	put_uint16_t_by_index(msg, 0, message_rate); // The requested interval between two messages of this type
	put_uint8_t_by_index(msg, 2, stream_id); // The ID of the requested data stream
	put_uint8_t_by_index(msg, 3, on_off); // 1 stream is enabled, 0 stream is stopped.

	return mavlink_finalize_message(msg, system_id, component_id, 4, 21);
}

/**
 * @brief Pack a data_stream message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param stream_id The ID of the requested data stream
 * @param message_rate The requested interval between two messages of this type
 * @param on_off 1 stream is enabled, 0 stream is stopped.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_data_stream_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t stream_id,uint16_t message_rate,uint8_t on_off)
{
	msg->msgid = MAVLINK_MSG_ID_DATA_STREAM;

	put_uint16_t_by_index(msg, 0, message_rate); // The requested interval between two messages of this type
	put_uint8_t_by_index(msg, 2, stream_id); // The ID of the requested data stream
	put_uint8_t_by_index(msg, 3, on_off); // 1 stream is enabled, 0 stream is stopped.

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 4, 21);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a data_stream message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param stream_id The ID of the requested data stream
 * @param message_rate The requested interval between two messages of this type
 * @param on_off 1 stream is enabled, 0 stream is stopped.
 */
static inline void mavlink_msg_data_stream_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t stream_id,uint16_t message_rate,uint8_t on_off)
{
	msg->msgid = MAVLINK_MSG_ID_DATA_STREAM;

	put_uint16_t_by_index(msg, 0, message_rate); // The requested interval between two messages of this type
	put_uint8_t_by_index(msg, 2, stream_id); // The ID of the requested data stream
	put_uint8_t_by_index(msg, 3, on_off); // 1 stream is enabled, 0 stream is stopped.

	mavlink_finalize_message_chan_send(msg, chan, 4, 21);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a data_stream struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param data_stream C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_data_stream_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_data_stream_t* data_stream)
{
	return mavlink_msg_data_stream_pack(system_id, component_id, msg, data_stream->stream_id, data_stream->message_rate, data_stream->on_off);
}

/**
 * @brief Send a data_stream message
 * @param chan MAVLink channel to send the message
 *
 * @param stream_id The ID of the requested data stream
 * @param message_rate The requested interval between two messages of this type
 * @param on_off 1 stream is enabled, 0 stream is stopped.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_data_stream_send(mavlink_channel_t chan, uint8_t stream_id, uint16_t message_rate, uint8_t on_off)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 4);
	mavlink_msg_data_stream_pack_chan_send(chan, msg, stream_id, message_rate, on_off);
}

#endif

// MESSAGE DATA_STREAM UNPACKING


/**
 * @brief Get field stream_id from data_stream message
 *
 * @return The ID of the requested data stream
 */
static inline uint8_t mavlink_msg_data_stream_get_stream_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field message_rate from data_stream message
 *
 * @return The requested interval between two messages of this type
 */
static inline uint16_t mavlink_msg_data_stream_get_message_rate(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field on_off from data_stream message
 *
 * @return 1 stream is enabled, 0 stream is stopped.
 */
static inline uint8_t mavlink_msg_data_stream_get_on_off(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Decode a data_stream message into a struct
 *
 * @param msg The message to decode
 * @param data_stream C-struct to decode the message contents into
 */
static inline void mavlink_msg_data_stream_decode(const mavlink_message_t* msg, mavlink_data_stream_t* data_stream)
{
#if MAVLINK_NEED_BYTE_SWAP
	data_stream->message_rate = mavlink_msg_data_stream_get_message_rate(msg);
	data_stream->stream_id = mavlink_msg_data_stream_get_stream_id(msg);
	data_stream->on_off = mavlink_msg_data_stream_get_on_off(msg);
#else
	memcpy(data_stream, MAVLINK_PAYLOAD(msg), 4);
#endif
}
