// MESSAGE VFR_HUD PACKING

#define MAVLINK_MSG_ID_VFR_HUD 74

typedef struct __mavlink_vfr_hud_t
{
 float airspeed; ///< Current airspeed in m/s
 float groundspeed; ///< Current ground speed in m/s
 float alt; ///< Current altitude (MSL), in meters
 float climb; ///< Current climb rate in meters/second
 int16_t heading; ///< Current heading in degrees, in compass units (0..360, 0=north)
 uint16_t throttle; ///< Current throttle setting in integer percent, 0 to 100
} mavlink_vfr_hud_t;

#define MAVLINK_MSG_ID_VFR_HUD_LEN 20
#define MAVLINK_MSG_ID_74_LEN 20



#define MAVLINK_MESSAGE_INFO_VFR_HUD { \
	"VFR_HUD", \
	6, \
	{  { "airspeed", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_vfr_hud_t, airspeed) }, \
         { "groundspeed", MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_vfr_hud_t, groundspeed) }, \
         { "alt", MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_vfr_hud_t, alt) }, \
         { "climb", MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_vfr_hud_t, climb) }, \
         { "heading", MAVLINK_TYPE_INT16_T, 0, 16, offsetof(mavlink_vfr_hud_t, heading) }, \
         { "throttle", MAVLINK_TYPE_UINT16_T, 0, 18, offsetof(mavlink_vfr_hud_t, throttle) }, \
         } \
}


/**
 * @brief Pack a vfr_hud message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param airspeed Current airspeed in m/s
 * @param groundspeed Current ground speed in m/s
 * @param heading Current heading in degrees, in compass units (0..360, 0=north)
 * @param throttle Current throttle setting in integer percent, 0 to 100
 * @param alt Current altitude (MSL), in meters
 * @param climb Current climb rate in meters/second
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vfr_hud_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       float airspeed, float groundspeed, int16_t heading, uint16_t throttle, float alt, float climb)
{
	msg->msgid = MAVLINK_MSG_ID_VFR_HUD;

	put_float_by_index(msg, 0, airspeed); // Current airspeed in m/s
	put_float_by_index(msg, 4, groundspeed); // Current ground speed in m/s
	put_float_by_index(msg, 8, alt); // Current altitude (MSL), in meters
	put_float_by_index(msg, 12, climb); // Current climb rate in meters/second
	put_int16_t_by_index(msg, 16, heading); // Current heading in degrees, in compass units (0..360, 0=north)
	put_uint16_t_by_index(msg, 18, throttle); // Current throttle setting in integer percent, 0 to 100

	return mavlink_finalize_message(msg, system_id, component_id, 20, 20);
}

/**
 * @brief Pack a vfr_hud message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param airspeed Current airspeed in m/s
 * @param groundspeed Current ground speed in m/s
 * @param heading Current heading in degrees, in compass units (0..360, 0=north)
 * @param throttle Current throttle setting in integer percent, 0 to 100
 * @param alt Current altitude (MSL), in meters
 * @param climb Current climb rate in meters/second
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vfr_hud_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           float airspeed,float groundspeed,int16_t heading,uint16_t throttle,float alt,float climb)
{
	msg->msgid = MAVLINK_MSG_ID_VFR_HUD;

	put_float_by_index(msg, 0, airspeed); // Current airspeed in m/s
	put_float_by_index(msg, 4, groundspeed); // Current ground speed in m/s
	put_float_by_index(msg, 8, alt); // Current altitude (MSL), in meters
	put_float_by_index(msg, 12, climb); // Current climb rate in meters/second
	put_int16_t_by_index(msg, 16, heading); // Current heading in degrees, in compass units (0..360, 0=north)
	put_uint16_t_by_index(msg, 18, throttle); // Current throttle setting in integer percent, 0 to 100

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 20, 20);
}

/**
 * @brief Encode a vfr_hud struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param vfr_hud C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_vfr_hud_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_vfr_hud_t* vfr_hud)
{
	return mavlink_msg_vfr_hud_pack(system_id, component_id, msg, vfr_hud->airspeed, vfr_hud->groundspeed, vfr_hud->heading, vfr_hud->throttle, vfr_hud->alt, vfr_hud->climb);
}

/**
 * @brief Send a vfr_hud message
 * @param chan MAVLink channel to send the message
 *
 * @param airspeed Current airspeed in m/s
 * @param groundspeed Current ground speed in m/s
 * @param heading Current heading in degrees, in compass units (0..360, 0=north)
 * @param throttle Current throttle setting in integer percent, 0 to 100
 * @param alt Current altitude (MSL), in meters
 * @param climb Current climb rate in meters/second
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_vfr_hud_send(mavlink_channel_t chan, float airspeed, float groundspeed, int16_t heading, uint16_t throttle, float alt, float climb)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 20);
	msg->msgid = MAVLINK_MSG_ID_VFR_HUD;

	put_float_by_index(msg, 0, airspeed); // Current airspeed in m/s
	put_float_by_index(msg, 4, groundspeed); // Current ground speed in m/s
	put_float_by_index(msg, 8, alt); // Current altitude (MSL), in meters
	put_float_by_index(msg, 12, climb); // Current climb rate in meters/second
	put_int16_t_by_index(msg, 16, heading); // Current heading in degrees, in compass units (0..360, 0=north)
	put_uint16_t_by_index(msg, 18, throttle); // Current throttle setting in integer percent, 0 to 100

	mavlink_finalize_message_chan_send(msg, chan, 20, 20);
}

#endif

// MESSAGE VFR_HUD UNPACKING


/**
 * @brief Get field airspeed from vfr_hud message
 *
 * @return Current airspeed in m/s
 */
