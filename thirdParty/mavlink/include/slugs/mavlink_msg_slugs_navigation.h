// MESSAGE SLUGS_NAVIGATION PACKING

#define MAVLINK_MSG_ID_SLUGS_NAVIGATION 176

typedef struct __mavlink_slugs_navigation_t
{
 float u_m; ///< Measured Airspeed prior to the Nav Filter
 float phi_c; ///< Commanded Roll
 float theta_c; ///< Commanded Pitch
 float psiDot_c; ///< Commanded Turn rate
 float ay_body; ///< Y component of the body acceleration
 float totalDist; ///< Total Distance to Run on this leg of Navigation
 float dist2Go; ///< Remaining distance to Run on this leg of Navigation
 uint8_t fromWP; ///< Origin WP
 uint8_t toWP; ///< Destination WP
} mavlink_slugs_navigation_t;

#define MAVLINK_MSG_ID_SLUGS_NAVIGATION_LEN 30
#define MAVLINK_MSG_ID_176_LEN 30



#define MAVLINK_MESSAGE_INFO_SLUGS_NAVIGATION { \
	"SLUGS_NAVIGATION", \
	9, \
	{  { "u_m", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_slugs_navigation_t, u_m) }, \
         { "phi_c", MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_slugs_navigation_t, phi_c) }, \
         { "theta_c", MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_slugs_navigation_t, theta_c) }, \
         { "psiDot_c", MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_slugs_navigation_t, psiDot_c) }, \
         { "ay_body", MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_slugs_navigation_t, ay_body) }, \
         { "totalDist", MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_slugs_navigation_t, totalDist) }, \
         { "dist2Go", MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_slugs_navigation_t, dist2Go) }, \
         { "fromWP", MAVLINK_TYPE_UINT8_T, 0, 28, offsetof(mavlink_slugs_navigation_t, fromWP) }, \
         { "toWP", MAVLINK_TYPE_UINT8_T, 0, 29, offsetof(mavlink_slugs_navigation_t, toWP) }, \
         } \
}


/**
 * @brief Pack a slugs_navigation message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param u_m Measured Airspeed prior to the Nav Filter
 * @param phi_c Commanded Roll
 * @param theta_c Commanded Pitch
 * @param psiDot_c Commanded Turn rate
 * @param ay_body Y component of the body acceleration
 * @param totalDist Total Distance to Run on this leg of Navigation
 * @param dist2Go Remaining distance to Run on this leg of Navigation
 * @param fromWP Origin WP
 * @param toWP Destination WP
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_slugs_navigation_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       float u_m, float phi_c, float theta_c, float psiDot_c, float ay_body, float totalDist, float dist2Go, uint8_t fromWP, uint8_t toWP)
{
	msg->msgid = MAVLINK_MSG_ID_SLUGS_NAVIGATION;

	put_float_by_index(msg, 0, u_m); // Measured Airspeed prior to the Nav Filter
	put_float_by_index(msg, 4, phi_c); // Commanded Roll
	put_float_by_index(msg, 8, theta_c); // Commanded Pitch
	put_float_by_index(msg, 12, psiDot_c); // Commanded Turn rate
	put_float_by_index(msg, 16, ay_body); // Y component of the body acceleration
	put_float_by_index(msg, 20, totalDist); // Total Distance to Run on this leg of Navigation
	put_float_by_index(msg, 24, dist2Go); // Remaining distance to Run on this leg of Navigation
	put_uint8_t_by_index(msg, 28, fromWP); // Origin WP
	put_uint8_t_by_index(msg, 29, toWP); // Destination WP

	return mavlink_finalize_message(msg, system_id, component_id, 30, 120);
}

/**
 * @brief Pack a slugs_navigation message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param u_m Measured Airspeed prior to the Nav Filter
 * @param phi_c Commanded Roll
 * @param theta_c Commanded Pitch
 * @param psiDot_c Commanded Turn rate
 * @param ay_body Y component of the body acceleration
 * @param totalDist Total Distance to Run on this leg of Navigation
 * @param dist2Go Remaining distance to Run on this leg of Navigation
 * @param fromWP Origin WP
 * @param toWP Destination WP
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_slugs_navigation_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           float u_m,float phi_c,float theta_c,float psiDot_c,float ay_body,float totalDist,float dist2Go,uint8_t fromWP,uint8_t toWP)
{
	msg->msgid = MAVLINK_MSG_ID_SLUGS_NAVIGATION;

	put_float_by_index(msg, 0, u_m); // Measured Airspeed prior to the Nav Filter
	put_float_by_index(msg, 4, phi_c); // Commanded Roll
	put_float_by_index(msg, 8, theta_c); // Commanded Pitch
	put_float_by_index(msg, 12, psiDot_c); // Commanded Turn rate
	put_float_by_index(msg, 16, ay_body); // Y component of the body acceleration
	put_float_by_index(msg, 20, totalDist); // Total Distance to Run on this leg of Navigation
	put_float_by_index(msg, 24, dist2Go); // Remaining distance to Run on this leg of Navigation
	put_uint8_t_by_index(msg, 28, fromWP); // Origin WP
	put_uint8_t_by_index(msg, 29, toWP); // Destination WP

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 30, 120);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a slugs_navigation message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param u_m Measured Airspeed prior to the Nav Filter
 * @param phi_c Commanded Roll
 * @param theta_c Commanded Pitch
 * @param psiDot_c Commanded Turn rate
 * @param ay_body Y component of the body acceleration
 * @param totalDist Total Distance to Run on this leg of Navigation
 * @param dist2Go Remaining distance to Run on this leg of Navigation
 * @param fromWP Origin WP
 * @param toWP Destination WP
 */
