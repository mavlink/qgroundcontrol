// MESSAGE OBS_AIR_VELOCITY PACKING

#define MAVLINK_MSG_ID_OBS_AIR_VELOCITY 178

typedef struct __mavlink_obs_air_velocity_t
{
 float magnitude; ///< 
                
            
 float aoa; ///< 
                
            
 float slip; ///< 
                
            
} mavlink_obs_air_velocity_t;

#define MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN 12
#define MAVLINK_MSG_ID_178_LEN 12

#define MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC 32
#define MAVLINK_MSG_ID_178_CRC 32



#define MAVLINK_MESSAGE_INFO_OBS_AIR_VELOCITY { \
	"OBS_AIR_VELOCITY", \
	3, \
	{  { "magnitude", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_obs_air_velocity_t, magnitude) }, \
         { "aoa", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_obs_air_velocity_t, aoa) }, \
         { "slip", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_obs_air_velocity_t, slip) }, \
         } \
}


/**
 * @brief Pack a obs_air_velocity message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param magnitude 
                
            
 * @param aoa 
                
            
 * @param slip 
                
            
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_air_velocity_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       float magnitude, float aoa, float slip)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN];
	_mav_put_float(buf, 0, magnitude);
	_mav_put_float(buf, 4, aoa);
	_mav_put_float(buf, 8, slip);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#else
	mavlink_obs_air_velocity_t packet;
	packet.magnitude = magnitude;
	packet.aoa = aoa;
	packet.slip = slip;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_OBS_AIR_VELOCITY;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
}

/**
 * @brief Pack a obs_air_velocity message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param magnitude 
                
            
 * @param aoa 
                
            
 * @param slip 
                
            
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_air_velocity_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           float magnitude,float aoa,float slip)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN];
	_mav_put_float(buf, 0, magnitude);
	_mav_put_float(buf, 4, aoa);
	_mav_put_float(buf, 8, slip);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#else
	mavlink_obs_air_velocity_t packet;
	packet.magnitude = magnitude;
	packet.aoa = aoa;
	packet.slip = slip;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_OBS_AIR_VELOCITY;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
}

/**
 * @brief Encode a obs_air_velocity struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_air_velocity C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_air_velocity_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_air_velocity_t* obs_air_velocity)
{
	return mavlink_msg_obs_air_velocity_pack(system_id, component_id, msg, obs_air_velocity->magnitude, obs_air_velocity->aoa, obs_air_velocity->slip);
}

/**
 * @brief Encode a obs_air_velocity struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param obs_air_velocity C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_air_velocity_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_obs_air_velocity_t* obs_air_velocity)
{
	return mavlink_msg_obs_air_velocity_pack_chan(system_id, component_id, chan, msg, obs_air_velocity->magnitude, obs_air_velocity->aoa, obs_air_velocity->slip);
}

/**
 * @brief Send a obs_air_velocity message
 * @param chan MAVLink channel to send the message
 *
 * @param magnitude 
                
            
 * @param aoa 
                
            
 * @param slip 
                
            
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_air_velocity_send(mavlink_channel_t chan, float magnitude, float aoa, float slip)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN];
	_mav_put_float(buf, 0, magnitude);
	_mav_put_float(buf, 4, aoa);
	_mav_put_float(buf, 8, slip);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, buf, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, buf, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
#else
	mavlink_obs_air_velocity_t packet;
	packet.magnitude = magnitude;
	packet.aoa = aoa;
	packet.slip = slip;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, (const char *)&packet, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, (const char *)&packet, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_obs_air_velocity_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  float magnitude, float aoa, float slip)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_float(buf, 0, magnitude);
	_mav_put_float(buf, 4, aoa);
	_mav_put_float(buf, 8, slip);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, buf, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, buf, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
#else
	mavlink_obs_air_velocity_t *packet = (mavlink_obs_air_velocity_t *)msgbuf;
	packet->magnitude = magnitude;
	packet->aoa = aoa;
	packet->slip = slip;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, (const char *)packet, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_OBS_AIR_VELOCITY, (const char *)packet, MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE OBS_AIR_VELOCITY UNPACKING


/**
 * @brief Get field magnitude from obs_air_velocity message
 *
 * @return 
                
            
 */
static inline float mavlink_msg_obs_air_velocity_get_magnitude(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field aoa from obs_air_velocity message
 *
 * @return 
                
            
 */
static inline float mavlink_msg_obs_air_velocity_get_aoa(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field slip from obs_air_velocity message
 *
 * @return 
                
            
 */
static inline float mavlink_msg_obs_air_velocity_get_slip(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Decode a obs_air_velocity message into a struct
 *
 * @param msg The message to decode
 * @param obs_air_velocity C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_air_velocity_decode(const mavlink_message_t* msg, mavlink_obs_air_velocity_t* obs_air_velocity)
{
#if MAVLINK_NEED_BYTE_SWAP
	obs_air_velocity->magnitude = mavlink_msg_obs_air_velocity_get_magnitude(msg);
	obs_air_velocity->aoa = mavlink_msg_obs_air_velocity_get_aoa(msg);
	obs_air_velocity->slip = mavlink_msg_obs_air_velocity_get_slip(msg);
#else
	memcpy(obs_air_velocity, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_OBS_AIR_VELOCITY_LEN);
#endif
}
