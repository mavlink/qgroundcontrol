// MESSAGE SCALED_IMU PACKING

#define MAVLINK_MSG_ID_SCALED_IMU 26

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
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	i += put_uint64_t_by_index(usec, i, msg->payload); // Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	i += put_int16_t_by_index(xacc, i, msg->payload); // X acceleration (mg)
	i += put_int16_t_by_index(yacc, i, msg->payload); // Y acceleration (mg)
	i += put_int16_t_by_index(zacc, i, msg->payload); // Z acceleration (mg)
	i += put_int16_t_by_index(xgyro, i, msg->payload); // Angular speed around X axis (millirad /sec)
	i += put_int16_t_by_index(ygyro, i, msg->payload); // Angular speed around Y axis (millirad /sec)
	i += put_int16_t_by_index(zgyro, i, msg->payload); // Angular speed around Z axis (millirad /sec)
	i += put_int16_t_by_index(xmag, i, msg->payload); // X Magnetic field (milli tesla)
	i += put_int16_t_by_index(ymag, i, msg->payload); // Y Magnetic field (milli tesla)
	i += put_int16_t_by_index(zmag, i, msg->payload); // Z Magnetic field (milli tesla)

	return mavlink_finalize_message(msg, system_id, component_id, i);
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
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	i += put_uint64_t_by_index(usec, i, msg->payload); // Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	i += put_int16_t_by_index(xacc, i, msg->payload); // X acceleration (mg)
	i += put_int16_t_by_index(yacc, i, msg->payload); // Y acceleration (mg)
	i += put_int16_t_by_index(zacc, i, msg->payload); // Z acceleration (mg)
	i += put_int16_t_by_index(xgyro, i, msg->payload); // Angular speed around X axis (millirad /sec)
	i += put_int16_t_by_index(ygyro, i, msg->payload); // Angular speed around Y axis (millirad /sec)
	i += put_int16_t_by_index(zgyro, i, msg->payload); // Angular speed around Z axis (millirad /sec)
	i += put_int16_t_by_index(xmag, i, msg->payload); // X Magnetic field (milli tesla)
	i += put_int16_t_by_index(ymag, i, msg->payload); // Y Magnetic field (milli tesla)
	i += put_int16_t_by_index(zmag, i, msg->payload); // Z Magnetic field (milli tesla)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
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
	mavlink_message_t msg;
	mavlink_msg_scaled_imu_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, usec, xacc, yacc, zacc, xgyro, ygyro, zgyro, xmag, ymag, zmag);
	mavlink_send_uart(chan, &msg);
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
	generic_64bit r;
	r.b[7] = (msg->payload)[0];
	r.b[6] = (msg->payload)[1];
	r.b[5] = (msg->payload)[2];
	r.b[4] = (msg->payload)[3];
	r.b[3] = (msg->payload)[4];
	r.b[2] = (msg->payload)[5];
	r.b[1] = (msg->payload)[6];
	r.b[0] = (msg->payload)[7];
	return (uint64_t)r.ll;
}

/**
 * @brief Get field xacc from scaled_imu message
 *
 * @return X acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xacc(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field yacc from scaled_imu message
 *
 * @return Y acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_yacc(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field zacc from scaled_imu message
 *
 * @return Z acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zacc(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field xgyro from scaled_imu message
 *
 * @return Angular speed around X axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xgyro(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field ygyro from scaled_imu message
 *
 * @return Angular speed around Y axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_ygyro(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field zgyro from scaled_imu message
 *
 * @return Angular speed around Z axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zgyro(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field xmag from scaled_imu message
 *
 * @return X Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xmag(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field ymag from scaled_imu message
 *
 * @return Y Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_ymag(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field zmag from scaled_imu message
 *
 * @return Z Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zmag(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Decode a scaled_imu message into a struct
 *
 * @param msg The message to decode
 * @param scaled_imu C-struct to decode the message contents into
 */
static inline void mavlink_msg_scaled_imu_decode(const mavlink_message_t* msg, mavlink_scaled_imu_t* scaled_imu)
{
	scaled_imu->usec = mavlink_msg_scaled_imu_get_usec(msg);
	scaled_imu->xacc = mavlink_msg_scaled_imu_get_xacc(msg);
	scaled_imu->yacc = mavlink_msg_scaled_imu_get_yacc(msg);
	scaled_imu->zacc = mavlink_msg_scaled_imu_get_zacc(msg);
	scaled_imu->xgyro = mavlink_msg_scaled_imu_get_xgyro(msg);
	scaled_imu->ygyro = mavlink_msg_scaled_imu_get_ygyro(msg);
	scaled_imu->zgyro = mavlink_msg_scaled_imu_get_zgyro(msg);
	scaled_imu->xmag = mavlink_msg_scaled_imu_get_xmag(msg);
	scaled_imu->ymag = mavlink_msg_scaled_imu_get_ymag(msg);
	scaled_imu->zmag = mavlink_msg_scaled_imu_get_zmag(msg);
}
