// MESSAGE SENSOR_OFFSETS PACKING

#define MAVLINK_MSG_ID_SENSOR_OFFSETS 150
#define MAVLINK_MSG_ID_SENSOR_OFFSETS_LEN 42
#define MAVLINK_MSG_150_LEN 42
#define MAVLINK_MSG_ID_SENSOR_OFFSETS_KEY 0xE2
#define MAVLINK_MSG_150_KEY 0xE2

typedef struct __mavlink_sensor_offsets_t 
{
	float mag_declination;	///< magnetic declination (radians)
	int32_t raw_press;	///< raw pressure from barometer
	int32_t raw_temp;	///< raw temperature from barometer
	float gyro_cal_x;	///< gyro X calibration
	float gyro_cal_y;	///< gyro Y calibration
	float gyro_cal_z;	///< gyro Z calibration
	float accel_cal_x;	///< accel X calibration
	float accel_cal_y;	///< accel Y calibration
	float accel_cal_z;	///< accel Z calibration
	int16_t mag_ofs_x;	///< magnetometer X offset
	int16_t mag_ofs_y;	///< magnetometer Y offset
	int16_t mag_ofs_z;	///< magnetometer Z offset

} mavlink_sensor_offsets_t;

/**
 * @brief Pack a sensor_offsets message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param mag_ofs_x magnetometer X offset
 * @param mag_ofs_y magnetometer Y offset
 * @param mag_ofs_z magnetometer Z offset
 * @param mag_declination magnetic declination (radians)
 * @param raw_press raw pressure from barometer
 * @param raw_temp raw temperature from barometer
 * @param gyro_cal_x gyro X calibration
 * @param gyro_cal_y gyro Y calibration
 * @param gyro_cal_z gyro Z calibration
 * @param accel_cal_x accel X calibration
 * @param accel_cal_y accel Y calibration
 * @param accel_cal_z accel Z calibration
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sensor_offsets_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, int16_t mag_ofs_x, int16_t mag_ofs_y, int16_t mag_ofs_z, float mag_declination, int32_t raw_press, int32_t raw_temp, float gyro_cal_x, float gyro_cal_y, float gyro_cal_z, float accel_cal_x, float accel_cal_y, float accel_cal_z)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SENSOR_OFFSETS;

	p->mag_ofs_x = mag_ofs_x;	// int16_t:magnetometer X offset
	p->mag_ofs_y = mag_ofs_y;	// int16_t:magnetometer Y offset
	p->mag_ofs_z = mag_ofs_z;	// int16_t:magnetometer Z offset
	p->mag_declination = mag_declination;	// float:magnetic declination (radians)
	p->raw_press = raw_press;	// int32_t:raw pressure from barometer
	p->raw_temp = raw_temp;	// int32_t:raw temperature from barometer
	p->gyro_cal_x = gyro_cal_x;	// float:gyro X calibration
	p->gyro_cal_y = gyro_cal_y;	// float:gyro Y calibration
	p->gyro_cal_z = gyro_cal_z;	// float:gyro Z calibration
	p->accel_cal_x = accel_cal_x;	// float:accel X calibration
	p->accel_cal_y = accel_cal_y;	// float:accel Y calibration
	p->accel_cal_z = accel_cal_z;	// float:accel Z calibration

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SENSOR_OFFSETS_LEN);
}

/**
 * @brief Pack a sensor_offsets message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param mag_ofs_x magnetometer X offset
 * @param mag_ofs_y magnetometer Y offset
 * @param mag_ofs_z magnetometer Z offset
 * @param mag_declination magnetic declination (radians)
 * @param raw_press raw pressure from barometer
 * @param raw_temp raw temperature from barometer
 * @param gyro_cal_x gyro X calibration
 * @param gyro_cal_y gyro Y calibration
 * @param gyro_cal_z gyro Z calibration
 * @param accel_cal_x accel X calibration
 * @param accel_cal_y accel Y calibration
 * @param accel_cal_z accel Z calibration
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sensor_offsets_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, int16_t mag_ofs_x, int16_t mag_ofs_y, int16_t mag_ofs_z, float mag_declination, int32_t raw_press, int32_t raw_temp, float gyro_cal_x, float gyro_cal_y, float gyro_cal_z, float accel_cal_x, float accel_cal_y, float accel_cal_z)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SENSOR_OFFSETS;

	p->mag_ofs_x = mag_ofs_x;	// int16_t:magnetometer X offset
	p->mag_ofs_y = mag_ofs_y;	// int16_t:magnetometer Y offset
	p->mag_ofs_z = mag_ofs_z;	// int16_t:magnetometer Z offset
	p->mag_declination = mag_declination;	// float:magnetic declination (radians)
	p->raw_press = raw_press;	// int32_t:raw pressure from barometer
	p->raw_temp = raw_temp;	// int32_t:raw temperature from barometer
	p->gyro_cal_x = gyro_cal_x;	// float:gyro X calibration
	p->gyro_cal_y = gyro_cal_y;	// float:gyro Y calibration
	p->gyro_cal_z = gyro_cal_z;	// float:gyro Z calibration
	p->accel_cal_x = accel_cal_x;	// float:accel X calibration
	p->accel_cal_y = accel_cal_y;	// float:accel Y calibration
	p->accel_cal_z = accel_cal_z;	// float:accel Z calibration

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SENSOR_OFFSETS_LEN);
}

/**
 * @brief Encode a sensor_offsets struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sensor_offsets C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sensor_offsets_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sensor_offsets_t* sensor_offsets)
{
	return mavlink_msg_sensor_offsets_pack(system_id, component_id, msg, sensor_offsets->mag_ofs_x, sensor_offsets->mag_ofs_y, sensor_offsets->mag_ofs_z, sensor_offsets->mag_declination, sensor_offsets->raw_press, sensor_offsets->raw_temp, sensor_offsets->gyro_cal_x, sensor_offsets->gyro_cal_y, sensor_offsets->gyro_cal_z, sensor_offsets->accel_cal_x, sensor_offsets->accel_cal_y, sensor_offsets->accel_cal_z);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a sensor_offsets message
 * @param chan MAVLink channel to send the message
 *
 * @param mag_ofs_x magnetometer X offset
 * @param mag_ofs_y magnetometer Y offset
 * @param mag_ofs_z magnetometer Z offset
 * @param mag_declination magnetic declination (radians)
 * @param raw_press raw pressure from barometer
 * @param raw_temp raw temperature from barometer
 * @param gyro_cal_x gyro X calibration
 * @param gyro_cal_y gyro Y calibration
 * @param gyro_cal_z gyro Z calibration
 * @param accel_cal_x accel X calibration
 * @param accel_cal_y accel Y calibration
 * @param accel_cal_z accel Z calibration
 */
