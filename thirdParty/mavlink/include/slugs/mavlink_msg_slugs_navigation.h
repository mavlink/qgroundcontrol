// MESSAGE SLUGS_NAVIGATION PACKING

#define MAVLINK_MSG_ID_SLUGS_NAVIGATION 176
#define MAVLINK_MSG_ID_SLUGS_NAVIGATION_LEN 30
#define MAVLINK_MSG_176_LEN 30
#define MAVLINK_MSG_ID_SLUGS_NAVIGATION_KEY 0xFF
#define MAVLINK_MSG_176_KEY 0xFF

typedef struct __mavlink_slugs_navigation_t 
{
	float u_m;	///< Measured Airspeed prior to the Nav Filter
	float phi_c;	///< Commanded Roll
	float theta_c;	///< Commanded Pitch
	float psiDot_c;	///< Commanded Turn rate
	float ay_body;	///< Y component of the body acceleration
	float totalDist;	///< Total Distance to Run on this leg of Navigation
	float dist2Go;	///< Remaining distance to Run on this leg of Navigation
	uint8_t fromWP;	///< Origin WP
	uint8_t toWP;	///< Destination WP

} mavlink_slugs_navigation_t;

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
static inline uint16_t mavlink_msg_slugs_navigation_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float u_m, float phi_c, float theta_c, float psiDot_c, float ay_body, float totalDist, float dist2Go, uint8_t fromWP, uint8_t toWP)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SLUGS_NAVIGATION;

	p->u_m = u_m;	// float:Measured Airspeed prior to the Nav Filter
	p->phi_c = phi_c;	// float:Commanded Roll
	p->theta_c = theta_c;	// float:Commanded Pitch
	p->psiDot_c = psiDot_c;	// float:Commanded Turn rate
	p->ay_body = ay_body;	// float:Y component of the body acceleration
	p->totalDist = totalDist;	// float:Total Distance to Run on this leg of Navigation
	p->dist2Go = dist2Go;	// float:Remaining distance to Run on this leg of Navigation
	p->fromWP = fromWP;	// uint8_t:Origin WP
	p->toWP = toWP;	// uint8_t:Destination WP

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SLUGS_NAVIGATION_LEN);
}

/**
 * @brief Pack a slugs_navigation message
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
static inline uint16_t mavlink_msg_slugs_navigation_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float u_m, float phi_c, float theta_c, float psiDot_c, float ay_body, float totalDist, float dist2Go, uint8_t fromWP, uint8_t toWP)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SLUGS_NAVIGATION;

	p->u_m = u_m;	// float:Measured Airspeed prior to the Nav Filter
	p->phi_c = phi_c;	// float:Commanded Roll
	p->theta_c = theta_c;	// float:Commanded Pitch
	p->psiDot_c = psiDot_c;	// float:Commanded Turn rate
	p->ay_body = ay_body;	// float:Y component of the body acceleration
	p->totalDist = totalDist;	// float:Total Distance to Run on this leg of Navigation
	p->dist2Go = dist2Go;	// float:Remaining distance to Run on this leg of Navigation
	p->fromWP = fromWP;	// uint8_t:Origin WP
	p->toWP = toWP;	// uint8_t:Destination WP

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SLUGS_NAVIGATION_LEN);
}

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


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
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
static inline void mavlink_msg_slugs_navigation_send(mavlink_channel_t chan, float u_m, float phi_c, float theta_c, float psiDot_c, float ay_body, float totalDist, float dist2Go, uint8_t fromWP, uint8_t toWP)
{
	mavlink_header_t hdr;
	mavlink_slugs_navigation_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SLUGS_NAVIGATION_LEN )
	payload.u_m = u_m;	// float:Measured Airspeed prior to the Nav Filter
	payload.phi_c = phi_c;	// float:Commanded Roll
	payload.theta_c = theta_c;	// float:Commanded Pitch
	payload.psiDot_c = psiDot_c;	// float:Commanded Turn rate
	payload.ay_body = ay_body;	// float:Y component of the body acceleration
	payload.totalDist = totalDist;	// float:Total Distance to Run on this leg of Navigation
	payload.dist2Go = dist2Go;	// float:Remaining distance to Run on this leg of Navigation
	payload.fromWP = fromWP;	// uint8_t:Origin WP
	payload.toWP = toWP;	// uint8_t:Destination WP

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SLUGS_NAVIGATION_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SLUGS_NAVIGATION;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xFF, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
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
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->u_m);
}

/**
 * @brief Get field phi_c from slugs_navigation message
 *
 * @return Commanded Roll
 */
static inline float mavlink_msg_slugs_navigation_get_phi_c(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->phi_c);
}

/**
 * @brief Get field theta_c from slugs_navigation message
 *
 * @return Commanded Pitch
 */
static inline float mavlink_msg_slugs_navigation_get_theta_c(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->theta_c);
}

/**
 * @brief Get field psiDot_c from slugs_navigation message
 *
 * @return Commanded Turn rate
 */
static inline float mavlink_msg_slugs_navigation_get_psiDot_c(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->psiDot_c);
}

/**
 * @brief Get field ay_body from slugs_navigation message
 *
 * @return Y component of the body acceleration
 */
static inline float mavlink_msg_slugs_navigation_get_ay_body(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->ay_body);
}

/**
 * @brief Get field totalDist from slugs_navigation message
 *
 * @return Total Distance to Run on this leg of Navigation
 */
static inline float mavlink_msg_slugs_navigation_get_totalDist(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->totalDist);
}

/**
 * @brief Get field dist2Go from slugs_navigation message
 *
 * @return Remaining distance to Run on this leg of Navigation
 */
static inline float mavlink_msg_slugs_navigation_get_dist2Go(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (float)(p->dist2Go);
}

/**
 * @brief Get field fromWP from slugs_navigation message
 *
 * @return Origin WP
 */
static inline uint8_t mavlink_msg_slugs_navigation_get_fromWP(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (uint8_t)(p->fromWP);
}

/**
 * @brief Get field toWP from slugs_navigation message
 *
 * @return Destination WP
 */
static inline uint8_t mavlink_msg_slugs_navigation_get_toWP(const mavlink_message_t* msg)
{
	mavlink_slugs_navigation_t *p = (mavlink_slugs_navigation_t *)&msg->payload[0];
	return (uint8_t)(p->toWP);
}

/**
 * @brief Decode a slugs_navigation message into a struct
 *
 * @param msg The message to decode
 * @param slugs_navigation C-struct to decode the message contents into
 */
static inline void mavlink_msg_slugs_navigation_decode(const mavlink_message_t* msg, mavlink_slugs_navigation_t* slugs_navigation)
{
	memcpy( slugs_navigation, msg->payload, sizeof(mavlink_slugs_navigation_t));
}
