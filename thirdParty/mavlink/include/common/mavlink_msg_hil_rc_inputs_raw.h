// MESSAGE HIL_RC_INPUTS_RAW PACKING

#define MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW 92
#define MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW_LEN 33
#define MAVLINK_MSG_92_LEN 33
#define MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW_KEY 0x3B
#define MAVLINK_MSG_92_KEY 0x3B

typedef struct __mavlink_hil_rc_inputs_raw_t 
{
	uint64_t time_us;	///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	uint16_t chan1_raw;	///< RC channel 1 value, in microseconds
	uint16_t chan2_raw;	///< RC channel 2 value, in microseconds
	uint16_t chan3_raw;	///< RC channel 3 value, in microseconds
	uint16_t chan4_raw;	///< RC channel 4 value, in microseconds
	uint16_t chan5_raw;	///< RC channel 5 value, in microseconds
	uint16_t chan6_raw;	///< RC channel 6 value, in microseconds
	uint16_t chan7_raw;	///< RC channel 7 value, in microseconds
	uint16_t chan8_raw;	///< RC channel 8 value, in microseconds
	uint16_t chan9_raw;	///< RC channel 9 value, in microseconds
	uint16_t chan10_raw;	///< RC channel 10 value, in microseconds
	uint16_t chan11_raw;	///< RC channel 11 value, in microseconds
	uint16_t chan12_raw;	///< RC channel 12 value, in microseconds
	uint8_t rssi;	///< Receive signal strength indicator, 0: 0%, 255: 100%

} mavlink_hil_rc_inputs_raw_t;

