// MESSAGE FILT_ROT_VEL PACKING

#define MAVLINK_MSG_ID_FILT_ROT_VEL 184

typedef struct __mavlink_filt_rot_vel_t 
{
	float rotVel[3]; ///< rotational velocity

} mavlink_filt_rot_vel_t;

#define MAVLINK_MSG_FILT_ROT_VEL_FIELD_ROTVEL_LEN 3


/**
 * @brief Pack a filt_rot_vel message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param rotVel rotational velocity
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_filt_rot_vel_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const float* rotVel)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_FILT_ROT_VEL;

	i += put_array_by_index((const int8_t*)rotVel, sizeof(float)*3, i, msg->payload); // rotational velocity

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a filt_rot_vel message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param rotVel rotational velocity
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_filt_rot_vel_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const float* rotVel)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_FILT_ROT_VEL;

	i += put_array_by_index((const int8_t*)rotVel, sizeof(float)*3, i, msg->payload); // rotational velocity

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a filt_rot_vel struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param filt_rot_vel C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_filt_rot_vel_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_filt_rot_vel_t* filt_rot_vel)
{
	return mavlink_msg_filt_rot_vel_pack(system_id, component_id, msg, filt_rot_vel->rotVel);
}

/**
 * @brief Send a filt_rot_vel message
 * @param chan MAVLink channel to send the message
 *
 * @param rotVel rotational velocity
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_filt_rot_vel_send(mavlink_channel_t chan, const float* rotVel)
{
	mavlink_message_t msg;
	mavlink_msg_filt_rot_vel_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, rotVel);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE FILT_ROT_VEL UNPACKING

/**
 * @brief Get field rotVel from filt_rot_vel message
 *
 * @return rotational velocity
 */
static inline uint16_t mavlink_msg_filt_rot_vel_get_rotVel(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload, sizeof(float)*3);
	return sizeof(float)*3;
}

/**
 * @brief Decode a filt_rot_vel message into a struct
 *
 * @param msg The message to decode
 * @param filt_rot_vel C-struct to decode the message contents into
 */
static inline void mavlink_msg_filt_rot_vel_decode(const mavlink_message_t* msg, mavlink_filt_rot_vel_t* filt_rot_vel)
{
	mavlink_msg_filt_rot_vel_get_rotVel(msg, filt_rot_vel->rotVel);
}