static inline void mavlink_msg_sensor_offsets_send(mavlink_channel_t chan, int16_t mag_ofs_x, int16_t mag_ofs_y, int16_t mag_ofs_z, float mag_declination, int32_t raw_press, int32_t raw_temp, float gyro_cal_x, float gyro_cal_y, float gyro_cal_z, float accel_cal_x, float accel_cal_y, float accel_cal_z)
{
	mavlink_header_t hdr;
	mavlink_sensor_offsets_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SENSOR_OFFSETS_LEN )
	payload.mag_ofs_x = mag_ofs_x;	// int16_t:magnetometer X offset
	payload.mag_ofs_y = mag_ofs_y;	// int16_t:magnetometer Y offset
	payload.mag_ofs_z = mag_ofs_z;	// int16_t:magnetometer Z offset
	payload.mag_declination = mag_declination;	// float:magnetic declination (radians)
	payload.raw_press = raw_press;	// int32_t:raw pressure from barometer
	payload.raw_temp = raw_temp;	// int32_t:raw temperature from barometer
	payload.gyro_cal_x = gyro_cal_x;	// float:gyro X calibration
	payload.gyro_cal_y = gyro_cal_y;	// float:gyro Y calibration
	payload.gyro_cal_z = gyro_cal_z;	// float:gyro Z calibration
	payload.accel_cal_x = accel_cal_x;	// float:accel X calibration
	payload.accel_cal_y = accel_cal_y;	// float:accel Y calibration
	payload.accel_cal_z = accel_cal_z;	// float:accel Z calibration

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SENSOR_OFFSETS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SENSOR_OFFSETS;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xE2, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SENSOR_OFFSETS UNPACKING

