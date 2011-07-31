// MESSAGE RC_CHANNELS_SCALED PACKING

#define MAVLINK_MSG_ID_RC_CHANNELS_SCALED 36
#define MAVLINK_MSG_ID_RC_CHANNELS_SCALED_LEN 17
#define MAVLINK_MSG_36_LEN 17

typedef struct __mavlink_rc_channels_scaled_t 
{
	int16_t chan1_scaled; ///< RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan2_scaled; ///< RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan3_scaled; ///< RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan4_scaled; ///< RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan5_scaled; ///< RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan6_scaled; ///< RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan7_scaled; ///< RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	int16_t chan8_scaled; ///< RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	uint8_t rssi; ///< Receive signal strength indicator, 0: 0%, 255: 100%

} mavlink_rc_channels_scaled_t;

/**
 * @brief Pack a rc_channels_scaled message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param chan1_scaled RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan2_scaled RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan3_scaled RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan4_scaled RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan5_scaled RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan6_scaled RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan7_scaled RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan8_scaled RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param rssi Receive signal strength indicator, 0: 0%, 255: 100%
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rc_channels_scaled_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, int16_t chan1_scaled, int16_t chan2_scaled, int16_t chan3_scaled, int16_t chan4_scaled, int16_t chan5_scaled, int16_t chan6_scaled, int16_t chan7_scaled, int16_t chan8_scaled, uint8_t rssi)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RC_CHANNELS_SCALED;

	p->chan1_scaled = chan1_scaled; // int16_t:RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan2_scaled = chan2_scaled; // int16_t:RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan3_scaled = chan3_scaled; // int16_t:RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan4_scaled = chan4_scaled; // int16_t:RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan5_scaled = chan5_scaled; // int16_t:RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan6_scaled = chan6_scaled; // int16_t:RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan7_scaled = chan7_scaled; // int16_t:RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan8_scaled = chan8_scaled; // int16_t:RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->rssi = rssi; // uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RC_CHANNELS_SCALED_LEN);
}

/**
 * @brief Pack a rc_channels_scaled message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param chan1_scaled RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan2_scaled RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan3_scaled RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan4_scaled RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan5_scaled RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan6_scaled RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan7_scaled RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan8_scaled RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param rssi Receive signal strength indicator, 0: 0%, 255: 100%
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rc_channels_scaled_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, int16_t chan1_scaled, int16_t chan2_scaled, int16_t chan3_scaled, int16_t chan4_scaled, int16_t chan5_scaled, int16_t chan6_scaled, int16_t chan7_scaled, int16_t chan8_scaled, uint8_t rssi)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RC_CHANNELS_SCALED;

	p->chan1_scaled = chan1_scaled; // int16_t:RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan2_scaled = chan2_scaled; // int16_t:RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan3_scaled = chan3_scaled; // int16_t:RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan4_scaled = chan4_scaled; // int16_t:RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan5_scaled = chan5_scaled; // int16_t:RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan6_scaled = chan6_scaled; // int16_t:RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan7_scaled = chan7_scaled; // int16_t:RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan8_scaled = chan8_scaled; // int16_t:RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->rssi = rssi; // uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RC_CHANNELS_SCALED_LEN);
}

/**
 * @brief Encode a rc_channels_scaled struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rc_channels_scaled C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rc_channels_scaled_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rc_channels_scaled_t* rc_channels_scaled)
{
	return mavlink_msg_rc_channels_scaled_pack(system_id, component_id, msg, rc_channels_scaled->chan1_scaled, rc_channels_scaled->chan2_scaled, rc_channels_scaled->chan3_scaled, rc_channels_scaled->chan4_scaled, rc_channels_scaled->chan5_scaled, rc_channels_scaled->chan6_scaled, rc_channels_scaled->chan7_scaled, rc_channels_scaled->chan8_scaled, rc_channels_scaled->rssi);
}

/**
 * @brief Send a rc_channels_scaled message
 * @param chan MAVLink channel to send the message
 *
 * @param chan1_scaled RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan2_scaled RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan3_scaled RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan4_scaled RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan5_scaled RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan6_scaled RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan7_scaled RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param chan8_scaled RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 * @param rssi Receive signal strength indicator, 0: 0%, 255: 100%
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_rc_channels_scaled_send(mavlink_channel_t chan, int16_t chan1_scaled, int16_t chan2_scaled, int16_t chan3_scaled, int16_t chan4_scaled, int16_t chan5_scaled, int16_t chan6_scaled, int16_t chan7_scaled, int16_t chan8_scaled, uint8_t rssi)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg.payload[0];

	p->chan1_scaled = chan1_scaled; // int16_t:RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan2_scaled = chan2_scaled; // int16_t:RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan3_scaled = chan3_scaled; // int16_t:RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan4_scaled = chan4_scaled; // int16_t:RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan5_scaled = chan5_scaled; // int16_t:RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan6_scaled = chan6_scaled; // int16_t:RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan7_scaled = chan7_scaled; // int16_t:RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan8_scaled = chan8_scaled; // int16_t:RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->rssi = rssi; // uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_RC_CHANNELS_SCALED_LEN;
	msg.msgid = MAVLINK_MSG_ID_RC_CHANNELS_SCALED;
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
static inline void mavlink_msg_rc_channels_scaled_send(mavlink_channel_t chan, int16_t chan1_scaled, int16_t chan2_scaled, int16_t chan3_scaled, int16_t chan4_scaled, int16_t chan5_scaled, int16_t chan6_scaled, int16_t chan7_scaled, int16_t chan8_scaled, uint8_t rssi)
{
	mavlink_header_t hdr;
	mavlink_rc_channels_scaled_t payload;
	uint16_t checksum;
	mavlink_rc_channels_scaled_t *p = &payload;

	p->chan1_scaled = chan1_scaled; // int16_t:RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan2_scaled = chan2_scaled; // int16_t:RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan3_scaled = chan3_scaled; // int16_t:RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan4_scaled = chan4_scaled; // int16_t:RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan5_scaled = chan5_scaled; // int16_t:RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan6_scaled = chan6_scaled; // int16_t:RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan7_scaled = chan7_scaled; // int16_t:RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->chan8_scaled = chan8_scaled; // int16_t:RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
	p->rssi = rssi; // uint8_t:Receive signal strength indicator, 0: 0%, 255: 100%

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_RC_CHANNELS_SCALED_LEN;
	hdr.msgid = MAVLINK_MSG_ID_RC_CHANNELS_SCALED;
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
// MESSAGE RC_CHANNELS_SCALED UNPACKING

/**
 * @brief Get field chan1_scaled from rc_channels_scaled message
 *
 * @return RC channel 1 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan1_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan1_scaled);
}

/**
 * @brief Get field chan2_scaled from rc_channels_scaled message
 *
 * @return RC channel 2 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan2_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan2_scaled);
}

/**
 * @brief Get field chan3_scaled from rc_channels_scaled message
 *
 * @return RC channel 3 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan3_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan3_scaled);
}

/**
 * @brief Get field chan4_scaled from rc_channels_scaled message
 *
 * @return RC channel 4 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan4_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan4_scaled);
}

/**
 * @brief Get field chan5_scaled from rc_channels_scaled message
 *
 * @return RC channel 5 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan5_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan5_scaled);
}

/**
 * @brief Get field chan6_scaled from rc_channels_scaled message
 *
 * @return RC channel 6 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan6_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan6_scaled);
}

/**
 * @brief Get field chan7_scaled from rc_channels_scaled message
 *
 * @return RC channel 7 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan7_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan7_scaled);
}

/**
 * @brief Get field chan8_scaled from rc_channels_scaled message
 *
 * @return RC channel 8 value scaled, (-100%) -10000, (0%) 0, (100%) 10000
 */
static inline int16_t mavlink_msg_rc_channels_scaled_get_chan8_scaled(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (int16_t)(p->chan8_scaled);
}

/**
 * @brief Get field rssi from rc_channels_scaled message
 *
 * @return Receive signal strength indicator, 0: 0%, 255: 100%
 */
static inline uint8_t mavlink_msg_rc_channels_scaled_get_rssi(const mavlink_message_t* msg)
{
	mavlink_rc_channels_scaled_t *p = (mavlink_rc_channels_scaled_t *)&msg->payload[0];
	return (uint8_t)(p->rssi);
}

/**
 * @brief Decode a rc_channels_scaled message into a struct
 *
 * @param msg The message to decode
 * @param rc_channels_scaled C-struct to decode the message contents into
 */
static inline void mavlink_msg_rc_channels_scaled_decode(const mavlink_message_t* msg, mavlink_rc_channels_scaled_t* rc_channels_scaled)
{
	memcpy( rc_channels_scaled, msg->payload, sizeof(mavlink_rc_channels_scaled_t));
}