static inline void mavlink_msg_slugs_navigation_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           float u_m,float phi_c,float theta_c,float psiDot_c,float ay_body,float totalDist,float dist2Go,uint8_t fromWP,uint8_t toWP)
{
	msg->msgid = MAVLINK_MSG_ID_SLUGS_NAVIGATION;

	put_float_by_index(msg, 0, u_m); // Measured Airspeed prior to the Nav Filter
	put_float_by_index(msg, 4, phi_c); // Commanded Roll
	put_float_by_index(msg, 8, theta_c); // Commanded Pitch
	put_float_by_index(msg, 12, psiDot_c); // Commanded Turn rate
	put_float_by_index(msg, 16, ay_body); // Y component of the body acceleration
	put_float_by_index(msg, 20, totalDist); // Total Distance to Run on this leg of Navigation
	put_float_by_index(msg, 24, dist2Go); // Remaining distance to Run on this leg of Navigation
	put_uint8_t_by_index(msg, 28, fromWP); // Origin WP
	put_uint8_t_by_index(msg, 29, toWP); // Destination WP

	mavlink_finalize_message_chan_send(msg, chan, 30, 120);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a slugs_navigation struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param slugs_navigation C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_slugs_navigation_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_slugs_navigation_t* slugs_navigation)
{
	return mavlink_msg_slugs_navigation_pack(system_id, component_id, msg, slugs_navigation->u_m, slugs_navigation->phi_c, slugs_navigation->theta_c, slugs_navigation->psiDot_c, slugs_navigation->ay_body, slugs_navigation->totalDist, slugs_navigation->dist2Go, slugs_navigation->fromWP, slugs_navigation->toWP);
}

