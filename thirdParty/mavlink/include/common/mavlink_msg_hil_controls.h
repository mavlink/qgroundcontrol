// MESSAGE HIL_CONTROLS PACKING

#define MAVLINK_MSG_ID_HIL_CONTROLS 91
#define MAVLINK_MSG_ID_HIL_CONTROLS_LEN 42
#define MAVLINK_MSG_91_LEN 42
#define MAVLINK_MSG_ID_HIL_CONTROLS_KEY 0x67
#define MAVLINK_MSG_91_KEY 0x67

typedef struct __mavlink_hil_controls_t 
{
	uint64_t time_us;	///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	float roll_ailerons;	///< Control output -1 .. 1
	float pitch_elevator;	///< Control output -1 .. 1
	float yaw_rudder;	///< Control output -1 .. 1
	float throttle;	///< Throttle 0 .. 1
	float aux1;	///< Aux 1, -1 .. 1
	float aux2;	///< Aux 2, -1 .. 1
	float aux3;	///< Aux 3, -1 .. 1
	float aux4;	///< Aux 4, -1 .. 1
	uint8_t mode;	///< System mode (MAV_MODE)
	uint8_t nav_mode;	///< Navigation mode (MAV_NAV_MODE)

} mavlink_hil_controls_t;

/**
 * @brief Pack a hil_controls message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll_ailerons Control output -1 .. 1
 * @param pitch_elevator Control output -1 .. 1
 * @param yaw_rudder Control output -1 .. 1
 * @param throttle Throttle 0 .. 1
 * @param aux1 Aux 1, -1 .. 1
 * @param aux2 Aux 2, -1 .. 1
 * @param aux3 Aux 3, -1 .. 1
 * @param aux4 Aux 4, -1 .. 1
 * @param mode System mode (MAV_MODE)
 * @param nav_mode Navigation mode (MAV_NAV_MODE)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_controls_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t time_us, float roll_ailerons, float pitch_elevator, float yaw_rudder, float throttle, float aux1, float aux2, float aux3, float aux4, uint8_t mode, uint8_t nav_mode)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HIL_CONTROLS;

	p->time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll_ailerons = roll_ailerons;	// float:Control output -1 .. 1
	p->pitch_elevator = pitch_elevator;	// float:Control output -1 .. 1
	p->yaw_rudder = yaw_rudder;	// float:Control output -1 .. 1
	p->throttle = throttle;	// float:Throttle 0 .. 1
	p->aux1 = aux1;	// float:Aux 1, -1 .. 1
	p->aux2 = aux2;	// float:Aux 2, -1 .. 1
	p->aux3 = aux3;	// float:Aux 3, -1 .. 1
	p->aux4 = aux4;	// float:Aux 4, -1 .. 1
	p->mode = mode;	// uint8_t:System mode (MAV_MODE)
	p->nav_mode = nav_mode;	// uint8_t:Navigation mode (MAV_NAV_MODE)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_CONTROLS_LEN);
}

/**
 * @brief Pack a hil_controls message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll_ailerons Control output -1 .. 1
 * @param pitch_elevator Control output -1 .. 1
 * @param yaw_rudder Control output -1 .. 1
 * @param throttle Throttle 0 .. 1
 * @param aux1 Aux 1, -1 .. 1
 * @param aux2 Aux 2, -1 .. 1
 * @param aux3 Aux 3, -1 .. 1
 * @param aux4 Aux 4, -1 .. 1
 * @param mode System mode (MAV_MODE)
 * @param nav_mode Navigation mode (MAV_NAV_MODE)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_controls_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t time_us, float roll_ailerons, float pitch_elevator, float yaw_rudder, float throttle, float aux1, float aux2, float aux3, float aux4, uint8_t mode, uint8_t nav_mode)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HIL_CONTROLS;

	p->time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll_ailerons = roll_ailerons;	// float:Control output -1 .. 1
	p->pitch_elevator = pitch_elevator;	// float:Control output -1 .. 1
	p->yaw_rudder = yaw_rudder;	// float:Control output -1 .. 1
	p->throttle = throttle;	// float:Throttle 0 .. 1
	p->aux1 = aux1;	// float:Aux 1, -1 .. 1
	p->aux2 = aux2;	// float:Aux 2, -1 .. 1
	p->aux3 = aux3;	// float:Aux 3, -1 .. 1
	p->aux4 = aux4;	// float:Aux 4, -1 .. 1
	p->mode = mode;	// uint8_t:System mode (MAV_MODE)
	p->nav_mode = nav_mode;	// uint8_t:Navigation mode (MAV_NAV_MODE)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_CONTROLS_LEN);
}

/**
 * @brief Encode a hil_controls struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param hil_controls C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_hil_controls_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_hil_controls_t* hil_controls)
{
	return mavlink_msg_hil_controls_pack(system_id, component_id, msg, hil_controls->time_us, hil_controls->roll_ailerons, hil_controls->pitch_elevator, hil_controls->yaw_rudder, hil_controls->throttle, hil_controls->aux1, hil_controls->aux2, hil_controls->aux3, hil_controls->aux4, hil_controls->mode, hil_controls->nav_mode);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a hil_controls message
 * @param chan MAVLink channel to send the message
 *
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll_ailerons Control output -1 .. 1
 * @param pitch_elevator Control output -1 .. 1
 * @param yaw_rudder Control output -1 .. 1
 * @param throttle Throttle 0 .. 1
 * @param aux1 Aux 1, -1 .. 1
 * @param aux2 Aux 2, -1 .. 1
 * @param aux3 Aux 3, -1 .. 1
 * @param aux4 Aux 4, -1 .. 1
 * @param mode System mode (MAV_MODE)
 * @param nav_mode Navigation mode (MAV_NAV_MODE)
 */
