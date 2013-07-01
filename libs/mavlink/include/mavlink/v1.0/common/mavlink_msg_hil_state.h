// MESSAGE HIL_STATE PACKING

#define MAVLINK_MSG_ID_HIL_STATE 90

typedef struct __mavlink_hil_state_t
{
 uint64_t time_usec; ///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 float attitude_quaternion[4]; ///< Vehicle attitude expressed as normalized quaternion
 float rollspeed; ///< Body frame roll / phi angular speed (rad/s)
 float pitchspeed; ///< Body frame pitch / theta angular speed (rad/s)
 float yawspeed; ///< Body frame yaw / psi angular speed (rad/s)
 int32_t lat; ///< Latitude, expressed as * 1E7
 int32_t lon; ///< Longitude, expressed as * 1E7
 int32_t alt; ///< Altitude in meters, expressed as * 1000 (millimeters)
 int16_t vx; ///< Ground X Speed (Latitude), expressed as m/s * 100
 int16_t vy; ///< Ground Y Speed (Longitude), expressed as m/s * 100
 int16_t vz; ///< Ground Z Speed (Altitude), expressed as m/s * 100
 uint16_t ind_airspeed; ///< Indicated airspeed, expressed as m/s * 100
 uint16_t true_airspeed; ///< True airspeed, expressed as m/s * 100
 int16_t xacc; ///< X acceleration (mg)
 int16_t yacc; ///< Y acceleration (mg)
 int16_t zacc; ///< Z acceleration (mg)
} mavlink_hil_state_t;

#define MAVLINK_MSG_ID_HIL_STATE_LEN 64
#define MAVLINK_MSG_ID_90_LEN 64

#define MAVLINK_MSG_ID_HIL_STATE_CRC 162
#define MAVLINK_MSG_ID_90_CRC 162

#define MAVLINK_MSG_HIL_STATE_FIELD_ATTITUDE_QUATERNION_LEN 4

#define MAVLINK_MESSAGE_INFO_HIL_STATE { \
	"HIL_STATE", \
	16, \
	{  { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_hil_state_t, time_usec) }, \
         { "attitude_quaternion", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_hil_state_t, attitude_quaternion) }, \
         { "rollspeed", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_hil_state_t, rollspeed) }, \
         { "pitchspeed", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_hil_state_t, pitchspeed) }, \
         { "yawspeed", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_hil_state_t, yawspeed) }, \
         { "lat", NULL, MAVLINK_TYPE_INT32_T, 0, 36, offsetof(mavlink_hil_state_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_INT32_T, 0, 40, offsetof(mavlink_hil_state_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_INT32_T, 0, 44, offsetof(mavlink_hil_state_t, alt) }, \
         { "vx", NULL, MAVLINK_TYPE_INT16_T, 0, 48, offsetof(mavlink_hil_state_t, vx) }, \
         { "vy", NULL, MAVLINK_TYPE_INT16_T, 0, 50, offsetof(mavlink_hil_state_t, vy) }, \
         { "vz", NULL, MAVLINK_TYPE_INT16_T, 0, 52, offsetof(mavlink_hil_state_t, vz) }, \
         { "ind_airspeed", NULL, MAVLINK_TYPE_UINT16_T, 0, 54, offsetof(mavlink_hil_state_t, ind_airspeed) }, \
         { "true_airspeed", NULL, MAVLINK_TYPE_UINT16_T, 0, 56, offsetof(mavlink_hil_state_t, true_airspeed) }, \
         { "xacc", NULL, MAVLINK_TYPE_INT16_T, 0, 58, offsetof(mavlink_hil_state_t, xacc) }, \
         { "yacc", NULL, MAVLINK_TYPE_INT16_T, 0, 60, offsetof(mavlink_hil_state_t, yacc) }, \
         { "zacc", NULL, MAVLINK_TYPE_INT16_T, 0, 62, offsetof(mavlink_hil_state_t, zacc) }, \
         } \
}