/**
 * @brief Pack a hil_rc_inputs_raw message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param chan1_raw RC channel 1 value, in microseconds
 * @param chan2_raw RC channel 2 value, in microseconds
 * @param chan3_raw RC channel 3 value, in microseconds
 * @param chan4_raw RC channel 4 value, in microseconds
 * @param chan5_raw RC channel 5 value, in microseconds
 * @param chan6_raw RC channel 6 value, in microseconds
 * @param chan7_raw RC channel 7 value, in microseconds
 * @param chan8_raw RC channel 8 value, in microseconds
 * @param chan9_raw RC channel 9 value, in microseconds
 * @param chan10_raw RC channel 10 value, in microseconds
 * @param chan11_raw RC channel 11 value, in microseconds
 * @param chan12_raw RC channel 12 value, in microseconds
 * @param rssi Receive signal strength indicator, 0: 0%, 255: 100%
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t time_us, uint16_t chan1_raw, uint16_t chan2_raw, uint16_t chan3_raw, uint16_t chan4_raw, uint16_t chan5_raw, uint16_t chan6_raw, uint16_t chan7_raw, uint16_t chan8_raw, uint16_t chan9_raw, uint16_t chan10_raw, uint16_t chan11_raw, uint16_t chan12_raw, uint8_t rssi)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW;

	p->time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->chan1_raw = chan1_raw;	// uint16_t:RC channel 1 value, in microseconds
	p->chan2_raw = chan2_raw;	// uint16_t:RC channel 2 value, in microseconds
	p->chan3_raw = chan3_raw;	// uint16_t:RC channel 3 value, in microseconds
	p->chan4_raw = chan4_raw;	// uint16_t:RC channel 4 value, in microseconds
	p->chan5_raw = chan5_raw;	// uint16_t:RC channel 5 value, in microseconds
	p->chan6_raw = chan6_raw;	// uint16_t:RC channel 6 value, in microseconds
	p->chan7_raw = chan7_raw;	// uint16_t:RC channel 7 value, in microseconds
	p->chan8_raw = chan8_raw;	// uint16_t:RC channel 8 value, in microseconds
	p->chan9_raw = chan9_raw;	// uint16_t:RC channel 9 value, in microseconds
	p->chan10_raw = chan10_raw;	// uint16_t:RC channel 10 value, in microseconds
	p->chan11_raw = chan11_raw;	// uint16_t:RC channel 11 value, in microseconds
	p->chan12_raw = chan12_raw;	// uint16_t:RC channel 12 value, in microseconds
	p->rssi = rssi;	// uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW_LEN);
}

/**
 * @brief Pack a hil_rc_inputs_raw message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param chan1_raw RC channel 1 value, in microseconds
 * @param chan2_raw RC channel 2 value, in microseconds
 * @param chan3_raw RC channel 3 value, in microseconds
 * @param chan4_raw RC channel 4 value, in microseconds
 * @param chan5_raw RC channel 5 value, in microseconds
 * @param chan6_raw RC channel 6 value, in microseconds
 * @param chan7_raw RC channel 7 value, in microseconds
 * @param chan8_raw RC channel 8 value, in microseconds
 * @param chan9_raw RC channel 9 value, in microseconds
 * @param chan10_raw RC channel 10 value, in microseconds
 * @param chan11_raw RC channel 11 value, in microseconds
 * @param chan12_raw RC channel 12 value, in microseconds
 * @param rssi Receive signal strength indicator, 0: 0%, 255: 100%
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t time_us, uint16_t chan1_raw, uint16_t chan2_raw, uint16_t chan3_raw, uint16_t chan4_raw, uint16_t chan5_raw, uint16_t chan6_raw, uint16_t chan7_raw, uint16_t chan8_raw, uint16_t chan9_raw, uint16_t chan10_raw, uint16_t chan11_raw, uint16_t chan12_raw, uint8_t rssi)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW;

	p->time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->chan1_raw = chan1_raw;	// uint16_t:RC channel 1 value, in microseconds
	p->chan2_raw = chan2_raw;	// uint16_t:RC channel 2 value, in microseconds
	p->chan3_raw = chan3_raw;	// uint16_t:RC channel 3 value, in microseconds
	p->chan4_raw = chan4_raw;	// uint16_t:RC channel 4 value, in microseconds
	p->chan5_raw = chan5_raw;	// uint16_t:RC channel 5 value, in microseconds
	p->chan6_raw = chan6_raw;	// uint16_t:RC channel 6 value, in microseconds
	p->chan7_raw = chan7_raw;	// uint16_t:RC channel 7 value, in microseconds
	p->chan8_raw = chan8_raw;	// uint16_t:RC channel 8 value, in microseconds
	p->chan9_raw = chan9_raw;	// uint16_t:RC channel 9 value, in microseconds
	p->chan10_raw = chan10_raw;	// uint16_t:RC channel 10 value, in microseconds
	p->chan11_raw = chan11_raw;	// uint16_t:RC channel 11 value, in microseconds
	p->chan12_raw = chan12_raw;	// uint16_t:RC channel 12 value, in microseconds
	p->rssi = rssi;	// uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW_LEN);
}

/**
 * @brief Encode a hil_rc_inputs_raw struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param hil_rc_inputs_raw C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_hil_rc_inputs_raw_t* hil_rc_inputs_raw)
{
	return mavlink_msg_hil_rc_inputs_raw_pack(system_id, component_id, msg, hil_rc_inputs_raw->time_us, hil_rc_inputs_raw->chan1_raw, hil_rc_inputs_raw->chan2_raw, hil_rc_inputs_raw->chan3_raw, hil_rc_inputs_raw->chan4_raw, hil_rc_inputs_raw->chan5_raw, hil_rc_inputs_raw->chan6_raw, hil_rc_inputs_raw->chan7_raw, hil_rc_inputs_raw->chan8_raw, hil_rc_inputs_raw->chan9_raw, hil_rc_inputs_raw->chan10_raw, hil_rc_inputs_raw->chan11_raw, hil_rc_inputs_raw->chan12_raw, hil_rc_inputs_raw->rssi);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a hil_rc_inputs_raw message
 * @param chan MAVLink channel to send the message
 *
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param chan1_raw RC channel 1 value, in microseconds
 * @param chan2_raw RC channel 2 value, in microseconds
 * @param chan3_raw RC channel 3 value, in microseconds
 * @param chan4_raw RC channel 4 value, in microseconds
 * @param chan5_raw RC channel 5 value, in microseconds
 * @param chan6_raw RC channel 6 value, in microseconds
 * @param chan7_raw RC channel 7 value, in microseconds
 * @param chan8_raw RC channel 8 value, in microseconds
 * @param chan9_raw RC channel 9 value, in microseconds
 * @param chan10_raw RC channel 10 value, in microseconds
 * @param chan11_raw RC channel 11 value, in microseconds
 * @param chan12_raw RC channel 12 value, in microseconds
 * @param rssi Receive signal strength indicator, 0: 0%, 255: 100%
 */
