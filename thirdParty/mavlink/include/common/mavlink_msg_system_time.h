// MESSAGE SYSTEM_TIME PACKING

#define MAVLINK_MSG_ID_SYSTEM_TIME 2
#define MAVLINK_MSG_ID_SYSTEM_TIME_LEN 8
#define MAVLINK_MSG_2_LEN 8
#define MAVLINK_MSG_ID_SYSTEM_TIME_KEY 0xE8
#define MAVLINK_MSG_2_KEY 0xE8

typedef struct __mavlink_system_time_t 
{
	uint64_t time_usec;	///< Timestamp of the master clock in microseconds since UNIX epoch.

} mavlink_system_time_t;

/**
 * @brief Pack a system_time message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec Timestamp of the master clock in microseconds since UNIX epoch.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_system_time_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t time_usec)
{
	mavlink_system_time_t *p = (mavlink_system_time_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYSTEM_TIME;

	p->time_usec = time_usec;	// uint64_t:Timestamp of the master clock in microseconds since UNIX epoch.

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYSTEM_TIME_LEN);
}

/**
 * @brief Pack a system_time message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_usec Timestamp of the master clock in microseconds since UNIX epoch.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_system_time_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t time_usec)
{
	mavlink_system_time_t *p = (mavlink_system_time_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYSTEM_TIME;

	p->time_usec = time_usec;	// uint64_t:Timestamp of the master clock in microseconds since UNIX epoch.

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYSTEM_TIME_LEN);
}

/**
 * @brief Encode a system_time struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param system_time C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_system_time_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_system_time_t* system_time)
{
	return mavlink_msg_system_time_pack(system_id, component_id, msg, system_time->time_usec);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a system_time message
 * @param chan MAVLink channel to send the message
 *
 * @param time_usec Timestamp of the master clock in microseconds since UNIX epoch.
 */
static inline void mavlink_msg_system_time_send(mavlink_channel_t chan, uint64_t time_usec)
{
	mavlink_header_t hdr;
	mavlink_system_time_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SYSTEM_TIME_LEN )
	payload.time_usec = time_usec;	// uint64_t:Timestamp of the master clock in microseconds since UNIX epoch.

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SYSTEM_TIME_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SYSTEM_TIME;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xE8, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SYSTEM_TIME UNPACKING

/**
 * @brief Get field time_usec from system_time message
 *
 * @return Timestamp of the master clock in microseconds since UNIX epoch.
 */
static inline uint64_t mavlink_msg_system_time_get_time_usec(const mavlink_message_t* msg)
{
	mavlink_system_time_t *p = (mavlink_system_time_t *)&msg->payload[0];
	return (uint64_t)(p->time_usec);
}

/**
 * @brief Decode a system_time message into a struct
 *
 * @param msg The message to decode
 * @param system_time C-struct to decode the message contents into
 */
static inline void mavlink_msg_system_time_decode(const mavlink_message_t* msg, mavlink_system_time_t* system_time)
{
	memcpy( system_time, msg->payload, sizeof(mavlink_system_time_t));
}
