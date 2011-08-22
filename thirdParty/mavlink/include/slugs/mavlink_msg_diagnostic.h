// MESSAGE DIAGNOSTIC PACKING

#define MAVLINK_MSG_ID_DIAGNOSTIC 173
#define MAVLINK_MSG_ID_DIAGNOSTIC_LEN 18
#define MAVLINK_MSG_173_LEN 18
#define MAVLINK_MSG_ID_DIAGNOSTIC_KEY 0xFE
#define MAVLINK_MSG_173_KEY 0xFE

typedef struct __mavlink_diagnostic_t 
{
	float diagFl1;	///< Diagnostic float 1
	float diagFl2;	///< Diagnostic float 2
	float diagFl3;	///< Diagnostic float 3
	int16_t diagSh1;	///< Diagnostic short 1
	int16_t diagSh2;	///< Diagnostic short 2
	int16_t diagSh3;	///< Diagnostic short 3

} mavlink_diagnostic_t;

/**
 * @brief Pack a diagnostic message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param diagFl1 Diagnostic float 1
 * @param diagFl2 Diagnostic float 2
 * @param diagFl3 Diagnostic float 3
 * @param diagSh1 Diagnostic short 1
 * @param diagSh2 Diagnostic short 2
 * @param diagSh3 Diagnostic short 3
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_diagnostic_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float diagFl1, float diagFl2, float diagFl3, int16_t diagSh1, int16_t diagSh2, int16_t diagSh3)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DIAGNOSTIC;

	p->diagFl1 = diagFl1;	// float:Diagnostic float 1
	p->diagFl2 = diagFl2;	// float:Diagnostic float 2
	p->diagFl3 = diagFl3;	// float:Diagnostic float 3
	p->diagSh1 = diagSh1;	// int16_t:Diagnostic short 1
	p->diagSh2 = diagSh2;	// int16_t:Diagnostic short 2
	p->diagSh3 = diagSh3;	// int16_t:Diagnostic short 3

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DIAGNOSTIC_LEN);
}

/**
 * @brief Pack a diagnostic message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param diagFl1 Diagnostic float 1
 * @param diagFl2 Diagnostic float 2
 * @param diagFl3 Diagnostic float 3
 * @param diagSh1 Diagnostic short 1
 * @param diagSh2 Diagnostic short 2
 * @param diagSh3 Diagnostic short 3
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_diagnostic_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float diagFl1, float diagFl2, float diagFl3, int16_t diagSh1, int16_t diagSh2, int16_t diagSh3)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DIAGNOSTIC;

	p->diagFl1 = diagFl1;	// float:Diagnostic float 1
	p->diagFl2 = diagFl2;	// float:Diagnostic float 2
	p->diagFl3 = diagFl3;	// float:Diagnostic float 3
	p->diagSh1 = diagSh1;	// int16_t:Diagnostic short 1
	p->diagSh2 = diagSh2;	// int16_t:Diagnostic short 2
	p->diagSh3 = diagSh3;	// int16_t:Diagnostic short 3

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DIAGNOSTIC_LEN);
}

/**
 * @brief Encode a diagnostic struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param diagnostic C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_diagnostic_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_diagnostic_t* diagnostic)
{
	return mavlink_msg_diagnostic_pack(system_id, component_id, msg, diagnostic->diagFl1, diagnostic->diagFl2, diagnostic->diagFl3, diagnostic->diagSh1, diagnostic->diagSh2, diagnostic->diagSh3);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a diagnostic message
 * @param chan MAVLink channel to send the message
 *
 * @param diagFl1 Diagnostic float 1
 * @param diagFl2 Diagnostic float 2
 * @param diagFl3 Diagnostic float 3
 * @param diagSh1 Diagnostic short 1
 * @param diagSh2 Diagnostic short 2
 * @param diagSh3 Diagnostic short 3
 */
static inline void mavlink_msg_diagnostic_send(mavlink_channel_t chan, float diagFl1, float diagFl2, float diagFl3, int16_t diagSh1, int16_t diagSh2, int16_t diagSh3)
{
	mavlink_header_t hdr;
	mavlink_diagnostic_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_DIAGNOSTIC_LEN )
	payload.diagFl1 = diagFl1;	// float:Diagnostic float 1
	payload.diagFl2 = diagFl2;	// float:Diagnostic float 2
	payload.diagFl3 = diagFl3;	// float:Diagnostic float 3
	payload.diagSh1 = diagSh1;	// int16_t:Diagnostic short 1
	payload.diagSh2 = diagSh2;	// int16_t:Diagnostic short 2
	payload.diagSh3 = diagSh3;	// int16_t:Diagnostic short 3

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_DIAGNOSTIC_LEN;
	hdr.msgid = MAVLINK_MSG_ID_DIAGNOSTIC;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xFE, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE DIAGNOSTIC UNPACKING

/**
 * @brief Get field diagFl1 from diagnostic message
 *
 * @return Diagnostic float 1
 */
static inline float mavlink_msg_diagnostic_get_diagFl1(const mavlink_message_t* msg)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	return (float)(p->diagFl1);
}

/**
 * @brief Get field diagFl2 from diagnostic message
 *
 * @return Diagnostic float 2
 */
static inline float mavlink_msg_diagnostic_get_diagFl2(const mavlink_message_t* msg)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	return (float)(p->diagFl2);
}

/**
 * @brief Get field diagFl3 from diagnostic message
 *
 * @return Diagnostic float 3
 */
static inline float mavlink_msg_diagnostic_get_diagFl3(const mavlink_message_t* msg)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	return (float)(p->diagFl3);
}

/**
 * @brief Get field diagSh1 from diagnostic message
 *
 * @return Diagnostic short 1
 */
static inline int16_t mavlink_msg_diagnostic_get_diagSh1(const mavlink_message_t* msg)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	return (int16_t)(p->diagSh1);
}

/**
 * @brief Get field diagSh2 from diagnostic message
 *
 * @return Diagnostic short 2
 */
static inline int16_t mavlink_msg_diagnostic_get_diagSh2(const mavlink_message_t* msg)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	return (int16_t)(p->diagSh2);
}

/**
 * @brief Get field diagSh3 from diagnostic message
 *
 * @return Diagnostic short 3
 */
static inline int16_t mavlink_msg_diagnostic_get_diagSh3(const mavlink_message_t* msg)
{
	mavlink_diagnostic_t *p = (mavlink_diagnostic_t *)&msg->payload[0];
	return (int16_t)(p->diagSh3);
}

/**
 * @brief Decode a diagnostic message into a struct
 *
 * @param msg The message to decode
 * @param diagnostic C-struct to decode the message contents into
 */
static inline void mavlink_msg_diagnostic_decode(const mavlink_message_t* msg, mavlink_diagnostic_t* diagnostic)
{
	memcpy( diagnostic, msg->payload, sizeof(mavlink_diagnostic_t));
}
