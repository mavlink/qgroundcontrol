// MESSAGE HIL_SENSOR PACKING

#define MAVLINK_MSG_ID_HIL_SENSOR 107

typedef struct __mavlink_hil_sensor_t
{
 uint64_t time_usec; ///< Timestamp (microseconds, synced to UNIX time or since system boot)
 float roll; ///< Roll angle in inertial frame (rad)
 float pitch; ///< Pitch angle in inertial frame (rad)
 float yaw; ///< Yaw angle in inertial frame (rad)
 int32_t lat; ///< Latitude, expressed as * 1E7 degrees
 int32_t lon; ///< Longitude, expressed as * 1E7 degrees
 float xacc; ///< X acceleration (m/s^2)
 float yacc; ///< Y acceleration (m/s^2)
 float zacc; ///< Z acceleration (m/s^2)
 float xgyro; ///< Angular speed around X axis in body frame (rad / sec)
 float ygyro; ///< Angular speed around Y axis in body frame (rad / sec)
 float zgyro; ///< Angular speed around Z axis in body frame (rad / sec)
 float xmag; ///< X Magnetic field (Gauss)
 float ymag; ///< Y Magnetic field (Gauss)
 float zmag; ///< Z Magnetic field (Gauss)
 float abs_pressure; ///< Absolute pressure in millibar
 float diff_pressure; ///< Differential pressure (airspeed) in millibar
 float pressure_alt; ///< Altitude calculated from pressure
 float gps_alt; ///< GPS altitude (MSL) WGS84
 float temperature; ///< Temperature in degrees celsius
 uint32_t fields_updated; ///< Bitmask for fields that have updated since last message, bit 0 = xacc, bit 12: temperature
} mavlink_hil_sensor_t;

#define MAVLINK_MSG_ID_HIL_SENSOR_LEN 88
#define MAVLINK_MSG_ID_107_LEN 88

#define MAVLINK_MSG_ID_HIL_SENSOR_CRC 5
#define MAVLINK_MSG_ID_107_CRC 5



#define MAVLINK_MESSAGE_INFO_HIL_SENSOR { \
	"HIL_SENSOR", \
	21, \
	{  { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_hil_sensor_t, time_usec) }, \
         { "roll", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_hil_sensor_t, roll) }, \
         { "pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_hil_sensor_t, pitch) }, \
         { "yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_hil_sensor_t, yaw) }, \
         { "lat", NULL, MAVLINK_TYPE_INT32_T, 0, 20, offsetof(mavlink_hil_sensor_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_INT32_T, 0, 24, offsetof(mavlink_hil_sensor_t, lon) }, \
         { "xacc", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_hil_sensor_t, xacc) }, \
         { "yacc", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_hil_sensor_t, yacc) }, \
         { "zacc", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_hil_sensor_t, zacc) }, \
         { "xgyro", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_hil_sensor_t, xgyro) }, \
         { "ygyro", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_hil_sensor_t, ygyro) }, \
         { "zgyro", NULL, MAVLINK_TYPE_FLOAT, 0, 48, offsetof(mavlink_hil_sensor_t, zgyro) }, \
         { "xmag", NULL, MAVLINK_TYPE_FLOAT, 0, 52, offsetof(mavlink_hil_sensor_t, xmag) }, \
         { "ymag", NULL, MAVLINK_TYPE_FLOAT, 0, 56, offsetof(mavlink_hil_sensor_t, ymag) }, \
         { "zmag", NULL, MAVLINK_TYPE_FLOAT, 0, 60, offsetof(mavlink_hil_sensor_t, zmag) }, \
         { "abs_pressure", NULL, MAVLINK_TYPE_FLOAT, 0, 64, offsetof(mavlink_hil_sensor_t, abs_pressure) }, \
         { "diff_pressure", NULL, MAVLINK_TYPE_FLOAT, 0, 68, offsetof(mavlink_hil_sensor_t, diff_pressure) }, \
         { "pressure_alt", NULL, MAVLINK_TYPE_FLOAT, 0, 72, offsetof(mavlink_hil_sensor_t, pressure_alt) }, \
         { "gps_alt", NULL, MAVLINK_TYPE_FLOAT, 0, 76, offsetof(mavlink_hil_sensor_t, gps_alt) }, \
         { "temperature", NULL, MAVLINK_TYPE_FLOAT, 0, 80, offsetof(mavlink_hil_sensor_t, temperature) }, \
         { "fields_updated", NULL, MAVLINK_TYPE_UINT32_T, 0, 84, offsetof(mavlink_hil_sensor_t, fields_updated) }, \
         } \
}