/**
 * @brief Pack a hil_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param attitude_quaternion Vehicle attitude expressed as normalized quaternion
 * @param rollspeed Body frame roll / phi angular speed (rad/s)
 * @param pitchspeed Body frame pitch / theta angular speed (rad/s)
 * @param yawspeed Body frame yaw / psi angular speed (rad/s)
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters)
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param ind_airspeed Indicated airspeed, expressed as m/s * 100
 * @param true_airspeed True airspeed, expressed as m/s * 100
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint64_t time_usec, const float *attitude_quaternion, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t ind_airspeed, uint16_t true_airspeed, int16_t xacc, int16_t yacc, int16_t zacc)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_HIL_STATE_LEN];
	_mav_put_uint64_t(buf, 0, time_usec);
	_mav_put_float(buf, 24, rollspeed);
	_mav_put_float(buf, 28, pitchspeed);
	_mav_put_float(buf, 32, yawspeed);
	_mav_put_int32_t(buf, 36, lat);
	_mav_put_int32_t(buf, 40, lon);
	_mav_put_int32_t(buf, 44, alt);
	_mav_put_int16_t(buf, 48, vx);
	_mav_put_int16_t(buf, 50, vy);
	_mav_put_int16_t(buf, 52, vz);
	_mav_put_uint16_t(buf, 54, ind_airspeed);
	_mav_put_uint16_t(buf, 56, true_airspeed);
	_mav_put_int16_t(buf, 58, xacc);
	_mav_put_int16_t(buf, 60, yacc);
	_mav_put_int16_t(buf, 62, zacc);
	_mav_put_float_array(buf, 8, attitude_quaternion, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_HIL_STATE_LEN);
#else
	mavlink_hil_state_t packet;
	packet.time_usec = time_usec;
	packet.rollspeed = rollspeed;
	packet.pitchspeed = pitchspeed;
	packet.yawspeed = yawspeed;
	packet.lat = lat;
	packet.lon = lon;
	packet.alt = alt;
	packet.vx = vx;
	packet.vy = vy;
	packet.vz = vz;
	packet.ind_airspeed = ind_airspeed;
	packet.true_airspeed = true_airspeed;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	mav_array_memcpy(packet.attitude_quaternion, attitude_quaternion, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_HIL_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_STATE_LEN, MAVLINK_MSG_ID_HIL_STATE_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif
}

/**
 * @brief Pack a hil_state message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param attitude_quaternion Vehicle attitude expressed as normalized quaternion
 * @param rollspeed Body frame roll / phi angular speed (rad/s)
 * @param pitchspeed Body frame pitch / theta angular speed (rad/s)
 * @param yawspeed Body frame yaw / psi angular speed (rad/s)
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters)
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param ind_airspeed Indicated airspeed, expressed as m/s * 100
 * @param true_airspeed True airspeed, expressed as m/s * 100
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint64_t time_usec,const float *attitude_quaternion,float rollspeed,float pitchspeed,float yawspeed,int32_t lat,int32_t lon,int32_t alt,int16_t vx,int16_t vy,int16_t vz,uint16_t ind_airspeed,uint16_t true_airspeed,int16_t xacc,int16_t yacc,int16_t zacc)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_HIL_STATE_LEN];
	_mav_put_uint64_t(buf, 0, time_usec);
	_mav_put_float(buf, 24, rollspeed);
	_mav_put_float(buf, 28, pitchspeed);
	_mav_put_float(buf, 32, yawspeed);
	_mav_put_int32_t(buf, 36, lat);
	_mav_put_int32_t(buf, 40, lon);
	_mav_put_int32_t(buf, 44, alt);
	_mav_put_int16_t(buf, 48, vx);
	_mav_put_int16_t(buf, 50, vy);
	_mav_put_int16_t(buf, 52, vz);
	_mav_put_uint16_t(buf, 54, ind_airspeed);
	_mav_put_uint16_t(buf, 56, true_airspeed);
	_mav_put_int16_t(buf, 58, xacc);
	_mav_put_int16_t(buf, 60, yacc);
	_mav_put_int16_t(buf, 62, zacc);
	_mav_put_float_array(buf, 8, attitude_quaternion, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_HIL_STATE_LEN);
#else
	mavlink_hil_state_t packet;
	packet.time_usec = time_usec;
	packet.rollspeed = rollspeed;
	packet.pitchspeed = pitchspeed;
	packet.yawspeed = yawspeed;
	packet.lat = lat;
	packet.lon = lon;
	packet.alt = alt;
	packet.vx = vx;
	packet.vy = vy;
	packet.vz = vz;
	packet.ind_airspeed = ind_airspeed;
	packet.true_airspeed = true_airspeed;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	mav_array_memcpy(packet.attitude_quaternion, attitude_quaternion, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_HIL_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_STATE_LEN, MAVLINK_MSG_ID_HIL_STATE_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif
}

/**
 * @brief Encode a hil_state struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param hil_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_hil_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_hil_state_t* hil_state)
{
	return mavlink_msg_hil_state_pack(system_id, component_id, msg, hil_state->time_usec, hil_state->attitude_quaternion, hil_state->rollspeed, hil_state->pitchspeed, hil_state->yawspeed, hil_state->lat, hil_state->lon, hil_state->alt, hil_state->vx, hil_state->vy, hil_state->vz, hil_state->ind_airspeed, hil_state->true_airspeed, hil_state->xacc, hil_state->yacc, hil_state->zacc);
}

/**
 * @brief Send a hil_state message
 * @param chan MAVLink channel to send the message
 *
 * @param time_usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param attitude_quaternion Vehicle attitude expressed as normalized quaternion
 * @param rollspeed Body frame roll / phi angular speed (rad/s)
 * @param pitchspeed Body frame pitch / theta angular speed (rad/s)
 * @param yawspeed Body frame yaw / psi angular speed (rad/s)
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters)
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param ind_airspeed Indicated airspeed, expressed as m/s * 100
 * @param true_airspeed True airspeed, expressed as m/s * 100
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_hil_state_send(mavlink_channel_t chan, uint64_t time_usec, const float *attitude_quaternion, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t ind_airspeed, uint16_t true_airspeed, int16_t xacc, int16_t yacc, int16_t zacc)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_HIL_STATE_LEN];
	_mav_put_uint64_t(buf, 0, time_usec);
	_mav_put_float(buf, 24, rollspeed);
	_mav_put_float(buf, 28, pitchspeed);
	_mav_put_float(buf, 32, yawspeed);
	_mav_put_int32_t(buf, 36, lat);
	_mav_put_int32_t(buf, 40, lon);
	_mav_put_int32_t(buf, 44, alt);
	_mav_put_int16_t(buf, 48, vx);
	_mav_put_int16_t(buf, 50, vy);
	_mav_put_int16_t(buf, 52, vz);
	_mav_put_uint16_t(buf, 54, ind_airspeed);
	_mav_put_uint16_t(buf, 56, true_airspeed);
	_mav_put_int16_t(buf, 58, xacc);
	_mav_put_int16_t(buf, 60, yacc);
	_mav_put_int16_t(buf, 62, zacc);
	_mav_put_float_array(buf, 8, attitude_quaternion, 4);
#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_STATE, buf, MAVLINK_MSG_ID_HIL_STATE_LEN, MAVLINK_MSG_ID_HIL_STATE_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_STATE, buf, MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif
#else
	mavlink_hil_state_t packet;
	packet.time_usec = time_usec;
	packet.rollspeed = rollspeed;
	packet.pitchspeed = pitchspeed;
	packet.yawspeed = yawspeed;
	packet.lat = lat;
	packet.lon = lon;
	packet.alt = alt;
	packet.vx = vx;
	packet.vy = vy;
	packet.vz = vz;
	packet.ind_airspeed = ind_airspeed;
	packet.true_airspeed = true_airspeed;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	mav_array_memcpy(packet.attitude_quaternion, attitude_quaternion, sizeof(float)*4);
#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_STATE, (const char *)&packet, MAVLINK_MSG_ID_HIL_STATE_LEN, MAVLINK_MSG_ID_HIL_STATE_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_HIL_STATE, (const char *)&packet, MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif
#endif
}

#endif

// MESSAGE HIL_STATE UNPACKING


/**
 * @brief Get field time_usec from hil_state message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_hil_state_get_time_usec(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field attitude_quaternion from hil_state message
 *
 * @return Vehicle attitude expressed as normalized quaternion
 */