static inline void mavlink_msg_hil_controls_send(mavlink_channel_t chan, uint64_t time_us, float roll_ailerons, float pitch_elevator, float yaw_rudder, float throttle, float aux1, float aux2, float aux3, float aux4, uint8_t mode, uint8_t nav_mode)
{
	mavlink_header_t hdr;
	mavlink_hil_controls_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_HIL_CONTROLS_LEN )
	payload.time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	payload.roll_ailerons = roll_ailerons;	// float:Control output -1 .. 1
	payload.pitch_elevator = pitch_elevator;	// float:Control output -1 .. 1
	payload.yaw_rudder = yaw_rudder;	// float:Control output -1 .. 1
	payload.throttle = throttle;	// float:Throttle 0 .. 1
	payload.aux1 = aux1;	// float:Aux 1, -1 .. 1
	payload.aux2 = aux2;	// float:Aux 2, -1 .. 1
	payload.aux3 = aux3;	// float:Aux 3, -1 .. 1
	payload.aux4 = aux4;	// float:Aux 4, -1 .. 1
	payload.mode = mode;	// uint8_t:System mode (MAV_MODE)
	payload.nav_mode = nav_mode;	// uint8_t:Navigation mode (MAV_NAV_MODE)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_HIL_CONTROLS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_HIL_CONTROLS;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x67, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE HIL_CONTROLS UNPACKING

/**
 * @brief Get field time_us from hil_controls message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_hil_controls_get_time_us(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (uint64_t)(p->time_us);
}

/**
 * @brief Get field roll_ailerons from hil_controls message
 *
 * @return Control output -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_roll_ailerons(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->roll_ailerons);
}

/**
 * @brief Get field pitch_elevator from hil_controls message
 *
 * @return Control output -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_pitch_elevator(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->pitch_elevator);
}

/**
 * @brief Get field yaw_rudder from hil_controls message
 *
 * @return Control output -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_yaw_rudder(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->yaw_rudder);
}

/**
 * @brief Get field throttle from hil_controls message
 *
 * @return Throttle 0 .. 1
 */
static inline float mavlink_msg_hil_controls_get_throttle(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->throttle);
}

/**
 * @brief Get field aux1 from hil_controls message
 *
 * @return Aux 1, -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_aux1(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->aux1);
}

/**
 * @brief Get field aux2 from hil_controls message
 *
 * @return Aux 2, -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_aux2(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->aux2);
}

/**
 * @brief Get field aux3 from hil_controls message
 *
 * @return Aux 3, -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_aux3(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->aux3);
}

/**
 * @brief Get field aux4 from hil_controls message
 *
 * @return Aux 4, -1 .. 1
 */
static inline float mavlink_msg_hil_controls_get_aux4(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (float)(p->aux4);
}

/**
 * @brief Get field mode from hil_controls message
 *
 * @return System mode (MAV_MODE)
 */
static inline uint8_t mavlink_msg_hil_controls_get_mode(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (uint8_t)(p->mode);
}

/**
 * @brief Get field nav_mode from hil_controls message
 *
 * @return Navigation mode (MAV_NAV_MODE)
 */
static inline uint8_t mavlink_msg_hil_controls_get_nav_mode(const mavlink_message_t* msg)
{
	mavlink_hil_controls_t *p = (mavlink_hil_controls_t *)&msg->payload[0];
	return (uint8_t)(p->nav_mode);
}

/**
 * @brief Decode a hil_controls message into a struct
 *
 * @param msg The message to decode
 * @param hil_controls C-struct to decode the message contents into
 */
static inline void mavlink_msg_hil_controls_decode(const mavlink_message_t* msg, mavlink_hil_controls_t* hil_controls)
{
	memcpy( hil_controls, msg->payload, sizeof(mavlink_hil_controls_t));
}