/**
 * @brief Pack a hil_sensor message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec Timestamp (microseconds, synced to UNIX time or since system boot)
 * @param roll Roll angle in inertial frame (rad)
 * @param pitch Pitch angle in inertial frame (rad)
 * @param yaw Yaw angle in inertial frame (rad)
 * @param lat Latitude, expressed as * 1E7 degrees
 * @param lon Longitude, expressed as * 1E7 degrees
 * @param xacc X acceleration (m/s^2)
 * @param yacc Y acceleration (m/s^2)
 * @param zacc Z acceleration (m/s^2)
 * @param xgyro Angular speed around X axis in body frame (rad / sec)
 * @param ygyro Angular speed around Y axis in body frame (rad / sec)
 * @param zgyro Angular speed around Z axis in body frame (rad / sec)
 * @param xmag X Magnetic field (Gauss)
 * @param ymag Y Magnetic field (Gauss)
 * @param zmag Z Magnetic field (Gauss)
 * @param abs_pressure Absolute pressure in millibar
 * @param diff_pressure Differential pressure (airspeed) in millibar
 * @param pressure_alt Altitude calculated from pressure
 * @param gps_alt GPS altitude (MSL) WGS84
 * @param temperature Temperature in degrees celsius
 * @param fields_updated Bitmask for fields that have updated since last message, bit 0 = xacc, bit 12: temperature
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_sensor_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint64_t time_usec, float roll, float pitch, float yaw, int32_t lat, int32_t lon, float xacc, float yacc, float zacc, float xgyro, float ygyro, float zgyro, float xmag, float ymag, float zmag, float abs_pressure, float diff_pressure, float pressure_alt, float gps_alt, float temperature, uint32_t fields_updated)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_HIL_SENSOR_LEN];
	_mav_put_uint64_t(buf, 0, time_usec);
	_mav_put_float(buf, 8, roll);
	_mav_put_float(buf, 12, pitch);
	_mav_put_float(buf, 16, yaw);
	_mav_put_int32_t(buf, 20, lat);
	_mav_put_int32_t(buf, 24, lon);
	_mav_put_float(buf, 28, xacc);
	_mav_put_float(buf, 32, yacc);
	_mav_put_float(buf, 36, zacc);
	_mav_put_float(buf, 40, xgyro);
	_mav_put_float(buf, 44, ygyro);
	_mav_put_float(buf, 48, zgyro);
	_mav_put_float(buf, 52, xmag);
	_mav_put_float(buf, 56, ymag);
	_mav_put_float(buf, 60, zmag);
	_mav_put_float(buf, 64, abs_pressure);
	_mav_put_float(buf, 68, diff_pressure);
	_mav_put_float(buf, 72, pressure_alt);
	_mav_put_float(buf, 76, gps_alt);
	_mav_put_float(buf, 80, temperature);
	_mav_put_uint32_t(buf, 84, fields_updated);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#else
	mavlink_hil_sensor_t packet;
	packet.time_usec = time_usec;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;
	packet.lat = lat;
	packet.lon = lon;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	packet.xgyro = xgyro;
	packet.ygyro = ygyro;
	packet.zgyro = zgyro;
	packet.xmag = xmag;
	packet.ymag = ymag;
	packet.zmag = zmag;
	packet.abs_pressure = abs_pressure;
	packet.diff_pressure = diff_pressure;
	packet.pressure_alt = pressure_alt;
	packet.gps_alt = gps_alt;
	packet.temperature = temperature;
	packet.fields_updated = fields_updated;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_HIL_SENSOR;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_SENSOR_LEN, MAVLINK_MSG_ID_HIL_SENSOR_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif
}

/**
 * @brief Pack a hil_sensor message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_usec Timestamp (microseconds, synced to UNIX time or since system boot)
 * @param roll Roll angle in inertial frame (rad)
 * @param pitch Pitch angle in inertial frame (rad)
 * @param yaw Yaw angle in inertial frame (rad)
 * @param lat Latitude, expressed as * 1E7 degrees
 * @param lon Longitude, expressed as * 1E7 degrees
 * @param xacc X acceleration (m/s^2)
 * @param yacc Y acceleration (m/s^2)
 * @param zacc Z acceleration (m/s^2)
 * @param xgyro Angular speed around X axis in body frame (rad / sec)
 * @param ygyro Angular speed around Y axis in body frame (rad / sec)
 * @param zgyro Angular speed around Z axis in body frame (rad / sec)
 * @param xmag X Magnetic field (Gauss)
 * @param ymag Y Magnetic field (Gauss)
 * @param zmag Z Magnetic field (Gauss)
 * @param abs_pressure Absolute pressure in millibar
 * @param diff_pressure Differential pressure (airspeed) in millibar
 * @param pressure_alt Altitude calculated from pressure
 * @param gps_alt GPS altitude (MSL) WGS84
 * @param temperature Temperature in degrees celsius
 * @param fields_updated Bitmask for fields that have updated since last message, bit 0 = xacc, bit 12: temperature
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_sensor_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint64_t time_usec,float roll,float pitch,float yaw,int32_t lat,int32_t lon,float xacc,float yacc,float zacc,float xgyro,float ygyro,float zgyro,float xmag,float ymag,float zmag,float abs_pressure,float diff_pressure,float pressure_alt,float gps_alt,float temperature,uint32_t fields_updated)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_HIL_SENSOR_LEN];
	_mav_put_uint64_t(buf, 0, time_usec);
	_mav_put_float(buf, 8, roll);
	_mav_put_float(buf, 12, pitch);
	_mav_put_float(buf, 16, yaw);
	_mav_put_int32_t(buf, 20, lat);
	_mav_put_int32_t(buf, 24, lon);
	_mav_put_float(buf, 28, xacc);
	_mav_put_float(buf, 32, yacc);
	_mav_put_float(buf, 36, zacc);
	_mav_put_float(buf, 40, xgyro);
	_mav_put_float(buf, 44, ygyro);
	_mav_put_float(buf, 48, zgyro);
	_mav_put_float(buf, 52, xmag);
	_mav_put_float(buf, 56, ymag);
	_mav_put_float(buf, 60, zmag);
	_mav_put_float(buf, 64, abs_pressure);
	_mav_put_float(buf, 68, diff_pressure);
	_mav_put_float(buf, 72, pressure_alt);
	_mav_put_float(buf, 76, gps_alt);
	_mav_put_float(buf, 80, temperature);
	_mav_put_uint32_t(buf, 84, fields_updated);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#else
	mavlink_hil_sensor_t packet;
	packet.time_usec = time_usec;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;
	packet.lat = lat;
	packet.lon = lon;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	packet.xgyro = xgyro;
	packet.ygyro = ygyro;
	packet.zgyro = zgyro;
	packet.xmag = xmag;
	packet.ymag = ymag;
	packet.zmag = zmag;
	packet.abs_pressure = abs_pressure;
	packet.diff_pressure = diff_pressure;
	packet.pressure_alt = pressure_alt;
	packet.gps_alt = gps_alt;
	packet.temperature = temperature;
	packet.fields_updated = fields_updated;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_HIL_SENSOR;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_SENSOR_LEN, MAVLINK_MSG_ID_HIL_SENSOR_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif
}

/**
 * @brief Encode a hil_sensor struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param hil_sensor C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_hil_sensor_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_hil_sensor_t* hil_sensor)
{
	return mavlink_msg_hil_sensor_pack(system_id, component_id, msg, hil_sensor->time_usec, hil_sensor->roll, hil_sensor->pitch, hil_sensor->yaw, hil_sensor->lat, hil_sensor->lon, hil_sensor->xacc, hil_sensor->yacc, hil_sensor->zacc, hil_sensor->xgyro, hil_sensor->ygyro, hil_sensor->zgyro, hil_sensor->xmag, hil_sensor->ymag, hil_sensor->zmag, hil_sensor->abs_pressure, hil_sensor->diff_pressure, hil_sensor->pressure_alt, hil_sensor->gps_alt, hil_sensor->temperature, hil_sensor->fields_updated);
}

/**
 * @brief Send a hil_sensor message
 * @param chan MAVLink channel to send the message
 *
 * @param time_usec Timestamp (microseconds, synced to UNIX time or since system boot)
 * @param roll Roll angle in inertial frame (rad)
 * @param pitch Pitch angle in inertial frame (rad)
 * @param yaw Yaw angle in inertial frame (rad)
 * @param lat Latitude, expressed as * 1E7 degrees
 * @param lon Longitude, expressed as * 1E7 degrees
 * @param xacc X acceleration (m/s^2)
 * @param yacc Y acceleration (m/s^2)
 * @param zacc Z acceleration (m/s^2)
 * @param xgyro Angular speed around X axis in body frame (rad / sec)
 * @param ygyro Angular speed around Y axis in body frame (rad / sec)
 * @param zgyro Angular speed around Z axis in body frame (rad / sec)
 * @param xmag X Magnetic field (Gauss)
 * @param ymag Y Magnetic field (Gauss)
 * @param zmag Z Magnetic field (Gauss)
 * @param abs_pressure Absolute pressure in millibar
 * @param diff_pressure Differential pressure (airspeed) in millibar
 * @param pressure_alt Altitude calculated from pressure
 * @param gps_alt GPS altitude (MSL) WGS84
 * @param temperature Temperature in degrees celsius
 * @param fields_updated Bitmask for fields that have updated since last message, bit 0 = xacc, bit 12: temperature
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_hil_sensor_send(mavlink_channel_t chan, uint64_t time_usec, float roll, float pitch, float yaw, int32_t lat, int32_t lon, float xacc, float yacc, float zacc, float xgyro, float ygyro, float zgyro, float xmag, float ymag, float zmag, float abs_pressure, float diff_pressure, float pressure_alt, float gps_alt, float temperature, uint32_t fields_updated)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_HIL_SENSOR_LEN];
	_mav_put_uint64_t(buf, 0, time_usec);
	_mav_put_float(buf, 8, roll);
	_mav_put_float(buf, 12, pitch);
	_mav_put_float(buf, 16, yaw);
	_mav_put_int32_t(buf, 20, lat);
	_mav_put_int32_t(buf, 24, lon);
	_mav_put_float(buf, 28, xacc);
	_mav_put_float(buf, 32, yacc);
	_mav_put_float(buf, 36, zacc);
	_mav_put_float(buf, 40, xgyro);
	_mav_put_float(buf, 44, ygyro);
	_mav_put_float(buf, 48, zgyro);
	_mav_put_float(buf, 52, xmag);
	_mav_put_float(buf, 56, ymag);
	_mav_put_float(buf, 60, zmag);
	_mav_put_float(buf, 64, abs_pressure);
	_mav_put_float(buf, 68, diff_pressure);
	_mav_put_float(buf, 72, pressure_alt);
	_mav_put_float(buf, 76, gps_alt);
	_mav_put_float(buf, 80, temperature);
	_mav_put_uint32_t(buf, 84, fields_updated);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_SENSOR, buf, MAVLINK_MSG_ID_HIL_SENSOR_LEN, MAVLINK_MSG_ID_HIL_SENSOR_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_SENSOR, buf, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif
#else
	mavlink_hil_sensor_t packet;
	packet.time_usec = time_usec;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;
	packet.lat = lat;
	packet.lon = lon;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	packet.xgyro = xgyro;
	packet.ygyro = ygyro;
	packet.zgyro = zgyro;
	packet.xmag = xmag;
	packet.ymag = ymag;
	packet.zmag = zmag;
	packet.abs_pressure = abs_pressure;
	packet.diff_pressure = diff_pressure;
	packet.pressure_alt = pressure_alt;
	packet.gps_alt = gps_alt;
	packet.temperature = temperature;
	packet.fields_updated = fields_updated;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_SENSOR, (const char *)&packet, MAVLINK_MSG_ID_HIL_SENSOR_LEN, MAVLINK_MSG_ID_HIL_SENSOR_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_SENSOR, (const char *)&packet, MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif
#endif
}

#endif

// MESSAGE HIL_SENSOR UNPACKING


/**
 * @brief Get field time_usec from hil_sensor message
 *
 * @return Timestamp (microseconds, synced to UNIX time or since system boot)
 */
