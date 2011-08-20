// MESSAGE RAW_IMU PACKING

#define MAVLINK_MSG_ID_RAW_IMU 28
#define MAVLINK_MSG_ID_RAW_IMU_LEN 26
#define MAVLINK_MSG_28_LEN 26
#define MAVLINK_MSG_ID_RAW_IMU_KEY 0x1C
#define MAVLINK_MSG_28_KEY 0x1C

typedef struct __mavlink_raw_imu_t 
{
	uint64_t usec;	///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	int16_t xacc;	///< X acceleration (raw)
	int16_t yacc;	///< Y acceleration (raw)
	int16_t zacc;	///< Z acceleration (raw)
	int16_t xgyro;	///< Angular speed around X axis (raw)
	int16_t ygyro;	///< Angular speed around Y axis (raw)
	int16_t zgyro;	///< Angular speed around Z axis (raw)
	int16_t xmag;	///< X Magnetic field (raw)
	int16_t ymag;	///< Y Magnetic field (raw)
	int16_t zmag;	///< Z Magnetic field (raw)

} mavlink_raw_imu_t;

/**
 * @brief Pack a raw_imu message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param xacc X acceleration (raw)
 * @param yacc Y acceleration (raw)
 * @param zacc Z acceleration (raw)
 * @param xgyro Angular speed around X axis (raw)
 * @param ygyro Angular speed around Y axis (raw)
 * @param zgyro Angular speed around Z axis (raw)
 * @param xmag X Magnetic field (raw)
 * @param ymag Y Magnetic field (raw)
 * @param zmag Z Magnetic field (raw)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_raw_imu_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RAW_IMU;

	p->usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->xacc = xacc;	// int16_t:X acceleration (raw)
	p->yacc = yacc;	// int16_t:Y acceleration (raw)
	p->zacc = zacc;	// int16_t:Z acceleration (raw)
	p->xgyro = xgyro;	// int16_t:Angular speed around X axis (raw)
	p->ygyro = ygyro;	// int16_t:Angular speed around Y axis (raw)
	p->zgyro = zgyro;	// int16_t:Angular speed around Z axis (raw)
	p->xmag = xmag;	// int16_t:X Magnetic field (raw)
	p->ymag = ymag;	// int16_t:Y Magnetic field (raw)
	p->zmag = zmag;	// int16_t:Z Magnetic field (raw)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RAW_IMU_LEN);
}

/**
 * @brief Pack a raw_imu message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param xacc X acceleration (raw)
 * @param yacc Y acceleration (raw)
 * @param zacc Z acceleration (raw)
 * @param xgyro Angular speed around X axis (raw)
 * @param ygyro Angular speed around Y axis (raw)
 * @param zgyro Angular speed around Z axis (raw)
 * @param xmag X Magnetic field (raw)
 * @param ymag Y Magnetic field (raw)
 * @param zmag Z Magnetic field (raw)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_raw_imu_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RAW_IMU;

	p->usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->xacc = xacc;	// int16_t:X acceleration (raw)
	p->yacc = yacc;	// int16_t:Y acceleration (raw)
	p->zacc = zacc;	// int16_t:Z acceleration (raw)
	p->xgyro = xgyro;	// int16_t:Angular speed around X axis (raw)
	p->ygyro = ygyro;	// int16_t:Angular speed around Y axis (raw)
	p->zgyro = zgyro;	// int16_t:Angular speed around Z axis (raw)
	p->xmag = xmag;	// int16_t:X Magnetic field (raw)
	p->ymag = ymag;	// int16_t:Y Magnetic field (raw)
	p->zmag = zmag;	// int16_t:Z Magnetic field (raw)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RAW_IMU_LEN);
}

/**
 * @brief Encode a raw_imu struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param raw_imu C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_raw_imu_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_raw_imu_t* raw_imu)
{
	return mavlink_msg_raw_imu_pack(system_id, component_id, msg, raw_imu->usec, raw_imu->xacc, raw_imu->yacc, raw_imu->zacc, raw_imu->xgyro, raw_imu->ygyro, raw_imu->zgyro, raw_imu->xmag, raw_imu->ymag, raw_imu->zmag);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a raw_imu message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param xacc X acceleration (raw)
 * @param yacc Y acceleration (raw)
 * @param zacc Z acceleration (raw)
 * @param xgyro Angular speed around X axis (raw)
 * @param ygyro Angular speed around Y axis (raw)
 * @param zgyro Angular speed around Z axis (raw)
 * @param xmag X Magnetic field (raw)
 * @param ymag Y Magnetic field (raw)
 * @param zmag Z Magnetic field (raw)
 */
static inline void mavlink_msg_raw_imu_send(mavlink_channel_t chan, uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	mavlink_header_t hdr;
	mavlink_raw_imu_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_RAW_IMU_LEN )
	payload.usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	payload.xacc = xacc;	// int16_t:X acceleration (raw)
	payload.yacc = yacc;	// int16_t:Y acceleration (raw)
	payload.zacc = zacc;	// int16_t:Z acceleration (raw)
	payload.xgyro = xgyro;	// int16_t:Angular speed around X axis (raw)
	payload.ygyro = ygyro;	// int16_t:Angular speed around Y axis (raw)
	payload.zgyro = zgyro;	// int16_t:Angular speed around Z axis (raw)
	payload.xmag = xmag;	// int16_t:X Magnetic field (raw)
	payload.ymag = ymag;	// int16_t:Y Magnetic field (raw)
	payload.zmag = zmag;	// int16_t:Z Magnetic field (raw)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_RAW_IMU_LEN;
	hdr.msgid = MAVLINK_MSG_ID_RAW_IMU;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x1C, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE RAW_IMU UNPACKING

/**
 * @brief Get field usec from raw_imu message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_raw_imu_get_usec(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field xacc from raw_imu message
 *
 * @return X acceleration (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_xacc(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->xacc);
}

/**
 * @brief Get field yacc from raw_imu message
 *
 * @return Y acceleration (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_yacc(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->yacc);
}

/**
 * @brief Get field zacc from raw_imu message
 *
 * @return Z acceleration (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_zacc(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->zacc);
}

/**
 * @brief Get field xgyro from raw_imu message
 *
 * @return Angular speed around X axis (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_xgyro(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->xgyro);
}

/**
 * @brief Get field ygyro from raw_imu message
 *
 * @return Angular speed around Y axis (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_ygyro(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->ygyro);
}

/**
 * @brief Get field zgyro from raw_imu message
 *
 * @return Angular speed around Z axis (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_zgyro(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->zgyro);
}

/**
 * @brief Get field xmag from raw_imu message
 *
 * @return X Magnetic field (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_xmag(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->xmag);
}

/**
 * @brief Get field ymag from raw_imu message
 *
 * @return Y Magnetic field (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_ymag(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->ymag);
}

/**
 * @brief Get field zmag from raw_imu message
 *
 * @return Z Magnetic field (raw)
 */
static inline int16_t mavlink_msg_raw_imu_get_zmag(const mavlink_message_t* msg)
{
	mavlink_raw_imu_t *p = (mavlink_raw_imu_t *)&msg->payload[0];
	return (int16_t)(p->zmag);
}

/**
 * @brief Decode a raw_imu message into a struct
 *
 * @param msg The message to decode
 * @param raw_imu C-struct to decode the message contents into
 */
static inline void mavlink_msg_raw_imu_decode(const mavlink_message_t* msg, mavlink_raw_imu_t* raw_imu)
{
	memcpy( raw_imu, msg->payload, sizeof(mavlink_raw_imu_t));
}
