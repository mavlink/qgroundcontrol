// MESSAGE RADIO_CALIBRATION PACKING

#define MAVLINK_MSG_ID_RADIO_CALIBRATION 221
#define MAVLINK_MSG_ID_RADIO_CALIBRATION_LEN 42
#define MAVLINK_MSG_221_LEN 42
#define MAVLINK_MSG_ID_RADIO_CALIBRATION_KEY 0x49
#define MAVLINK_MSG_221_KEY 0x49

typedef struct __mavlink_radio_calibration_t 
{
	uint16_t aileron[3];	///< Aileron setpoints: left, center, right
	uint16_t elevator[3];	///< Elevator setpoints: nose down, center, nose up
	uint16_t rudder[3];	///< Rudder setpoints: nose left, center, nose right
	uint16_t gyro[2];	///< Tail gyro mode/gain setpoints: heading hold, rate mode
	uint16_t pitch[5];	///< Pitch curve setpoints (every 25%)
	uint16_t throttle[5];	///< Throttle curve setpoints (every 25%)

} mavlink_radio_calibration_t;
#define MAVLINK_MSG_RADIO_CALIBRATION_FIELD_AILERON_LEN 3
#define MAVLINK_MSG_RADIO_CALIBRATION_FIELD_ELEVATOR_LEN 3
#define MAVLINK_MSG_RADIO_CALIBRATION_FIELD_RUDDER_LEN 3
#define MAVLINK_MSG_RADIO_CALIBRATION_FIELD_GYRO_LEN 2
#define MAVLINK_MSG_RADIO_CALIBRATION_FIELD_PITCH_LEN 5
#define MAVLINK_MSG_RADIO_CALIBRATION_FIELD_THROTTLE_LEN 5

/**
 * @brief Pack a radio_calibration message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param aileron Aileron setpoints: left, center, right
 * @param elevator Elevator setpoints: nose down, center, nose up
 * @param rudder Rudder setpoints: nose left, center, nose right
 * @param gyro Tail gyro mode/gain setpoints: heading hold, rate mode
 * @param pitch Pitch curve setpoints (every 25%)
 * @param throttle Throttle curve setpoints (every 25%)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_radio_calibration_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const uint16_t* aileron, const uint16_t* elevator, const uint16_t* rudder, const uint16_t* gyro, const uint16_t* pitch, const uint16_t* throttle)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RADIO_CALIBRATION;

	memcpy(p->aileron, aileron, sizeof(p->aileron));	// uint16_t[3]:Aileron setpoints: left, center, right
	memcpy(p->elevator, elevator, sizeof(p->elevator));	// uint16_t[3]:Elevator setpoints: nose down, center, nose up
	memcpy(p->rudder, rudder, sizeof(p->rudder));	// uint16_t[3]:Rudder setpoints: nose left, center, nose right
	memcpy(p->gyro, gyro, sizeof(p->gyro));	// uint16_t[2]:Tail gyro mode/gain setpoints: heading hold, rate mode
	memcpy(p->pitch, pitch, sizeof(p->pitch));	// uint16_t[5]:Pitch curve setpoints (every 25%)
	memcpy(p->throttle, throttle, sizeof(p->throttle));	// uint16_t[5]:Throttle curve setpoints (every 25%)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RADIO_CALIBRATION_LEN);
}

/**
 * @brief Pack a radio_calibration message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param aileron Aileron setpoints: left, center, right
 * @param elevator Elevator setpoints: nose down, center, nose up
 * @param rudder Rudder setpoints: nose left, center, nose right
 * @param gyro Tail gyro mode/gain setpoints: heading hold, rate mode
 * @param pitch Pitch curve setpoints (every 25%)
 * @param throttle Throttle curve setpoints (every 25%)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_radio_calibration_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const uint16_t* aileron, const uint16_t* elevator, const uint16_t* rudder, const uint16_t* gyro, const uint16_t* pitch, const uint16_t* throttle)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RADIO_CALIBRATION;

	memcpy(p->aileron, aileron, sizeof(p->aileron));	// uint16_t[3]:Aileron setpoints: left, center, right
	memcpy(p->elevator, elevator, sizeof(p->elevator));	// uint16_t[3]:Elevator setpoints: nose down, center, nose up
	memcpy(p->rudder, rudder, sizeof(p->rudder));	// uint16_t[3]:Rudder setpoints: nose left, center, nose right
	memcpy(p->gyro, gyro, sizeof(p->gyro));	// uint16_t[2]:Tail gyro mode/gain setpoints: heading hold, rate mode
	memcpy(p->pitch, pitch, sizeof(p->pitch));	// uint16_t[5]:Pitch curve setpoints (every 25%)
	memcpy(p->throttle, throttle, sizeof(p->throttle));	// uint16_t[5]:Throttle curve setpoints (every 25%)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RADIO_CALIBRATION_LEN);
}

/**
 * @brief Encode a radio_calibration struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param radio_calibration C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_radio_calibration_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_radio_calibration_t* radio_calibration)
{
	return mavlink_msg_radio_calibration_pack(system_id, component_id, msg, radio_calibration->aileron, radio_calibration->elevator, radio_calibration->rudder, radio_calibration->gyro, radio_calibration->pitch, radio_calibration->throttle);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a radio_calibration message
 * @param chan MAVLink channel to send the message
 *
 * @param aileron Aileron setpoints: left, center, right
 * @param elevator Elevator setpoints: nose down, center, nose up
 * @param rudder Rudder setpoints: nose left, center, nose right
 * @param gyro Tail gyro mode/gain setpoints: heading hold, rate mode
 * @param pitch Pitch curve setpoints (every 25%)
 * @param throttle Throttle curve setpoints (every 25%)
 */
