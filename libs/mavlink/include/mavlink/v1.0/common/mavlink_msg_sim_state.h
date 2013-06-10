// MESSAGE SIM_STATE PACKING

#define MAVLINK_MSG_ID_SIM_STATE 108

typedef struct __mavlink_sim_state_t
{
 float roll; ///< Roll angle (rad)
 float pitch; ///< Pitch angle (rad)
 float yaw; ///< Yaw angle (rad)
 float xacc; ///< X acceleration m/s/s
 float yacc; ///< Y acceleration m/s/s
 float zacc; ///< Z acceleration m/s/s
 float xgyro; ///< Angular speed around X axis rad/s
 float ygyro; ///< Angular speed around Y axis rad/s
 float zgyro; ///< Angular speed around Z axis rad/s
 float lat; ///< Latitude in degrees
 float lng; ///< Longitude in degrees
} mavlink_sim_state_t;

#define MAVLINK_MSG_ID_SIM_STATE_LEN 44
#define MAVLINK_MSG_ID_108_LEN 44

#define MAVLINK_MSG_ID_SIM_STATE_CRC 212
#define MAVLINK_MSG_ID_108_CRC 212



#define MAVLINK_MESSAGE_INFO_SIM_STATE { \
	"SIM_STATE", \
	11, \
	{  { "roll", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_sim_state_t, roll) }, \
         { "pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_sim_state_t, pitch) }, \
         { "yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_sim_state_t, yaw) }, \
         { "xacc", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_sim_state_t, xacc) }, \
         { "yacc", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_sim_state_t, yacc) }, \
         { "zacc", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_sim_state_t, zacc) }, \
         { "xgyro", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_sim_state_t, xgyro) }, \
         { "ygyro", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_sim_state_t, ygyro) }, \
         { "zgyro", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_sim_state_t, zgyro) }, \
         { "lat", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_sim_state_t, lat) }, \
         { "lng", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_sim_state_t, lng) }, \
         } \
}


/**
 * @brief Pack a sim_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param xacc X acceleration m/s/s
 * @param yacc Y acceleration m/s/s
 * @param zacc Z acceleration m/s/s
 * @param xgyro Angular speed around X axis rad/s
 * @param ygyro Angular speed around Y axis rad/s
 * @param zgyro Angular speed around Z axis rad/s
 * @param lat Latitude in degrees
 * @param lng Longitude in degrees
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sim_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       float roll, float pitch, float yaw, float xacc, float yacc, float zacc, float xgyro, float ygyro, float zgyro, float lat, float lng)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SIM_STATE_LEN];
	_mav_put_float(buf, 0, roll);
	_mav_put_float(buf, 4, pitch);
	_mav_put_float(buf, 8, yaw);
	_mav_put_float(buf, 12, xacc);
	_mav_put_float(buf, 16, yacc);
	_mav_put_float(buf, 20, zacc);
	_mav_put_float(buf, 24, xgyro);
	_mav_put_float(buf, 28, ygyro);
	_mav_put_float(buf, 32, zgyro);
	_mav_put_float(buf, 36, lat);
	_mav_put_float(buf, 40, lng);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SIM_STATE_LEN);
#else
	mavlink_sim_state_t packet;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	packet.xgyro = xgyro;
	packet.ygyro = ygyro;
	packet.zgyro = zgyro;
	packet.lat = lat;
	packet.lng = lng;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_SIM_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SIM_STATE_LEN, MAVLINK_MSG_ID_SIM_STATE_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif
}

/**
 * @brief Pack a sim_state message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param xacc X acceleration m/s/s
 * @param yacc Y acceleration m/s/s
 * @param zacc Z acceleration m/s/s
 * @param xgyro Angular speed around X axis rad/s
 * @param ygyro Angular speed around Y axis rad/s
 * @param zgyro Angular speed around Z axis rad/s
 * @param lat Latitude in degrees
 * @param lng Longitude in degrees
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sim_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           float roll,float pitch,float yaw,float xacc,float yacc,float zacc,float xgyro,float ygyro,float zgyro,float lat,float lng)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SIM_STATE_LEN];
	_mav_put_float(buf, 0, roll);
	_mav_put_float(buf, 4, pitch);
	_mav_put_float(buf, 8, yaw);
	_mav_put_float(buf, 12, xacc);
	_mav_put_float(buf, 16, yacc);
	_mav_put_float(buf, 20, zacc);
	_mav_put_float(buf, 24, xgyro);
	_mav_put_float(buf, 28, ygyro);
	_mav_put_float(buf, 32, zgyro);
	_mav_put_float(buf, 36, lat);
	_mav_put_float(buf, 40, lng);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SIM_STATE_LEN);
#else
	mavlink_sim_state_t packet;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	packet.xgyro = xgyro;
	packet.ygyro = ygyro;
	packet.zgyro = zgyro;
	packet.lat = lat;
	packet.lng = lng;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_SIM_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SIM_STATE_LEN, MAVLINK_MSG_ID_SIM_STATE_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif
}

/**
 * @brief Encode a sim_state struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sim_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sim_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sim_state_t* sim_state)
{
	return mavlink_msg_sim_state_pack(system_id, component_id, msg, sim_state->roll, sim_state->pitch, sim_state->yaw, sim_state->xacc, sim_state->yacc, sim_state->zacc, sim_state->xgyro, sim_state->ygyro, sim_state->zgyro, sim_state->lat, sim_state->lng);
}

/**
 * @brief Send a sim_state message
 * @param chan MAVLink channel to send the message
 *
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param xacc X acceleration m/s/s
 * @param yacc Y acceleration m/s/s
 * @param zacc Z acceleration m/s/s
 * @param xgyro Angular speed around X axis rad/s
 * @param ygyro Angular speed around Y axis rad/s
 * @param zgyro Angular speed around Z axis rad/s
 * @param lat Latitude in degrees
 * @param lng Longitude in degrees
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sim_state_send(mavlink_channel_t chan, float roll, float pitch, float yaw, float xacc, float yacc, float zacc, float xgyro, float ygyro, float zgyro, float lat, float lng)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SIM_STATE_LEN];
	_mav_put_float(buf, 0, roll);
	_mav_put_float(buf, 4, pitch);
	_mav_put_float(buf, 8, yaw);
	_mav_put_float(buf, 12, xacc);
	_mav_put_float(buf, 16, yacc);
	_mav_put_float(buf, 20, zacc);
	_mav_put_float(buf, 24, xgyro);
	_mav_put_float(buf, 28, ygyro);
	_mav_put_float(buf, 32, zgyro);
	_mav_put_float(buf, 36, lat);
	_mav_put_float(buf, 40, lng);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SIM_STATE, buf, MAVLINK_MSG_ID_SIM_STATE_LEN, MAVLINK_MSG_ID_SIM_STATE_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SIM_STATE, buf, MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif
#else
	mavlink_sim_state_t packet;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;
	packet.xacc = xacc;
	packet.yacc = yacc;
	packet.zacc = zacc;
	packet.xgyro = xgyro;
	packet.ygyro = ygyro;
	packet.zgyro = zgyro;
	packet.lat = lat;
	packet.lng = lng;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SIM_STATE, (const char *)&packet, MAVLINK_MSG_ID_SIM_STATE_LEN, MAVLINK_MSG_ID_SIM_STATE_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SIM_STATE, (const char *)&packet, MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif
#endif
}

#endif

// MESSAGE SIM_STATE UNPACKING


/**
 * @brief Get field roll from sim_state message
 *
 * @return Roll angle (rad)
 */
