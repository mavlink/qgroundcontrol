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

#define MAVLINK_MSG_ID_SCALED_IMU_LEN 26
#define MAVLINK_MSG_ID_26_LEN 26



#define MAVLINK_MESSAGE_INFO_SCALED_IMU { \
	"SCALED_IMU", \
	10, \
	{  { "usec", MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_scaled_imu_t, usec) }, \
         { "xacc", MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_scaled_imu_t, xacc) }, \
         { "yacc", MAVLINK_TYPE_INT16_T, 0, 10, offsetof(mavlink_scaled_imu_t, yacc) }, \
         { "zacc", MAVLINK_TYPE_INT16_T, 0, 12, offsetof(mavlink_scaled_imu_t, zacc) }, \
         { "xgyro", MAVLINK_TYPE_INT16_T, 0, 14, offsetof(mavlink_scaled_imu_t, xgyro) }, \
         { "ygyro", MAVLINK_TYPE_INT16_T, 0, 16, offsetof(mavlink_scaled_imu_t, ygyro) }, \
         { "zgyro", MAVLINK_TYPE_INT16_T, 0, 18, offsetof(mavlink_scaled_imu_t, zgyro) }, \
         { "xmag", MAVLINK_TYPE_INT16_T, 0, 20, offsetof(mavlink_scaled_imu_t, xmag) }, \
         { "ymag", MAVLINK_TYPE_INT16_T, 0, 22, offsetof(mavlink_scaled_imu_t, ymag) }, \
         { "zmag", MAVLINK_TYPE_INT16_T, 0, 24, offsetof(mavlink_scaled_imu_t, zmag) }, \
         } \
}


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
static inline uint16_t mavlink_msg_scaled_imu_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint64_t usec, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag)
{
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	put_uint64_t_by_index(msg, 0, usec); // Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	put_int16_t_by_index(msg, 8, xacc); // X acceleration (mg)
	put_int16_t_by_index(msg, 10, yacc); // Y acceleration (mg)
	put_int16_t_by_index(msg, 12, zacc); // Z acceleration (mg)
	put_int16_t_by_index(msg, 14, xgyro); // Angular speed around X axis (millirad /sec)
	put_int16_t_by_index(msg, 16, ygyro); // Angular speed around Y axis (millirad /sec)
	put_int16_t_by_index(msg, 18, zgyro); // Angular speed around Z axis (millirad /sec)
	put_int16_t_by_index(msg, 20, xmag); // X Magnetic field (milli tesla)
	put_int16_t_by_index(msg, 22, ymag); // Y Magnetic field (milli tesla)
	put_int16_t_by_index(msg, 24, zmag); // Z Magnetic field (milli tesla)

	return mavlink_finalize_message(msg, system_id, component_id, 26, 222);
}

/**
 * @brief Pack a scaled_imu message on a channel
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
static inline uint16_t mavlink_msg_scaled_imu_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint64_t usec,int16_t xacc,int16_t yacc,int16_t zacc,int16_t xgyro,int16_t ygyro,int16_t zgyro,int16_t xmag,int16_t ymag,int16_t zmag)
{
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	put_uint64_t_by_index(msg, 0, usec); // Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	put_int16_t_by_index(msg, 8, xacc); // X acceleration (mg)
	put_int16_t_by_index(msg, 10, yacc); // Y acceleration (mg)
	put_int16_t_by_index(msg, 12, zacc); // Z acceleration (mg)
	put_int16_t_by_index(msg, 14, xgyro); // Angular speed around X axis (millirad /sec)
	put_int16_t_by_index(msg, 16, ygyro); // Angular speed around Y axis (millirad /sec)
	put_int16_t_by_index(msg, 18, zgyro); // Angular speed around Z axis (millirad /sec)
	put_int16_t_by_index(msg, 20, xmag); // X Magnetic field (milli tesla)
	put_int16_t_by_index(msg, 22, ymag); // Y Magnetic field (milli tesla)
	put_int16_t_by_index(msg, 24, zmag); // Z Magnetic field (milli tesla)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 26, 222);
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
	MAVLINK_ALIGNED_MESSAGE(msg, 26);
	msg->msgid = MAVLINK_MSG_ID_SCALED_IMU;

	put_uint64_t_by_index(msg, 0, usec); // Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	put_int16_t_by_index(msg, 8, xacc); // X acceleration (mg)
	put_int16_t_by_index(msg, 10, yacc); // Y acceleration (mg)
	put_int16_t_by_index(msg, 12, zacc); // Z acceleration (mg)
	put_int16_t_by_index(msg, 14, xgyro); // Angular speed around X axis (millirad /sec)
	put_int16_t_by_index(msg, 16, ygyro); // Angular speed around Y axis (millirad /sec)
	put_int16_t_by_index(msg, 18, zgyro); // Angular speed around Z axis (millirad /sec)
	put_int16_t_by_index(msg, 20, xmag); // X Magnetic field (milli tesla)
	put_int16_t_by_index(msg, 22, ymag); // Y Magnetic field (milli tesla)
	put_int16_t_by_index(msg, 24, zmag); // Z Magnetic field (milli tesla)

	mavlink_finalize_message_chan_send(msg, chan, 26, 222);
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
	return MAVLINK_MSG_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field xacc from scaled_imu message
 *
 * @return X acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xacc(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  8);
}

/**
 * @brief Get field yacc from scaled_imu message
 *
 * @return Y acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_yacc(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  10);
}

/**
 * @brief Get field zacc from scaled_imu message
 *
 * @return Z acceleration (mg)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zacc(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  12);
}

/**
 * @brief Get field xgyro from scaled_imu message
 *
 * @return Angular speed around X axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xgyro(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  14);
}

/**
 * @brief Get field ygyro from scaled_imu message
 *
 * @return Angular speed around Y axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_ygyro(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  16);
}

/**
 * @brief Get field zgyro from scaled_imu message
 *
 * @return Angular speed around Z axis (millirad /sec)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zgyro(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  18);
}

/**
 * @brief Get field xmag from scaled_imu message
 *
 * @return X Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_xmag(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  20);
}

/**
 * @brief Get field ymag from scaled_imu message
 *
 * @return Y Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_ymag(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  22);
}

/**
 * @brief Get field zmag from scaled_imu message
 *
 * @return Z Magnetic field (milli tesla)
 */
static inline int16_t mavlink_msg_scaled_imu_get_zmag(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  24);
}

/**
 * @brief Decode a scaled_imu message into a struct
 *
 * @param msg The message to decode
 * @param scaled_imu C-struct to decode the message contents into
 */
static inline void mavlink_msg_scaled_imu_decode(const mavlink_message_t* msg, mavlink_scaled_imu_t* scaled_imu)
{
#if MAVLINK_NEED_BYTE_SWAP
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
#else
	memcpy(scaled_imu, MAVLINK_PAYLOAD(msg), 26);
#endif
}