/**
 * @brief Send a slugs_navigation message
 * @param chan MAVLink channel to send the message
 *
 * @param u_m Measured Airspeed prior to the Nav Filter
 * @param phi_c Commanded Roll
 * @param theta_c Commanded Pitch
 * @param psiDot_c Commanded Turn rate
 * @param ay_body Y component of the body acceleration
 * @param totalDist Total Distance to Run on this leg of Navigation
 * @param dist2Go Remaining distance to Run on this leg of Navigation
 * @param fromWP Origin WP
 * @param toWP Destination WP
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_slugs_navigation_send(mavlink_channel_t chan, float u_m, float phi_c, float theta_c, float psiDot_c, float ay_body, float totalDist, float dist2Go, uint8_t fromWP, uint8_t toWP)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 30);
	mavlink_msg_slugs_navigation_pack_chan_send(chan, msg, u_m, phi_c, theta_c, psiDot_c, ay_body, totalDist, dist2Go, fromWP, toWP);
}

#endif

// MESSAGE SLUGS_NAVIGATION UNPACKING


/**
 * @brief Get field u_m from slugs_navigation message
 *
 * @return Measured Airspeed prior to the Nav Filter
 */
static inline float mavlink_msg_slugs_navigation_get_u_m(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
}

/**
 * @brief Get field phi_c from slugs_navigation message
 *
 * @return Commanded Roll
 */
static inline float mavlink_msg_slugs_navigation_get_phi_c(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  4);
}

/**
 * @brief Get field theta_c from slugs_navigation message
 *
 * @return Commanded Pitch
 */
static inline float mavlink_msg_slugs_navigation_get_theta_c(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  8);
}

/**
 * @brief Get field psiDot_c from slugs_navigation message
 *
 * @return Commanded Turn rate
 */
static inline float mavlink_msg_slugs_navigation_get_psiDot_c(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  12);
}

/**
 * @brief Get field ay_body from slugs_navigation message
 *
 * @return Y component of the body acceleration
 */
static inline float mavlink_msg_slugs_navigation_get_ay_body(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  16);
}

/**
 * @brief Get field totalDist from slugs_navigation message
 *
 * @return Total Distance to Run on this leg of Navigation
 */
static inline float mavlink_msg_slugs_navigation_get_totalDist(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  20);
}

/**
 * @brief Get field dist2Go from slugs_navigation message
 *
 * @return Remaining distance to Run on this leg of Navigation
 */
static inline float mavlink_msg_slugs_navigation_get_dist2Go(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  24);
}

/**
 * @brief Get field fromWP from slugs_navigation message
 *
 * @return Origin WP
 */
static inline uint8_t mavlink_msg_slugs_navigation_get_fromWP(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  28);
}

/**
 * @brief Get field toWP from slugs_navigation message
 *
 * @return Destination WP
 */
static inline uint8_t mavlink_msg_slugs_navigation_get_toWP(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  29);
}

/**
 * @brief Decode a slugs_navigation message into a struct
 *
 * @param msg The message to decode
 * @param slugs_navigation C-struct to decode the message contents into
 */
static inline void mavlink_msg_slugs_navigation_decode(const mavlink_message_t* msg, mavlink_slugs_navigation_t* slugs_navigation)
{
#if MAVLINK_NEED_BYTE_SWAP
	slugs_navigation->u_m = mavlink_msg_slugs_navigation_get_u_m(msg);
	slugs_navigation->phi_c = mavlink_msg_slugs_navigation_get_phi_c(msg);
	slugs_navigation->theta_c = mavlink_msg_slugs_navigation_get_theta_c(msg);
	slugs_navigation->psiDot_c = mavlink_msg_slugs_navigation_get_psiDot_c(msg);
	slugs_navigation->ay_body = mavlink_msg_slugs_navigation_get_ay_body(msg);
	slugs_navigation->totalDist = mavlink_msg_slugs_navigation_get_totalDist(msg);
	slugs_navigation->dist2Go = mavlink_msg_slugs_navigation_get_dist2Go(msg);
	slugs_navigation->fromWP = mavlink_msg_slugs_navigation_get_fromWP(msg);
	slugs_navigation->toWP = mavlink_msg_slugs_navigation_get_toWP(msg);
#else
	memcpy(slugs_navigation, MAVLINK_PAYLOAD(msg), 30);
#endif
}
