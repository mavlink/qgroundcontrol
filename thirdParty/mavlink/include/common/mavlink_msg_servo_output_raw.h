// MESSAGE SERVO_OUTPUT_RAW PACKING

#define MAVLINK_MSG_ID_SERVO_OUTPUT_RAW 37
#define MAVLINK_MSG_ID_SERVO_OUTPUT_RAW_LEN 16
#define MAVLINK_MSG_37_LEN 16

typedef struct __mavlink_servo_output_raw_t 
{
	uint16_t servo1_raw; ///< Servo output 1 value, in microseconds
	uint16_t servo2_raw; ///< Servo output 2 value, in microseconds
	uint16_t servo3_raw; ///< Servo output 3 value, in microseconds
	uint16_t servo4_raw; ///< Servo output 4 value, in microseconds
	uint16_t servo5_raw; ///< Servo output 5 value, in microseconds
	uint16_t servo6_raw; ///< Servo output 6 value, in microseconds
	uint16_t servo7_raw; ///< Servo output 7 value, in microseconds
	uint16_t servo8_raw; ///< Servo output 8 value, in microseconds

} mavlink_servo_output_raw_t;

/**
 * @brief Pack a servo_output_raw message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param servo1_raw Servo output 1 value, in microseconds
 * @param servo2_raw Servo output 2 value, in microseconds
 * @param servo3_raw Servo output 3 value, in microseconds
 * @param servo4_raw Servo output 4 value, in microseconds
 * @param servo5_raw Servo output 5 value, in microseconds
 * @param servo6_raw Servo output 6 value, in microseconds
 * @param servo7_raw Servo output 7 value, in microseconds
 * @param servo8_raw Servo output 8 value, in microseconds
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_servo_output_raw_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t servo1_raw, uint16_t servo2_raw, uint16_t servo3_raw, uint16_t servo4_raw, uint16_t servo5_raw, uint16_t servo6_raw, uint16_t servo7_raw, uint16_t servo8_raw)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SERVO_OUTPUT_RAW;

	p->servo1_raw = servo1_raw; // uint16_t:Servo output 1 value, in microseconds
	p->servo2_raw = servo2_raw; // uint16_t:Servo output 2 value, in microseconds
	p->servo3_raw = servo3_raw; // uint16_t:Servo output 3 value, in microseconds
	p->servo4_raw = servo4_raw; // uint16_t:Servo output 4 value, in microseconds
	p->servo5_raw = servo5_raw; // uint16_t:Servo output 5 value, in microseconds
	p->servo6_raw = servo6_raw; // uint16_t:Servo output 6 value, in microseconds
	p->servo7_raw = servo7_raw; // uint16_t:Servo output 7 value, in microseconds
	p->servo8_raw = servo8_raw; // uint16_t:Servo output 8 value, in microseconds

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SERVO_OUTPUT_RAW_LEN);
}

/**
 * @brief Pack a servo_output_raw message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param servo1_raw Servo output 1 value, in microseconds
 * @param servo2_raw Servo output 2 value, in microseconds
 * @param servo3_raw Servo output 3 value, in microseconds
 * @param servo4_raw Servo output 4 value, in microseconds
 * @param servo5_raw Servo output 5 value, in microseconds
 * @param servo6_raw Servo output 6 value, in microseconds
 * @param servo7_raw Servo output 7 value, in microseconds
 * @param servo8_raw Servo output 8 value, in microseconds
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_servo_output_raw_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t servo1_raw, uint16_t servo2_raw, uint16_t servo3_raw, uint16_t servo4_raw, uint16_t servo5_raw, uint16_t servo6_raw, uint16_t servo7_raw, uint16_t servo8_raw)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SERVO_OUTPUT_RAW;

	p->servo1_raw = servo1_raw; // uint16_t:Servo output 1 value, in microseconds
	p->servo2_raw = servo2_raw; // uint16_t:Servo output 2 value, in microseconds
	p->servo3_raw = servo3_raw; // uint16_t:Servo output 3 value, in microseconds
	p->servo4_raw = servo4_raw; // uint16_t:Servo output 4 value, in microseconds
	p->servo5_raw = servo5_raw; // uint16_t:Servo output 5 value, in microseconds
	p->servo6_raw = servo6_raw; // uint16_t:Servo output 6 value, in microseconds
	p->servo7_raw = servo7_raw; // uint16_t:Servo output 7 value, in microseconds
	p->servo8_raw = servo8_raw; // uint16_t:Servo output 8 value, in microseconds

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SERVO_OUTPUT_RAW_LEN);
}

/**
 * @brief Encode a servo_output_raw struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param servo_output_raw C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_servo_output_raw_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_servo_output_raw_t* servo_output_raw)
{
	return mavlink_msg_servo_output_raw_pack(system_id, component_id, msg, servo_output_raw->servo1_raw, servo_output_raw->servo2_raw, servo_output_raw->servo3_raw, servo_output_raw->servo4_raw, servo_output_raw->servo5_raw, servo_output_raw->servo6_raw, servo_output_raw->servo7_raw, servo_output_raw->servo8_raw);
}

/**
 * @brief Send a servo_output_raw message
 * @param chan MAVLink channel to send the message
 *
 * @param servo1_raw Servo output 1 value, in microseconds
 * @param servo2_raw Servo output 2 value, in microseconds
 * @param servo3_raw Servo output 3 value, in microseconds
 * @param servo4_raw Servo output 4 value, in microseconds
 * @param servo5_raw Servo output 5 value, in microseconds
 * @param servo6_raw Servo output 6 value, in microseconds
 * @param servo7_raw Servo output 7 value, in microseconds
 * @param servo8_raw Servo output 8 value, in microseconds
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_servo_output_raw_send(mavlink_channel_t chan, uint16_t servo1_raw, uint16_t servo2_raw, uint16_t servo3_raw, uint16_t servo4_raw, uint16_t servo5_raw, uint16_t servo6_raw, uint16_t servo7_raw, uint16_t servo8_raw)
{
	mavlink_header_t hdr;
	mavlink_servo_output_raw_t payload;
	uint16_t checksum;
	mavlink_servo_output_raw_t *p = &payload;

	p->servo1_raw = servo1_raw; // uint16_t:Servo output 1 value, in microseconds
	p->servo2_raw = servo2_raw; // uint16_t:Servo output 2 value, in microseconds
	p->servo3_raw = servo3_raw; // uint16_t:Servo output 3 value, in microseconds
	p->servo4_raw = servo4_raw; // uint16_t:Servo output 4 value, in microseconds
	p->servo5_raw = servo5_raw; // uint16_t:Servo output 5 value, in microseconds
	p->servo6_raw = servo6_raw; // uint16_t:Servo output 6 value, in microseconds
	p->servo7_raw = servo7_raw; // uint16_t:Servo output 7 value, in microseconds
	p->servo8_raw = servo8_raw; // uint16_t:Servo output 8 value, in microseconds

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SERVO_OUTPUT_RAW_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SERVO_OUTPUT_RAW;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&checksum);
	checksum = crc_calculate_mem((uint8_t *)&hdr.len, &checksum, MAVLINK_CORE_HEADER_LEN);
	checksum = crc_calculate_mem((uint8_t *)&payload, &checksum, hdr.len );
	hdr.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	hdr.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_mem(chan, (uint8_t *)&payload, hdr.len);
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck_a, MAVLINK_NUM_CHECKSUM_BYTES);
}

#endif
// MESSAGE SERVO_OUTPUT_RAW UNPACKING

/**
 * @brief Get field servo1_raw from servo_output_raw message
 *
 * @return Servo output 1 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo1_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo1_raw);
}

/**
 * @brief Get field servo2_raw from servo_output_raw message
 *
 * @return Servo output 2 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo2_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo2_raw);
}

/**
 * @brief Get field servo3_raw from servo_output_raw message
 *
 * @return Servo output 3 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo3_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo3_raw);
}

/**
 * @brief Get field servo4_raw from servo_output_raw message
 *
 * @return Servo output 4 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo4_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo4_raw);
}

/**
 * @brief Get field servo5_raw from servo_output_raw message
 *
 * @return Servo output 5 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo5_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo5_raw);
}

/**
 * @brief Get field servo6_raw from servo_output_raw message
 *
 * @return Servo output 6 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo6_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo6_raw);
}

/**
 * @brief Get field servo7_raw from servo_output_raw message
 *
 * @return Servo output 7 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo7_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo7_raw);
}

/**
 * @brief Get field servo8_raw from servo_output_raw message
 *
 * @return Servo output 8 value, in microseconds
 */
static inline uint16_t mavlink_msg_servo_output_raw_get_servo8_raw(const mavlink_message_t* msg)
{
	mavlink_servo_output_raw_t *p = (mavlink_servo_output_raw_t *)&msg->payload[0];
	return (uint16_t)(p->servo8_raw);
}

/**
 * @brief Decode a servo_output_raw message into a struct
 *
 * @param msg The message to decode
 * @param servo_output_raw C-struct to decode the message contents into
 */
static inline void mavlink_msg_servo_output_raw_decode(const mavlink_message_t* msg, mavlink_servo_output_raw_t* servo_output_raw)
{
	memcpy( servo_output_raw, msg->payload, sizeof(mavlink_servo_output_raw_t));
}
