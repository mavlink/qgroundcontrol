// MESSAGE SCALED_IMU PACKING

#define MAVLINK_MSG_ID_SCALED_IMU 26
#define MAVLINK_MSG_ID_SCALED_IMU_LEN 26
#define MAVLINK_MSG_26_LEN 26

typedef struct __mavlink_scaled_imu_t 
{
	uint64_t usec; ///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	int16_t xacc; ///< X acceleration (mg)
	int16_t yacc; ///< Y acceleration (mg)
	int16_t zacc; ///< Z acceleration (mg)
	int16_t xgyro; ///< Angular speed around X axis (millirad /sec)
	int16_t ygyro; ///< Angular speed around Y axis (millirad /sec)
	int16_t zgyro; ///< Angular speed around Z axis (millirad /sec)
	int16_t xmag; ///< X Magnetic field (milli tesla)
	int16_t ymag; ///< Y Magnetic field (milli tesla)
	int16_t zmag; ///< Z Magnetic field (milli tesla)

} mavlink_scaled_imu_t;

/**
 * @brief Pack a scaled_imu message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @param xgyro Angular speed around X axis (millirad /sec)
 * @param ygyro Angular speed around Y axis (millirad /sec)
 * @param zgyro Angular speed around Z axis (millirad /sec)
 * @param xmag X Magnetic field (milli tesla)
 * @param ymag Y Magnetic field (milli tesla)
 * @param zmag Z Magnetic field (milli tesla)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_scaled_imu_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->xacc = xacc; // int16_t:X acceleration (mg)
	p->yacc = yacc; // int16_t:Y acceleration (mg)
	p->zacc = zacc; // int16_t:Z acceleration (mg)
	p->xgyro = xgyro; // int16_t:Angular speed around X axis (millirad /sec)
	p->ygyro = ygyro; // int16_t:Angular speed around Y axis (millirad /sec)
	p->zgyro = zgyro; // int16_t:Angular speed around Z axis (millirad /sec)
	p->xmag = xmag; // int16_t:X Magnetic field (milli tesla)
	p->ymag = ymag; // int16_t:Y Magnetic field (milli tesla)
	p->zmag = zmag; // int16_t:Z Magnetic field (milli tesla)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SCALED_IMU_LEN);
}

/**
 * @brief Pack a scaled_imu message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @param xgyro Angular speed around X axis (millirad /sec)
 * @param ygyro Angular speed around Y axis (millirad /sec)
 * @param zgyro Angular speed around Z axis (millirad /sec)
 * @param xmag X Magnetic field (milli tesla)
 * @param ymag Y Magnetic field (milli tesla)
 * @param zmag Z Magnetic field (milli tesla)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_scaled_imu_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->xacc = xacc; // int16_t:X acceleration (mg)
	p->yacc = yacc; // int16_t:Y acceleration (mg)
	p->zacc = zacc; // int16_t:Z acceleration (mg)
	p->xgyro = xgyro; // int16_t:Angular speed around X axis (millirad /sec)
	p->ygyro = ygyro; // int16_t:Angular speed around Y axis (millirad /sec)
	p->zgyro = zgyro; // int16_t:Angular speed around Z axis (millirad /sec)
	p->xmag = xmag; // int16_t:X Magnetic field (milli tesla)
	p->ymag = ymag; // int16_t:Y Magnetic field (milli tesla)
	p->zmag = zmag; // int16_t:Z Magnetic field (milli tesla)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SCALED_IMU_LEN);
}

/**
 * @brief Encode a scaled_imu struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param scaled_imu C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_scaled_imu_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_scaled_imu_t* scaled_imu)
{
	return mavlink_msg_scaled_imu_pack(system_id, component_id, msg, scaled_imu->usec, scaled_imu->xacc, scaled_imu->yacc, scaled_imu->zacc, scaled_imu->xgyro, scaled_imu->ygyro, scaled_imu->zgyro, scaled_imu->xmag, scaled_imu->ymag, scaled_imu->zmag);
}

/**
 * @brief Send a scaled_imu message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @param xgyro Angular speed around X axis (millirad /sec)
 * @param ygyro Angular speed around Y axis (millirad /sec)
 * @param zgyro Angular speed around Z axis (millirad /sec)
 * @param xmag X Magnetic field (milli tesla)
 * @param ymag Y Magnetic field (milli tesla)
 * @param zmag Z Magnetic field (milli tesla)
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_scaled_imu_send(mavlink_channel_t chan, uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	mavlink_header_t hdr;
	mavlink_scaled_imu_t payload;
	uint16_t checksum;
	mavlink_scaled_imu_t *p = &payload;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->xacc = xacc; // int16_t:X acceleration (mg)
	p->yacc = yacc; // int16_t:Y acceleration (mg)
	p->zacc = zacc; // int16_t:Z acceleration (mg)
	p->xgyro = xgyro; // int16_t:Angular speed around X axis (millirad /sec)
	p->ygyro = ygyro; // int16_t:Angular speed around Y axis (millirad /sec)
	p->zgyro = zgyro; // int16_t:Angular speed around Z axis (millirad /sec)
	p->xmag = xmag; // int16_t:X Magnetic field (milli tesla)
	p->ymag = ymag; // int16_t:Y Magnetic field (milli tesla)
	p->zmag = zmag; // int16_t:Z Magnetic field (milli tesla)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SCALED_IMU_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SCALED_IMU;
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
// MESSAGE SCALED_IMU UNPACKING

/**
 * @brief Get field usec from scaled_imu message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_scaled_imu_get_usec(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field xacc from scaled_imu message
 *
 * @return X acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xacc(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->xacc);
}

/**
 * @brief Get field yacc from scaled_imu message
 *
 * @return Y acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_yacc(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->yacc);
}

/**
 * @brief Get field zacc from scaled_imu message
 *
 * @return Z acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zacc(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->zacc);
}

/**
 * @brief Get field xgyro from scaled_imu message
 *
 * @return Angular speed around X axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xgyro(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->xgyro);
}

/**
 * @brief Get field ygyro from scaled_imu message
 *
 * @return Angular speed around Y axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_ygyro(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->ygyro);
}

/**
 * @brief Get field zgyro from scaled_imu message
 *
 * @return Angular speed around Z axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zgyro(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->zgyro);
}

/**
 * @brief Get field xmag from scaled_imu message
 *
 * @return X Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xmag(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->xmag);
}

/**
 * @brief Get field ymag from scaled_imu message
 *
 * @return Y Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_ymag(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->ymag);
}

/**
 * @brief Get field zmag from scaled_imu message
 *
 * @return Z Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zmag(const mavlink_message_t* msg)
{
	mavlink_scaled_imu_t *p = (mavlink_scaled_imu_t *)&msg->payload[0];
	return (int16_t)(p->zmag);
}

/**
 * @brief Decode a scaled_imu message into a struct
 *
 * @param msg The message to decode
 * @param scaled_imu C-struct to decode the message contents into
 */
static inline void mavlink_msg_scaled_imu_decode(const mavlink_message_t* msg, mavlink_scaled_imu_t* scaled_imu)
{
	memcpy( scaled_imu, msg->payload, sizeof(mavlink_scaled_imu_t));
}