static inline float mavlink_msg_vfr_hud_get_airspeed(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
}

/**
 * @brief Get field groundspeed from vfr_hud message
 *
 * @return Current ground speed in m/s
 */
static inline float mavlink_msg_vfr_hud_get_groundspeed(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  4);
}

/**
 * @brief Get field heading from vfr_hud message
 *
 * @return Current heading in degrees, in compass units (0..360, 0=north)
 */
static inline int16_t mavlink_msg_vfr_hud_get_heading(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  16);
}

/**
 * @brief Get field throttle from vfr_hud message
 *
 * @return Current throttle setting in integer percent, 0 to 100
 */
static inline uint16_t mavlink_msg_vfr_hud_get_throttle(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  18);
}

/**
 * @brief Get field alt from vfr_hud message
 *
 * @return Current altitude (MSL), in meters
 */
static inline float mavlink_msg_vfr_hud_get_alt(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  8);
}

/**
 * @brief Get field climb from vfr_hud message
 *
 * @return Current climb rate in meters/second
 */
static inline float mavlink_msg_vfr_hud_get_climb(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  12);
}

/**
 * @brief Decode a vfr_hud message into a struct
 *
 * @param msg The message to decode
 * @param vfr_hud C-struct to decode the message contents into
 */
static inline void mavlink_msg_vfr_hud_decode(const mavlink_message_t* msg, mavlink_vfr_hud_t* vfr_hud)
{
#if MAVLINK_NEED_BYTE_SWAP
	vfr_hud->airspeed = mavlink_msg_vfr_hud_get_airspeed(msg);
	vfr_hud->groundspeed = mavlink_msg_vfr_hud_get_groundspeed(msg);
	vfr_hud->alt = mavlink_msg_vfr_hud_get_alt(msg);
	vfr_hud->climb = mavlink_msg_vfr_hud_get_climb(msg);
	vfr_hud->heading = mavlink_msg_vfr_hud_get_heading(msg);
	vfr_hud->throttle = mavlink_msg_vfr_hud_get_throttle(msg);
#else
	memcpy(vfr_hud, MAVLINK_PAYLOAD(msg), 20);
#endif
}