static inline void mavlink_msg_radio_calibration_send(mavlink_channel_t chan, const uint16_t* aileron, const uint16_t* elevator, const uint16_t* rudder, const uint16_t* gyro, const uint16_t* pitch, const uint16_t* throttle)
{
	mavlink_header_t hdr;
	mavlink_radio_calibration_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_RADIO_CALIBRATION_LEN )
	memcpy(payload.aileron, aileron, sizeof(payload.aileron));	// uint16_t[3]:Aileron setpoints: left, center, right
	memcpy(payload.elevator, elevator, sizeof(payload.elevator));	// uint16_t[3]:Elevator setpoints: nose down, center, nose up
	memcpy(payload.rudder, rudder, sizeof(payload.rudder));	// uint16_t[3]:Rudder setpoints: nose left, center, nose right
	memcpy(payload.gyro, gyro, sizeof(payload.gyro));	// uint16_t[2]:Tail gyro mode/gain setpoints: heading hold, rate mode
	memcpy(payload.pitch, pitch, sizeof(payload.pitch));	// uint16_t[5]:Pitch curve setpoints (every 25%)
	memcpy(payload.throttle, throttle, sizeof(payload.throttle));	// uint16_t[5]:Throttle curve setpoints (every 25%)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_RADIO_CALIBRATION_LEN;
	hdr.msgid = MAVLINK_MSG_ID_RADIO_CALIBRATION;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x49, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE RADIO_CALIBRATION UNPACKING

/**
 * @brief Get field aileron from radio_calibration message
 *
 * @return Aileron setpoints: left, center, right
 */
static inline uint16_t mavlink_msg_radio_calibration_get_aileron(const mavlink_message_t* msg, uint16_t* aileron)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];

	memcpy(aileron, p->aileron, sizeof(p->aileron));
	return sizeof(p->aileron);
}

/**
 * @brief Get field elevator from radio_calibration message
 *
 * @return Elevator setpoints: nose down, center, nose up
 */
static inline uint16_t mavlink_msg_radio_calibration_get_elevator(const mavlink_message_t* msg, uint16_t* elevator)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];

	memcpy(elevator, p->elevator, sizeof(p->elevator));
	return sizeof(p->elevator);
}

/**
 * @brief Get field rudder from radio_calibration message
 *
 * @return Rudder setpoints: nose left, center, nose right
 */
static inline uint16_t mavlink_msg_radio_calibration_get_rudder(const mavlink_message_t* msg, uint16_t* rudder)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];

	memcpy(rudder, p->rudder, sizeof(p->rudder));
	return sizeof(p->rudder);
}

/**
 * @brief Get field gyro from radio_calibration message
 *
 * @return Tail gyro mode/gain setpoints: heading hold, rate mode
 */
static inline uint16_t mavlink_msg_radio_calibration_get_gyro(const mavlink_message_t* msg, uint16_t* gyro)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];

	memcpy(gyro, p->gyro, sizeof(p->gyro));
	return sizeof(p->gyro);
}

/**
 * @brief Get field pitch from radio_calibration message
 *
 * @return Pitch curve setpoints (every 25%)
 */
static inline uint16_t mavlink_msg_radio_calibration_get_pitch(const mavlink_message_t* msg, uint16_t* pitch)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];

	memcpy(pitch, p->pitch, sizeof(p->pitch));
	return sizeof(p->pitch);
}

/**
 * @brief Get field throttle from radio_calibration message
 *
 * @return Throttle curve setpoints (every 25%)
 */
static inline uint16_t mavlink_msg_radio_calibration_get_throttle(const mavlink_message_t* msg, uint16_t* throttle)
{
	mavlink_radio_calibration_t *p = (mavlink_radio_calibration_t *)&msg->payload[0];

	memcpy(throttle, p->throttle, sizeof(p->throttle));
	return sizeof(p->throttle);
}

/**
 * @brief Decode a radio_calibration message into a struct
 *
 * @param msg The message to decode
 * @param radio_calibration C-struct to decode the message contents into
 */
static inline void mavlink_msg_radio_calibration_decode(const mavlink_message_t* msg, mavlink_radio_calibration_t* radio_calibration)
{
	memcpy( radio_calibration, msg->payload, sizeof(mavlink_radio_calibration_t));
}