static inline float mavlink_msg_sim_state_get_roll(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field pitch from sim_state message
 *
 * @return Pitch angle (rad)
 */
static inline float mavlink_msg_sim_state_get_pitch(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field yaw from sim_state message
 *
 * @return Yaw angle (rad)
 */
static inline float mavlink_msg_sim_state_get_yaw(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field xacc from sim_state message
 *
 * @return X acceleration m/s/s
 */
static inline float mavlink_msg_sim_state_get_xacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field yacc from sim_state message
 *
 * @return Y acceleration m/s/s
 */
static inline float mavlink_msg_sim_state_get_yacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field zacc from sim_state message
 *
 * @return Z acceleration m/s/s
 */
static inline float mavlink_msg_sim_state_get_zacc(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  20);
}

/**
 * @brief Get field xgyro from sim_state message
 *
 * @return Angular speed around X axis rad/s
 */
static inline float mavlink_msg_sim_state_get_xgyro(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Get field ygyro from sim_state message
 *
 * @return Angular speed around Y axis rad/s
 */
static inline float mavlink_msg_sim_state_get_ygyro(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Get field zgyro from sim_state message
 *
 * @return Angular speed around Z axis rad/s
 */
static inline float mavlink_msg_sim_state_get_zgyro(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field lat from sim_state message
 *
 * @return Latitude in degrees
 */
static inline float mavlink_msg_sim_state_get_lat(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  36);
}

/**
 * @brief Get field lng from sim_state message
 *
 * @return Longitude in degrees
 */
static inline float mavlink_msg_sim_state_get_lng(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  40);
}

/**
 * @brief Decode a sim_state message into a struct
 *
 * @param msg The message to decode
 * @param sim_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_sim_state_decode(const mavlink_message_t* msg, mavlink_sim_state_t* sim_state)
{
#if MAVLINK_NEED_BYTE_SWAP
	sim_state->roll = mavlink_msg_sim_state_get_roll(msg);
	sim_state->pitch = mavlink_msg_sim_state_get_pitch(msg);
	sim_state->yaw = mavlink_msg_sim_state_get_yaw(msg);
	sim_state->xacc = mavlink_msg_sim_state_get_xacc(msg);
	sim_state->yacc = mavlink_msg_sim_state_get_yacc(msg);
	sim_state->zacc = mavlink_msg_sim_state_get_zacc(msg);
	sim_state->xgyro = mavlink_msg_sim_state_get_xgyro(msg);
	sim_state->ygyro = mavlink_msg_sim_state_get_ygyro(msg);
	sim_state->zgyro = mavlink_msg_sim_state_get_zgyro(msg);
	sim_state->lat = mavlink_msg_sim_state_get_lat(msg);
	sim_state->lng = mavlink_msg_sim_state_get_lng(msg);
#else
	memcpy(sim_state, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_SIM_STATE_LEN);
#endif
}