/**
 * @brief Get field mag_ofs_x from sensor_offsets message
 *
 * @return magnetometer X offset
 */
static inline int16_t mavlink_msg_sensor_offsets_get_mag_ofs_x(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (int16_t)(p->mag_ofs_x);
}

/**
 * @brief Get field mag_ofs_y from sensor_offsets message
 *
 * @return magnetometer Y offset
 */
static inline int16_t mavlink_msg_sensor_offsets_get_mag_ofs_y(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (int16_t)(p->mag_ofs_y);
}

/**
 * @brief Get field mag_ofs_z from sensor_offsets message
 *
 * @return magnetometer Z offset
 */
static inline int16_t mavlink_msg_sensor_offsets_get_mag_ofs_z(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (int16_t)(p->mag_ofs_z);
}

/**
 * @brief Get field mag_declination from sensor_offsets message
 *
 * @return magnetic declination (radians)
 */
static inline float mavlink_msg_sensor_offsets_get_mag_declination(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->mag_declination);
}

/**
 * @brief Get field raw_press from sensor_offsets message
 *
 * @return raw pressure from barometer
 */
static inline int32_t mavlink_msg_sensor_offsets_get_raw_press(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (int32_t)(p->raw_press);
}

/**
 * @brief Get field raw_temp from sensor_offsets message
 *
 * @return raw temperature from barometer
 */
static inline int32_t mavlink_msg_sensor_offsets_get_raw_temp(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (int32_t)(p->raw_temp);
}

/**
 * @brief Get field gyro_cal_x from sensor_offsets message
 *
 * @return gyro X calibration
 */
static inline float mavlink_msg_sensor_offsets_get_gyro_cal_x(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->gyro_cal_x);
}

/**
 * @brief Get field gyro_cal_y from sensor_offsets message
 *
 * @return gyro Y calibration
 */
static inline float mavlink_msg_sensor_offsets_get_gyro_cal_y(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->gyro_cal_y);
}

/**
 * @brief Get field gyro_cal_z from sensor_offsets message
 *
 * @return gyro Z calibration
 */
static inline float mavlink_msg_sensor_offsets_get_gyro_cal_z(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->gyro_cal_z);
}

/**
 * @brief Get field accel_cal_x from sensor_offsets message
 *
 * @return accel X calibration
 */
static inline float mavlink_msg_sensor_offsets_get_accel_cal_x(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->accel_cal_x);
}

/**
 * @brief Get field accel_cal_y from sensor_offsets message
 *
 * @return accel Y calibration
 */
static inline float mavlink_msg_sensor_offsets_get_accel_cal_y(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->accel_cal_y);
}

/**
 * @brief Get field accel_cal_z from sensor_offsets message
 *
 * @return accel Z calibration
 */
static inline float mavlink_msg_sensor_offsets_get_accel_cal_z(const mavlink_message_t* msg)
{
	mavlink_sensor_offsets_t *p = (mavlink_sensor_offsets_t *)&msg->payload[0];
	return (float)(p->accel_cal_z);
}

/**
 * @brief Decode a sensor_offsets message into a struct
 *
 * @param msg The message to decode
 * @param sensor_offsets C-struct to decode the message contents into
 */
static inline void mavlink_msg_sensor_offsets_decode(const mavlink_message_t* msg, mavlink_sensor_offsets_t* sensor_offsets)
{
	memcpy( sensor_offsets, msg->payload, sizeof(mavlink_sensor_offsets_t));
}
