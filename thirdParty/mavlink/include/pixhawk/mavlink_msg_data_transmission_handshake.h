// MESSAGE DATA_TRANSMISSION_HANDSHAKE PACKING

#define MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE 170
#define MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE_LEN 8
#define MAVLINK_MSG_170_LEN 8

typedef struct __mavlink_data_transmission_handshake_t 
{
	uint32_t size; ///< total data size in bytes (set on ACK only)
	uint8_t type; ///< type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
	uint8_t packets; ///< number of packets beeing sent (set on ACK only)
	uint8_t payload; ///< payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
	uint8_t jpg_quality; ///< JPEG quality out of [1,100]

} mavlink_data_transmission_handshake_t;

/**
 * @brief Pack a data_transmission_handshake message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param type type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
 * @param size total data size in bytes (set on ACK only)
 * @param packets number of packets beeing sent (set on ACK only)
 * @param payload payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
 * @param jpg_quality JPEG quality out of [1,100]
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_data_transmission_handshake_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t type, uint32_t size, uint8_t packets, uint8_t payload, uint8_t jpg_quality)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE;

	p->type = type; // uint8_t:type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
	p->size = size; // uint32_t:total data size in bytes (set on ACK only)
	p->packets = packets; // uint8_t:number of packets beeing sent (set on ACK only)
	p->payload = payload; // uint8_t:payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
	p->jpg_quality = jpg_quality; // uint8_t:JPEG quality out of [1,100]

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE_LEN);
}

/**
 * @brief Pack a data_transmission_handshake message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param type type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
 * @param size total data size in bytes (set on ACK only)
 * @param packets number of packets beeing sent (set on ACK only)
 * @param payload payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
 * @param jpg_quality JPEG quality out of [1,100]
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_data_transmission_handshake_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t type, uint32_t size, uint8_t packets, uint8_t payload, uint8_t jpg_quality)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE;

	p->type = type; // uint8_t:type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
	p->size = size; // uint32_t:total data size in bytes (set on ACK only)
	p->packets = packets; // uint8_t:number of packets beeing sent (set on ACK only)
	p->payload = payload; // uint8_t:payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
	p->jpg_quality = jpg_quality; // uint8_t:JPEG quality out of [1,100]

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE_LEN);
}

/**
 * @brief Encode a data_transmission_handshake struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param data_transmission_handshake C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_data_transmission_handshake_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_data_transmission_handshake_t* data_transmission_handshake)
{
	return mavlink_msg_data_transmission_handshake_pack(system_id, component_id, msg, data_transmission_handshake->type, data_transmission_handshake->size, data_transmission_handshake->packets, data_transmission_handshake->payload, data_transmission_handshake->jpg_quality);
}

/**
 * @brief Send a data_transmission_handshake message
 * @param chan MAVLink channel to send the message
 *
 * @param type type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
 * @param size total data size in bytes (set on ACK only)
 * @param packets number of packets beeing sent (set on ACK only)
 * @param payload payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
 * @param jpg_quality JPEG quality out of [1,100]
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_data_transmission_handshake_send(mavlink_channel_t chan, uint8_t type, uint32_t size, uint8_t packets, uint8_t payload, uint8_t jpg_quality)
{
	mavlink_header_t hdr;
	mavlink_data_transmission_handshake_t payload;
	uint16_t checksum;
	mavlink_data_transmission_handshake_t *p = &payload;

	p->type = type; // uint8_t:type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
	p->size = size; // uint32_t:total data size in bytes (set on ACK only)
	p->packets = packets; // uint8_t:number of packets beeing sent (set on ACK only)
	p->payload = payload; // uint8_t:payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
	p->jpg_quality = jpg_quality; // uint8_t:JPEG quality out of [1,100]

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&checksum);
	checksum = crc_calculate_mem((uint8_t *)&hdr.len, &checksum, MAVLINK_CORE_HEADER_LEN);
	checksum = crc_calculate_mem((uint8_t *)&payload, &checksum, hdr.len );
	hdr.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	hdr.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_mem(chan, (uint8_t *)&payload, hdr.len);
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck_a, MAVLINK_NUM_CHECKSUM_BYTES);
}

#endif
// MESSAGE DATA_TRANSMISSION_HANDSHAKE UNPACKING

/**
 * @brief Get field type from data_transmission_handshake message
 *
 * @return type of requested/acknowledged data (as defined in ENUM DATA_TYPES in mavlink/include/mavlink_types.h)
 */
static inline uint8_t mavlink_msg_data_transmission_handshake_get_type(const mavlink_message_t* msg)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	return (uint8_t)(p->type);
}

/**
 * @brief Get field size from data_transmission_handshake message
 *
 * @return total data size in bytes (set on ACK only)
 */
static inline uint32_t mavlink_msg_data_transmission_handshake_get_size(const mavlink_message_t* msg)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	return (uint32_t)(p->size);
}

/**
 * @brief Get field packets from data_transmission_handshake message
 *
 * @return number of packets beeing sent (set on ACK only)
 */
static inline uint8_t mavlink_msg_data_transmission_handshake_get_packets(const mavlink_message_t* msg)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	return (uint8_t)(p->packets);
}

/**
 * @brief Get field payload from data_transmission_handshake message
 *
 * @return payload size per packet (normally 253 byte, see DATA field size in message ENCAPSULATED_DATA) (set on ACK only)
 */
static inline uint8_t mavlink_msg_data_transmission_handshake_get_payload(const mavlink_message_t* msg)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	return (uint8_t)(p->payload);
}

/**
 * @brief Get field jpg_quality from data_transmission_handshake message
 *
 * @return JPEG quality out of [1,100]
 */
static inline uint8_t mavlink_msg_data_transmission_handshake_get_jpg_quality(const mavlink_message_t* msg)
{
	mavlink_data_transmission_handshake_t *p = (mavlink_data_transmission_handshake_t *)&msg->payload[0];
	return (uint8_t)(p->jpg_quality);
}

/**
 * @brief Decode a data_transmission_handshake message into a struct
 *
 * @param msg The message to decode
 * @param data_transmission_handshake C-struct to decode the message contents into
 */
static inline void mavlink_msg_data_transmission_handshake_decode(const mavlink_message_t* msg, mavlink_data_transmission_handshake_t* data_transmission_handshake)
{
	memcpy( data_transmission_handshake, msg->payload, sizeof(mavlink_data_transmission_handshake_t));
}
