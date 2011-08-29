// MESSAGE GLOBAL_POSITION_INT PACKING

#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT 73

typedef struct __mavlink_global_position_int_t
{
 int32_t lat; ///< Latitude, expressed as * 1E7
 int32_t lon; ///< Longitude, expressed as * 1E7
 int32_t alt; ///< Altitude in meters, expressed as * 1000 (millimeters), above MSL
 int16_t vx; ///< Ground X Speed (Latitude), expressed as m/s * 100
 int16_t vy; ///< Ground Y Speed (Longitude), expressed as m/s * 100
 int16_t vz; ///< Ground Z Speed (Altitude), expressed as m/s * 100
 uint16_t hdg; ///< Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
} mavlink_global_position_int_t;

#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN 20
#define MAVLINK_MSG_ID_73_LEN 20



#define MAVLINK_MESSAGE_INFO_GLOBAL_POSITION_INT { \
	"GLOBAL_POSITION_INT", \
	7, \
	{  { "lat", MAVLINK_TYPE_INT32_T, 0, 0, offsetof(mavlink_global_position_int_t, lat) }, \
         { "lon", MAVLINK_TYPE_INT32_T, 0, 4, offsetof(mavlink_global_position_int_t, lon) }, \
         { "alt", MAVLINK_TYPE_INT32_T, 0, 8, offsetof(mavlink_global_position_int_t, alt) }, \
         { "vx", MAVLINK_TYPE_INT16_T, 0, 12, offsetof(mavlink_global_position_int_t, vx) }, \
         { "vy", MAVLINK_TYPE_INT16_T, 0, 14, offsetof(mavlink_global_position_int_t, vy) }, \
         { "vz", MAVLINK_TYPE_INT16_T, 0, 16, offsetof(mavlink_global_position_int_t, vz) }, \
         { "hdg", MAVLINK_TYPE_UINT16_T, 0, 18, offsetof(mavlink_global_position_int_t, hdg) }, \
         } \
}


/**
 * @brief Pack a global_position_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters), above MSL
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_int_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)
{
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;

	put_int32_t_by_index(msg, 0, lat); // Latitude, expressed as * 1E7
	put_int32_t_by_index(msg, 4, lon); // Longitude, expressed as * 1E7
	put_int32_t_by_index(msg, 8, alt); // Altitude in meters, expressed as * 1000 (millimeters), above MSL
	put_int16_t_by_index(msg, 12, vx); // Ground X Speed (Latitude), expressed as m/s * 100
	put_int16_t_by_index(msg, 14, vy); // Ground Y Speed (Longitude), expressed as m/s * 100
	put_int16_t_by_index(msg, 16, vz); // Ground Z Speed (Altitude), expressed as m/s * 100
	put_uint16_t_by_index(msg, 18, hdg); // Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

	return mavlink_finalize_message(msg, system_id, component_id, 20, 241);
}

/**
 * @brief Pack a global_position_int message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters), above MSL
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_int_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           int32_t lat,int32_t lon,int32_t alt,int16_t vx,int16_t vy,int16_t vz,uint16_t hdg)
{
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;

	put_int32_t_by_index(msg, 0, lat); // Latitude, expressed as * 1E7
	put_int32_t_by_index(msg, 4, lon); // Longitude, expressed as * 1E7
	put_int32_t_by_index(msg, 8, alt); // Altitude in meters, expressed as * 1000 (millimeters), above MSL
	put_int16_t_by_index(msg, 12, vx); // Ground X Speed (Latitude), expressed as m/s * 100
	put_int16_t_by_index(msg, 14, vy); // Ground Y Speed (Longitude), expressed as m/s * 100
	put_int16_t_by_index(msg, 16, vz); // Ground Z Speed (Altitude), expressed as m/s * 100
	put_uint16_t_by_index(msg, 18, hdg); // Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 20, 241);
}

/**
 * @brief Encode a global_position_int struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param global_position_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_int_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_global_position_int_t* global_position_int)
{
	return mavlink_msg_global_position_int_pack(system_id, component_id, msg, global_position_int->lat, global_position_int->lon, global_position_int->alt, global_position_int->vx, global_position_int->vy, global_position_int->vz, global_position_int->hdg);
}

/**
 * @brief Send a global_position_int message
 * @param chan MAVLink channel to send the message
 *
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters), above MSL
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_global_position_int_send(mavlink_channel_t chan, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 20);
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;

	put_int32_t_by_index(msg, 0, lat); // Latitude, expressed as * 1E7
	put_int32_t_by_index(msg, 4, lon); // Longitude, expressed as * 1E7
	put_int32_t_by_index(msg, 8, alt); // Altitude in meters, expressed as * 1000 (millimeters), above MSL
	put_int16_t_by_index(msg, 12, vx); // Ground X Speed (Latitude), expressed as m/s * 100
	put_int16_t_by_index(msg, 14, vy); // Ground Y Speed (Longitude), expressed as m/s * 100
	put_int16_t_by_index(msg, 16, vz); // Ground Z Speed (Altitude), expressed as m/s * 100
	put_uint16_t_by_index(msg, 18, hdg); // Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

	mavlink_finalize_message_chan_send(msg, chan, 20, 241);
}

#endif

// MESSAGE GLOBAL_POSITION_INT UNPACKING


/**
 * @brief Get field lat from global_position_int message
 *
 * @return Latitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_global_position_int_get_lat(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int32_t(msg,  0);
}

/**
 * @brief Get field lon from global_position_int message
 *
 * @return Longitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_global_position_int_get_lon(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int32_t(msg,  4);
}

/**
 * @brief Get field alt from global_position_int message
 *
 * @return Altitude in meters, expressed as * 1000 (millimeters), above MSL
 */
static inline int32_t mavlink_msg_global_position_int_get_alt(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int32_t(msg,  8);
}

/**
 * @brief Get field vx from global_position_int message
 *
 * @return Ground X Speed (Latitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_global_position_int_get_vx(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  12);
}

/**
 * @brief Get field vy from global_position_int message
 *
 * @return Ground Y Speed (Longitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_global_position_int_get_vy(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  14);
}

/**
 * @brief Get field vz from global_position_int message
 *
 * @return Ground Z Speed (Altitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_global_position_int_get_vz(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  16);
}

/**
 * @brief Get field hdg from global_position_int message
 *
 * @return Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 */
static inline uint16_t mavlink_msg_global_position_int_get_hdg(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  18);
}

/**
 * @brief Decode a global_position_int message into a struct
 *
 * @param msg The message to decode
 * @param global_position_int C-struct to decode the message contents into
 */
static inline void mavlink_msg_global_position_int_decode(const mavlink_message_t* msg, mavlink_global_position_int_t* global_position_int)
{
#if MAVLINK_NEED_BYTE_SWAP
	global_position_int->lat = mavlink_msg_global_position_int_get_lat(msg);
	global_position_int->lon = mavlink_msg_global_position_int_get_lon(msg);
	global_position_int->alt = mavlink_msg_global_position_int_get_alt(msg);
	global_position_int->vx = mavlink_msg_global_position_int_get_vx(msg);
	global_position_int->vy = mavlink_msg_global_position_int_get_vy(msg);
	global_position_int->vz = mavlink_msg_global_position_int_get_vz(msg);
	global_position_int->hdg = mavlink_msg_global_position_int_get_hdg(msg);
#else
	memcpy(global_position_int, MAVLINK_PAYLOAD(msg), 20);
#endif
}