static inline uint64_t mavlink_msg_hil_sensor_get_time_usec(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field roll from hil_sensor message
 *
 * @return Roll angle in inertial frame (rad)
 */
static inline float mavlink_msg_hil_sensor_get_roll(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field pitch from hil_sensor message
 *
 * @return Pitch angle in inertial frame (rad)
 */
static inline float mavlink_msg_hil_sensor_get_pitch(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field yaw from hil_sensor message
 *
 * @return Yaw angle in inertial frame (rad)
 */
static inline float mavlink_msg_hil_sensor_get_yaw(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field lat from hil_sensor message
 *
 * @return Latitude, expressed as * 1E7 degrees
 */
static inline int32_t mavlink_msg_hil_sensor_get_lat(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int32_t(msg,  20);
}

/**
 * @brief Get field lon from hil_sensor message
 *
 * @return Longitude, expressed as * 1E7 degrees
 */
static inline int32_t mavlink_msg_hil_sensor_get_lon(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int32_t(msg,  24);
}

/**
 * @brief Get field xacc from hil_sensor message
 *
 * @return X acceleration (m/s^2)
 */
static inline float mavlink_msg_hil_sensor_get_xacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Get field yacc from hil_sensor message
 *
 * @return Y acceleration (m/s^2)
 */
static inline float mavlink_msg_hil_sensor_get_yacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field zacc from hil_sensor message
 *
 * @return Z acceleration (m/s^2)
 */
static inline float mavlink_msg_hil_sensor_get_zacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  36);
}

/**
 * @brief Get field xgyro from hil_sensor message
 *
 * @return Angular speed around X axis in body frame (rad / sec)
 */
static inline float mavlink_msg_hil_sensor_get_xgyro(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  40);
}

/**
 * @brief Get field ygyro from hil_sensor message
 *
 * @return Angular speed around Y axis in body frame (rad / sec)
 */
static inline float mavlink_msg_hil_sensor_get_ygyro(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  44);
}

