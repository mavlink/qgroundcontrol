// MESSAGE DATA_STREAM PACKING

#define MAVLINK_MSG_ID_DATA_STREAM 67
#define MAVLINK_MSG_ID_DATA_STREAM_LEN 4
#define MAVLINK_MSG_67_LEN 4
#define MAVLINK_MSG_ID_DATA_STREAM_KEY 0x50
#define MAVLINK_MSG_67_KEY 0x50

typedef struct __mavlink_data_stream_t 
{
	uint16_t message_rate;	///< The requested interval between two messages of this type
	uint8_t stream_id;	///< The ID of the requested data stream
	uint8_t on_off;	///< 1 stream is enabled, 0 stream is stopped.

} mavlink_data_stream_t;

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
static inline uint16_t mavlink_msg_data_stream_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t stream_id, uint16_t message_rate, uint8_t on_off)
{
	mavlink_data_stream_t *p = (mavlink_data_stream_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DATA_STREAM;

	p->stream_id = stream_id;	// uint8_t:The ID of the requested data stream
	p->message_rate = message_rate;	// uint16_t:The requested interval between two messages of this type
	p->on_off = on_off;	// uint8_t:1 stream is enabled, 0 stream is stopped.

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DATA_STREAM_LEN);
}

/**
 * @brief Pack a data_stream message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param stream_id The ID of the requested data stream
 * @param message_rate The requested interval between two messages of this type
 * @param on_off 1 stream is enabled, 0 stream is stopped.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_data_stream_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t stream_id, uint16_t message_rate, uint8_t on_off)
{
	mavlink_data_stream_t *p = (mavlink_data_stream_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DATA_STREAM;

	p->stream_id = stream_id;	// uint8_t:The ID of the requested data stream
	p->message_rate = message_rate;	// uint16_t:The requested interval between two messages of this type
	p->on_off = on_off;	// uint8_t:1 stream is enabled, 0 stream is stopped.

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DATA_STREAM_LEN);
}

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


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a data_stream message
 * @param chan MAVLink channel to send the message
 *
 * @param stream_id The ID of the requested data stream
 * @param message_rate The requested interval between two messages of this type
 * @param on_off 1 stream is enabled, 0 stream is stopped.
 */
static inline void mavlink_msg_data_stream_send(mavlink_channel_t chan, uint8_t stream_id, uint16_t message_rate, uint8_t on_off)
{
	mavlink_header_t hdr;
	mavlink_data_stream_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_DATA_STREAM_LEN )
	payload.stream_id = stream_id;	// uint8_t:The ID of the requested data stream
	payload.message_rate = message_rate;	// uint16_t:The requested interval between two messages of this type
	payload.on_off = on_off;	// uint8_t:1 stream is enabled, 0 stream is stopped.

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_DATA_STREAM_LEN;
	hdr.msgid = MAVLINK_MSG_ID_DATA_STREAM;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x50, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
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
	mavlink_data_stream_t *p = (mavlink_data_stream_t *)&msg->payload[0];
	return (uint8_t)(p->stream_id);
}

/**
 * @brief Get field message_rate from data_stream message
 *
 * @return The requested interval between two messages of this type
 */
static inline uint16_t mavlink_msg_data_stream_get_message_rate(const mavlink_message_t* msg)
{
	mavlink_data_stream_t *p = (mavlink_data_stream_t *)&msg->payload[0];
	return (uint16_t)(p->message_rate);
}

/**
 * @brief Get field on_off from data_stream message
 *
 * @return 1 stream is enabled, 0 stream is stopped.
 */
static inline uint8_t mavlink_msg_data_stream_get_on_off(const mavlink_message_t* msg)
{
	mavlink_data_stream_t *p = (mavlink_data_stream_t *)&msg->payload[0];
	return (uint8_t)(p->on_off);
}

/**
 * @brief Decode a data_stream message into a struct
 *
 * @param msg The message to decode
 * @param data_stream C-struct to decode the message contents into
 */
static inline void mavlink_msg_data_stream_decode(const mavlink_message_t* msg, mavlink_data_stream_t* data_stream)
{
	memcpy( data_stream, msg->payload, sizeof(mavlink_data_stream_t));
}
