// MESSAGE RC_CHANNELS_OVERRIDE PACKING

#define MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE 70
#define MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE_LEN 18
#define MAVLINK_MSG_70_LEN 18

typedef struct __mavlink_rc_channels_override_t 
{
	uint16_t chan1_raw; ///< RC channel 1 value, in microseconds
	uint16_t chan2_raw; ///< RC channel 2 value, in microseconds
	uint16_t chan3_raw; ///< RC channel 3 value, in microseconds
	uint16_t chan4_raw; ///< RC channel 4 value, in microseconds
	uint16_t chan5_raw; ///< RC channel 5 value, in microseconds
	uint16_t chan6_raw; ///< RC channel 6 value, in microseconds
	uint16_t chan7_raw; ///< RC channel 7 value, in microseconds
	uint16_t chan8_raw; ///< RC channel 8 value, in microseconds
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID

} mavlink_rc_channels_override_t;

/**
 * @brief Pack a rc_channels_override message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param chan1_raw RC channel 1 value, in microseconds
 * @param chan2_raw RC channel 2 value, in microseconds
 * @param chan3_raw RC channel 3 value, in microseconds
 * @param chan4_raw RC channel 4 value, in microseconds
 * @param chan5_raw RC channel 5 value, in microseconds
 * @param chan6_raw RC channel 6 value, in microseconds
 * @param chan7_raw RC channel 7 value, in microseconds
 * @param chan8_raw RC channel 8 value, in microseconds
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rc_channels_override_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint16_t chan1_raw, uint16_t chan2_raw, uint16_t chan3_raw, uint16_t chan4_raw, uint16_t chan5_raw, uint16_t chan6_raw, uint16_t chan7_raw, uint16_t chan8_raw)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->chan1_raw = chan1_raw; // uint16_t:RC channel 1 value, in microseconds
	p->chan2_raw = chan2_raw; // uint16_t:RC channel 2 value, in microseconds
	p->chan3_raw = chan3_raw; // uint16_t:RC channel 3 value, in microseconds
	p->chan4_raw = chan4_raw; // uint16_t:RC channel 4 value, in microseconds
	p->chan5_raw = chan5_raw; // uint16_t:RC channel 5 value, in microseconds
	p->chan6_raw = chan6_raw; // uint16_t:RC channel 6 value, in microseconds
	p->chan7_raw = chan7_raw; // uint16_t:RC channel 7 value, in microseconds
	p->chan8_raw = chan8_raw; // uint16_t:RC channel 8 value, in microseconds

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE_LEN);
}

/**
 * @brief Pack a rc_channels_override message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param chan1_raw RC channel 1 value, in microseconds
 * @param chan2_raw RC channel 2 value, in microseconds
 * @param chan3_raw RC channel 3 value, in microseconds
 * @param chan4_raw RC channel 4 value, in microseconds
 * @param chan5_raw RC channel 5 value, in microseconds
 * @param chan6_raw RC channel 6 value, in microseconds
 * @param chan7_raw RC channel 7 value, in microseconds
 * @param chan8_raw RC channel 8 value, in microseconds
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rc_channels_override_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint16_t chan1_raw, uint16_t chan2_raw, uint16_t chan3_raw, uint16_t chan4_raw, uint16_t chan5_raw, uint16_t chan6_raw, uint16_t chan7_raw, uint16_t chan8_raw)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->chan1_raw = chan1_raw; // uint16_t:RC channel 1 value, in microseconds
	p->chan2_raw = chan2_raw; // uint16_t:RC channel 2 value, in microseconds
	p->chan3_raw = chan3_raw; // uint16_t:RC channel 3 value, in microseconds
	p->chan4_raw = chan4_raw; // uint16_t:RC channel 4 value, in microseconds
	p->chan5_raw = chan5_raw; // uint16_t:RC channel 5 value, in microseconds
	p->chan6_raw = chan6_raw; // uint16_t:RC channel 6 value, in microseconds
	p->chan7_raw = chan7_raw; // uint16_t:RC channel 7 value, in microseconds
	p->chan8_raw = chan8_raw; // uint16_t:RC channel 8 value, in microseconds

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE_LEN);
}

/**
 * @brief Encode a rc_channels_override struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rc_channels_override C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rc_channels_override_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rc_channels_override_t* rc_channels_override)
{
	return mavlink_msg_rc_channels_override_pack(system_id, component_id, msg, rc_channels_override->target_system, rc_channels_override->target_component, rc_channels_override->chan1_raw, rc_channels_override->chan2_raw, rc_channels_override->chan3_raw, rc_channels_override->chan4_raw, rc_channels_override->chan5_raw, rc_channels_override->chan6_raw, rc_channels_override->chan7_raw, rc_channels_override->chan8_raw);
}

/**
 * @brief Send a rc_channels_override message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param chan1_raw RC channel 1 value, in microseconds
 * @param chan2_raw RC channel 2 value, in microseconds
 * @param chan3_raw RC channel 3 value, in microseconds
 * @param chan4_raw RC channel 4 value, in microseconds
 * @param chan5_raw RC channel 5 value, in microseconds
 * @param chan6_raw RC channel 6 value, in microseconds
 * @param chan7_raw RC channel 7 value, in microseconds
 * @param chan8_raw RC channel 8 value, in microseconds
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_rc_channels_override_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t chan1_raw, uint16_t chan2_raw, uint16_t chan3_raw, uint16_t chan4_raw, uint16_t chan5_raw, uint16_t chan6_raw, uint16_t chan7_raw, uint16_t chan8_raw)
{
	mavlink_header_t hdr;
	mavlink_rc_channels_override_t payload;
	uint16_t checksum;
	mavlink_rc_channels_override_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->chan1_raw = chan1_raw; // uint16_t:RC channel 1 value, in microseconds
	p->chan2_raw = chan2_raw; // uint16_t:RC channel 2 value, in microseconds
	p->chan3_raw = chan3_raw; // uint16_t:RC channel 3 value, in microseconds
	p->chan4_raw = chan4_raw; // uint16_t:RC channel 4 value, in microseconds
	p->chan5_raw = chan5_raw; // uint16_t:RC channel 5 value, in microseconds
	p->chan6_raw = chan6_raw; // uint16_t:RC channel 6 value, in microseconds
	p->chan7_raw = chan7_raw; // uint16_t:RC channel 7 value, in microseconds
	p->chan8_raw = chan8_raw; // uint16_t:RC channel 8 value, in microseconds

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE;
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
// MESSAGE RC_CHANNELS_OVERRIDE UNPACKING

/**
 * @brief Get field target_system from rc_channels_override message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_rc_channels_override_get_target_system(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from rc_channels_override message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_rc_channels_override_get_target_component(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field chan1_raw from rc_channels_override message
 *
 * @return RC channel 1 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan1_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan1_raw);
}

/**
 * @brief Get field chan2_raw from rc_channels_override message
 *
 * @return RC channel 2 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan2_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan2_raw);
}

/**
 * @brief Get field chan3_raw from rc_channels_override message
 *
 * @return RC channel 3 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan3_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan3_raw);
}

/**
 * @brief Get field chan4_raw from rc_channels_override message
 *
 * @return RC channel 4 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan4_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan4_raw);
}

/**
 * @brief Get field chan5_raw from rc_channels_override message
 *
 * @return RC channel 5 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan5_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan5_raw);
}

/**
 * @brief Get field chan6_raw from rc_channels_override message
 *
 * @return RC channel 6 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan6_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan6_raw);
}

/**
 * @brief Get field chan7_raw from rc_channels_override message
 *
 * @return RC channel 7 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan7_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan7_raw);
}

/**
 * @brief Get field chan8_raw from rc_channels_override message
 *
 * @return RC channel 8 value, in microseconds
 */
static inline uint16_t mavlink_msg_rc_channels_override_get_chan8_raw(const mavlink_message_t* msg)
{
	mavlink_rc_channels_override_t *p = (mavlink_rc_channels_override_t *)&msg->payload[0];
	return (uint16_t)(p->chan8_raw);
}

/**
 * @brief Decode a rc_channels_override message into a struct
 *
 * @param msg The message to decode
 * @param rc_channels_override C-struct to decode the message contents into
 */
static inline void mavlink_msg_rc_channels_override_decode(const mavlink_message_t* msg, mavlink_rc_channels_override_t* rc_channels_override)
{
	memcpy( rc_channels_override, msg->payload, sizeof(mavlink_rc_channels_override_t));
}