static inline void mavlink_msg_hil_rc_inputs_raw_send(mavlink_channel_t chan, uint64_t time_us, uint16_t chan1_raw, uint16_t chan2_raw, uint16_t chan3_raw, uint16_t chan4_raw, uint16_t chan5_raw, uint16_t chan6_raw, uint16_t chan7_raw, uint16_t chan8_raw, uint16_t chan9_raw, uint16_t chan10_raw, uint16_t chan11_raw, uint16_t chan12_raw, uint8_t rssi)
{
	mavlink_header_t hdr;
	mavlink_hil_rc_inputs_raw_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW_LEN )
	payload.time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	payload.chan1_raw = chan1_raw;	// uint16_t:RC channel 1 value, in microseconds
	payload.chan2_raw = chan2_raw;	// uint16_t:RC channel 2 value, in microseconds
	payload.chan3_raw = chan3_raw;	// uint16_t:RC channel 3 value, in microseconds
	payload.chan4_raw = chan4_raw;	// uint16_t:RC channel 4 value, in microseconds
	payload.chan5_raw = chan5_raw;	// uint16_t:RC channel 5 value, in microseconds
	payload.chan6_raw = chan6_raw;	// uint16_t:RC channel 6 value, in microseconds
	payload.chan7_raw = chan7_raw;	// uint16_t:RC channel 7 value, in microseconds
	payload.chan8_raw = chan8_raw;	// uint16_t:RC channel 8 value, in microseconds
	payload.chan9_raw = chan9_raw;	// uint16_t:RC channel 9 value, in microseconds
	payload.chan10_raw = chan10_raw;	// uint16_t:RC channel 10 value, in microseconds
	payload.chan11_raw = chan11_raw;	// uint16_t:RC channel 11 value, in microseconds
	payload.chan12_raw = chan12_raw;	// uint16_t:RC channel 12 value, in microseconds
	payload.rssi = rssi;	// uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW_LEN;
	hdr.msgid = MAVLINK_MSG_ID_HIL_RC_INPUTS_RAW;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x3B, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE HIL_RC_INPUTS_RAW UNPACKING

/**
 * @brief Get field time_us from hil_rc_inputs_raw message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_hil_rc_inputs_raw_get_time_us(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint64_t)(p->time_us);
}

/**
 * @brief Get field chan1_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 1 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan1_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan1_raw);
}

/**
 * @brief Get field chan2_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 2 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan2_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan2_raw);
}

/**
 * @brief Get field chan3_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 3 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan3_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan3_raw);
}

/**
 * @brief Get field chan4_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 4 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan4_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan4_raw);
}

/**
 * @brief Get field chan5_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 5 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan5_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan5_raw);
}

/**
 * @brief Get field chan6_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 6 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan6_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan6_raw);
}

/**
 * @brief Get field chan7_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 7 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan7_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan7_raw);
}

/**
 * @brief Get field chan8_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 8 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan8_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan8_raw);
}

/**
 * @brief Get field chan9_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 9 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan9_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan9_raw);
}

/**
 * @brief Get field chan10_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 10 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan10_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan10_raw);
}

/**
 * @brief Get field chan11_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 11 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan11_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan11_raw);
}

/**
 * @brief Get field chan12_raw from hil_rc_inputs_raw message
 *
 * @return RC channel 12 value, in microseconds
 */
static inline uint16_t mavlink_msg_hil_rc_inputs_raw_get_chan12_raw(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint16_t)(p->chan12_raw);
}

/**
 * @brief Get field rssi from hil_rc_inputs_raw message
 *
 * @return Receive signal strength indicator, 0: 0%, 255: 100%
 */
static inline uint8_t mavlink_msg_hil_rc_inputs_raw_get_rssi(const mavlink_message_t* msg)
{
	mavlink_hil_rc_inputs_raw_t *p = (mavlink_hil_rc_inputs_raw_t *)&msg->payload[0];
	return (uint8_t)(p->rssi);
}

/**
 * @brief Decode a hil_rc_inputs_raw message into a struct
 *
 * @param msg The message to decode
 * @param hil_rc_inputs_raw C-struct to decode the message contents into
 */
static inline void mavlink_msg_hil_rc_inputs_raw_decode(const mavlink_message_t* msg, mavlink_hil_rc_inputs_raw_t* hil_rc_inputs_raw)
{
	memcpy( hil_rc_inputs_raw, msg->payload, sizeof(mavlink_hil_rc_inputs_raw_t));
}