/**
 * @brief Get field zgyro from hil_sensor message
 *
 * @return Angular speed around Z axis in body frame (rad / sec)
 */
static inline float mavlink_msg_hil_sensor_get_zgyro(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  48);
}

/**
 * @brief Get field xmag from hil_sensor message
 *
 * @return X Magnetic field (Gauss)
 */
static inline float mavlink_msg_hil_sensor_get_xmag(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  52);
}

/**
 * @brief Get field ymag from hil_sensor message
 *
 * @return Y Magnetic field (Gauss)
 */
static inline float mavlink_msg_hil_sensor_get_ymag(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  56);
}

/**
 * @brief Get field zmag from hil_sensor message
 *
 * @return Z Magnetic field (Gauss)
 */
static inline float mavlink_msg_hil_sensor_get_zmag(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  60);
}

/**
 * @brief Get field abs_pressure from hil_sensor message
 *
 * @return Absolute pressure in millibar
 */
static inline float mavlink_msg_hil_sensor_get_abs_pressure(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  64);
}

/**
 * @brief Get field diff_pressure from hil_sensor message
 *
 * @return Differential pressure (airspeed) in millibar
 */
static inline float mavlink_msg_hil_sensor_get_diff_pressure(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  68);
}

/**
 * @brief Get field pressure_alt from hil_sensor message
 *
 * @return Altitude calculated from pressure
 */
