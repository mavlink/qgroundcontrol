// MESSAGE SYSTEM_TIME_UTC PACKING

#define MAVLINK_MSG_ID_SYSTEM_TIME_UTC 4
#define MAVLINK_MSG_ID_SYSTEM_TIME_UTC_LEN 8
#define MAVLINK_MSG_4_LEN 8

typedef struct __mavlink_system_time_utc_t 
{
	uint32_t utc_date; ///< GPS UTC date ddmmyy
	uint32_t utc_time; ///< GPS UTC time hhmmss

} mavlink_system_time_utc_t;

/**
 * @brief Pack a system_time_utc message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param utc_date GPS UTC date ddmmyy
 * @param utc_time GPS UTC time hhmmss
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_system_time_utc_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint32_t utc_date, uint32_t utc_time)
{
	mavlink_system_time_utc_t *p = (mavlink_system_time_utc_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYSTEM_TIME_UTC;

	p->utc_date = utc_date; // uint32_t:GPS UTC date ddmmyy
	p->utc_time = utc_time; // uint32_t:GPS UTC time hhmmss

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYSTEM_TIME_UTC_LEN);
}

/**
 * @brief Pack a system_time_utc message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param utc_date GPS UTC date ddmmyy
 * @param utc_time GPS UTC time hhmmss
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_system_time_utc_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint32_t utc_date, uint32_t utc_time)
{
	mavlink_system_time_utc_t *p = (mavlink_system_time_utc_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYSTEM_TIME_UTC;

	p->utc_date = utc_date; // uint32_t:GPS UTC date ddmmyy
	p->utc_time = utc_time; // uint32_t:GPS UTC time hhmmss

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYSTEM_TIME_UTC_LEN);
}

/**
 * @brief Encode a system_time_utc struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param system_time_utc C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_system_time_utc_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_system_time_utc_t* system_time_utc)
{
	return mavlink_msg_system_time_utc_pack(system_id, component_id, msg, system_time_utc->utc_date, system_time_utc->utc_time);
}

/**
 * @brief Send a system_time_utc message
 * @param chan MAVLink channel to send the message
 *
 * @param utc_date GPS UTC date ddmmyy
 * @param utc_time GPS UTC time hhmmss
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_system_time_utc_send(mavlink_channel_t chan, uint32_t utc_date, uint32_t utc_time)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_system_time_utc_t *p = (mavlink_system_time_utc_t *)&msg.payload[0];

	p->utc_date = utc_date; // uint32_t:GPS UTC date ddmmyy
	p->utc_time = utc_time; // uint32_t:GPS UTC time hhmmss

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_SYSTEM_TIME_UTC_LEN;
	msg.msgid = MAVLINK_MSG_ID_SYSTEM_TIME_UTC;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_system_time_utc_send(mavlink_channel_t chan, uint32_t utc_date, uint32_t utc_time)
{
	mavlink_header_t hdr;
	mavlink_system_time_utc_t payload;
	uint16_t checksum;
	mavlink_system_time_utc_t *p = &payload;

	p->utc_date = utc_date; // uint32_t:GPS UTC date ddmmyy
	p->utc_time = utc_time; // uint32_t:GPS UTC time hhmmss

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SYSTEM_TIME_UTC_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SYSTEM_TIME_UTC;
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
// MESSAGE SYSTEM_TIME_UTC UNPACKING

/**
 * @brief Get field utc_date from system_time_utc message
 *
 * @return GPS UTC date ddmmyy
 */
static inline uint32_t mavlink_msg_system_time_utc_get_utc_date(const mavlink_message_t* msg)
{
	mavlink_system_time_utc_t *p = (mavlink_system_time_utc_t *)&msg->payload[0];
	return (uint32_t)(p->utc_date);
}

/**
 * @brief Get field utc_time from system_time_utc message
 *
 * @return GPS UTC time hhmmss
 */
static inline uint32_t mavlink_msg_system_time_utc_get_utc_time(const mavlink_message_t* msg)
{
	mavlink_system_time_utc_t *p = (mavlink_system_time_utc_t *)&msg->payload[0];
	return (uint32_t)(p->utc_time);
}

/**
 * @brief Decode a system_time_utc message into a struct
 *
 * @param msg The message to decode
 * @param system_time_utc C-struct to decode the message contents into
 */
static inline void mavlink_msg_system_time_utc_decode(const mavlink_message_t* msg, mavlink_system_time_utc_t* system_time_utc)
{
	memcpy( system_time_utc, msg->payload, sizeof(mavlink_system_time_utc_t));
}