static inline uint16_t mavlink_msg_hil_state_get_attitude_quaternion(const mavlink_message_t* msg, float *attitude_quaternion)
{
	return _MAV_RETURN_float_array(msg, attitude_quaternion, 4,  8);
}

/**
 * @brief Get field rollspeed from hil_state message
 *
 * @return Body frame roll / phi angular speed (rad/s)
 */
static inline float mavlink_msg_hil_state_get_rollspeed(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Get field pitchspeed from hil_state message
 *
 * @return Body frame pitch / theta angular speed (rad/s)
 */
static inline float mavlink_msg_hil_state_get_pitchspeed(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Get field yawspeed from hil_state message
 *
 * @return Body frame yaw / psi angular speed (rad/s)
 */
static inline float mavlink_msg_hil_state_get_yawspeed(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field lat from hil_state message
 *
 * @return Latitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_hil_state_get_lat(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int32_t(msg,  36);
}

/**
 * @brief Get field lon from hil_state message
 *
 * @return Longitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_hil_state_get_lon(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int32_t(msg,  40);
}

/**
 * @brief Get field alt from hil_state message
 *
 * @return Altitude in meters, expressed as * 1000 (millimeters)
 */
static inline int32_t mavlink_msg_hil_state_get_alt(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int32_t(msg,  44);
}

/**
 * @brief Get field vx from hil_state message
 *
 * @return Ground X Speed (Latitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_hil_state_get_vx(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int16_t(msg,  48);
}

/**
 * @brief Get field vy from hil_state message
 *
 * @return Ground Y Speed (Longitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_hil_state_get_vy(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int16_t(msg,  50);
}

/**
 * @brief Get field vz from hil_state message
 *
 * @return Ground Z Speed (Altitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_hil_state_get_vz(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int16_t(msg,  52);
}

/**
 * @brief Get field ind_airspeed from hil_state message
 *
 * @return Indicated airspeed, expressed as m/s * 100
 */
static inline uint16_t mavlink_msg_hil_state_get_ind_airspeed(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint16_t(msg,  54);
}

/**
 * @brief Get field true_airspeed from hil_state message
 *
 * @return True airspeed, expressed as m/s * 100
 */
static inline uint16_t mavlink_msg_hil_state_get_true_airspeed(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint16_t(msg,  56);
}

/**
 * @brief Get field xacc from hil_state message
 *
 * @return X acceleration (mg)
 */
static inline int16_t mavlink_msg_hil_state_get_xacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int16_t(msg,  58);
}

/**
 * @brief Get field yacc from hil_state message
 *
 * @return Y acceleration (mg)
 */
static inline int16_t mavlink_msg_hil_state_get_yacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int16_t(msg,  60);
}

/**
 * @brief Get field zacc from hil_state message
 *
 * @return Z acceleration (mg)
 */
static inline int16_t mavlink_msg_hil_state_get_zacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_int16_t(msg,  62);
}