static inline float mavlink_msg_hil_sensor_get_pressure_alt(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  72);
}

/**
 * @brief Get field gps_alt from hil_sensor message
 *
 * @return GPS altitude (MSL) WGS84
 */
static inline float mavlink_msg_hil_sensor_get_gps_alt(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  76);
}

/**
 * @brief Get field temperature from hil_sensor message
 *
 * @return Temperature in degrees celsius
 */
static inline float mavlink_msg_hil_sensor_get_temperature(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  80);
}

/**
 * @brief Get field fields_updated from hil_sensor message
 *
 * @return Bitmask for fields that have updated since last message, bit 0 = xacc, bit 12: temperature
 */
static inline uint32_t mavlink_msg_hil_sensor_get_fields_updated(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint32_t(msg,  84);
}

/**
 * @brief Decode a hil_sensor message into a struct
 *
 * @param msg The message to decode
 * @param hil_sensor C-struct to decode the message contents into
 */
static inline void mavlink_msg_hil_sensor_decode(const mavlink_message_t* msg, mavlink_hil_sensor_t* hil_sensor)
{
#if MAVLINK_NEED_BYTE_SWAP
	hil_sensor->time_usec = mavlink_msg_hil_sensor_get_time_usec(msg);
	hil_sensor->roll = mavlink_msg_hil_sensor_get_roll(msg);
	hil_sensor->pitch = mavlink_msg_hil_sensor_get_pitch(msg);
	hil_sensor->yaw = mavlink_msg_hil_sensor_get_yaw(msg);
	hil_sensor->lat = mavlink_msg_hil_sensor_get_lat(msg);
	hil_sensor->lon = mavlink_msg_hil_sensor_get_lon(msg);
	hil_sensor->xacc = mavlink_msg_hil_sensor_get_xacc(msg);
	hil_sensor->yacc = mavlink_msg_hil_sensor_get_yacc(msg);
	hil_sensor->zacc = mavlink_msg_hil_sensor_get_zacc(msg);
	hil_sensor->xgyro = mavlink_msg_hil_sensor_get_xgyro(msg);
	hil_sensor->ygyro = mavlink_msg_hil_sensor_get_ygyro(msg);
	hil_sensor->zgyro = mavlink_msg_hil_sensor_get_zgyro(msg);
	hil_sensor->xmag = mavlink_msg_hil_sensor_get_xmag(msg);
	hil_sensor->ymag = mavlink_msg_hil_sensor_get_ymag(msg);
	hil_sensor->zmag = mavlink_msg_hil_sensor_get_zmag(msg);
	hil_sensor->abs_pressure = mavlink_msg_hil_sensor_get_abs_pressure(msg);
	hil_sensor->diff_pressure = mavlink_msg_hil_sensor_get_diff_pressure(msg);
	hil_sensor->pressure_alt = mavlink_msg_hil_sensor_get_pressure_alt(msg);
	hil_sensor->gps_alt = mavlink_msg_hil_sensor_get_gps_alt(msg);
	hil_sensor->temperature = mavlink_msg_hil_sensor_get_temperature(msg);
	hil_sensor->fields_updated = mavlink_msg_hil_sensor_get_fields_updated(msg);
#else
	memcpy(hil_sensor, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_HIL_SENSOR_LEN);
#endif
}