/**
 * @brief Decode a hil_state message into a struct
 *
 * @param msg The message to decode
 * @param hil_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_hil_state_decode(const mavlink_message_t* msg, mavlink_hil_state_t* hil_state)
{
#if MAVLINK_NEED_BYTE_SWAP
	hil_state->time_usec = mavlink_msg_hil_state_get_time_usec(msg);
	mavlink_msg_hil_state_get_attitude_quaternion(msg, hil_state->attitude_quaternion);
	hil_state->rollspeed = mavlink_msg_hil_state_get_rollspeed(msg);
	hil_state->pitchspeed = mavlink_msg_hil_state_get_pitchspeed(msg);
	hil_state->yawspeed = mavlink_msg_hil_state_get_yawspeed(msg);
	hil_state->lat = mavlink_msg_hil_state_get_lat(msg);
	hil_state->lon = mavlink_msg_hil_state_get_lon(msg);
	hil_state->alt = mavlink_msg_hil_state_get_alt(msg);
	hil_state->vx = mavlink_msg_hil_state_get_vx(msg);
	hil_state->vy = mavlink_msg_hil_state_get_vy(msg);
	hil_state->vz = mavlink_msg_hil_state_get_vz(msg);
	hil_state->ind_airspeed = mavlink_msg_hil_state_get_ind_airspeed(msg);
	hil_state->true_airspeed = mavlink_msg_hil_state_get_true_airspeed(msg);
	hil_state->xacc = mavlink_msg_hil_state_get_xacc(msg);
	hil_state->yacc = mavlink_msg_hil_state_get_yacc(msg);
	hil_state->zacc = mavlink_msg_hil_state_get_zacc(msg);
#else
	memcpy(hil_state, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_HIL_STATE_LEN);
#endif
}
