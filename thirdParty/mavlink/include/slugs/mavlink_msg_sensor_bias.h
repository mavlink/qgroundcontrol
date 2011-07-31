// MESSAGE SENSOR_BIAS PACKING

#define MAVLINK_MSG_ID_SENSOR_BIAS 172
#define MAVLINK_MSG_ID_SENSOR_BIAS_LEN 24
#define MAVLINK_MSG_172_LEN 24

typedef struct __mavlink_sensor_bias_t 
{
	float axBias; ///< Accelerometer X bias (m/s)
	float ayBias; ///< Accelerometer Y bias (m/s)
	float azBias; ///< Accelerometer Z bias (m/s)
	float gxBias; ///< Gyro X bias (rad/s)
	float gyBias; ///< Gyro Y bias (rad/s)
	float gzBias; ///< Gyro Z bias (rad/s)

} mavlink_sensor_bias_t;

/**
 * @brief Pack a sensor_bias message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param axBias Accelerometer X bias (m/s)
 * @param ayBias Accelerometer Y bias (m/s)
 * @param azBias Accelerometer Z bias (m/s)
 * @param gxBias Gyro X bias (rad/s)
 * @param gyBias Gyro Y bias (rad/s)
 * @param gzBias Gyro Z bias (rad/s)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sensor_bias_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float axBias, float ayBias, float azBias, float gxBias, float gyBias, float gzBias)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SENSOR_BIAS;

	p->axBias = axBias; // float:Accelerometer X bias (m/s)
	p->ayBias = ayBias; // float:Accelerometer Y bias (m/s)
	p->azBias = azBias; // float:Accelerometer Z bias (m/s)
	p->gxBias = gxBias; // float:Gyro X bias (rad/s)
	p->gyBias = gyBias; // float:Gyro Y bias (rad/s)
	p->gzBias = gzBias; // float:Gyro Z bias (rad/s)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SENSOR_BIAS_LEN);
}

/**
 * @brief Pack a sensor_bias message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param axBias Accelerometer X bias (m/s)
 * @param ayBias Accelerometer Y bias (m/s)
 * @param azBias Accelerometer Z bias (m/s)
 * @param gxBias Gyro X bias (rad/s)
 * @param gyBias Gyro Y bias (rad/s)
 * @param gzBias Gyro Z bias (rad/s)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sensor_bias_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float axBias, float ayBias, float azBias, float gxBias, float gyBias, float gzBias)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SENSOR_BIAS;

	p->axBias = axBias; // float:Accelerometer X bias (m/s)
	p->ayBias = ayBias; // float:Accelerometer Y bias (m/s)
	p->azBias = azBias; // float:Accelerometer Z bias (m/s)
	p->gxBias = gxBias; // float:Gyro X bias (rad/s)
	p->gyBias = gyBias; // float:Gyro Y bias (rad/s)
	p->gzBias = gzBias; // float:Gyro Z bias (rad/s)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SENSOR_BIAS_LEN);
}

/**
 * @brief Encode a sensor_bias struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sensor_bias C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sensor_bias_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sensor_bias_t* sensor_bias)
{
	return mavlink_msg_sensor_bias_pack(system_id, component_id, msg, sensor_bias->axBias, sensor_bias->ayBias, sensor_bias->azBias, sensor_bias->gxBias, sensor_bias->gyBias, sensor_bias->gzBias);
}

/**
 * @brief Send a sensor_bias message
 * @param chan MAVLink channel to send the message
 *
 * @param axBias Accelerometer X bias (m/s)
 * @param ayBias Accelerometer Y bias (m/s)
 * @param azBias Accelerometer Z bias (m/s)
 * @param gxBias Gyro X bias (rad/s)
 * @param gyBias Gyro Y bias (rad/s)
 * @param gzBias Gyro Z bias (rad/s)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_sensor_bias_send(mavlink_channel_t chan, float axBias, float ayBias, float azBias, float gxBias, float gyBias, float gzBias)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg.payload[0];

	p->axBias = axBias; // float:Accelerometer X bias (m/s)
	p->ayBias = ayBias; // float:Accelerometer Y bias (m/s)
	p->azBias = azBias; // float:Accelerometer Z bias (m/s)
	p->gxBias = gxBias; // float:Gyro X bias (rad/s)
	p->gyBias = gyBias; // float:Gyro Y bias (rad/s)
	p->gzBias = gzBias; // float:Gyro Z bias (rad/s)

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_SENSOR_BIAS_LEN;
	msg.msgid = MAVLINK_MSG_ID_SENSOR_BIAS;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_sensor_bias_send(mavlink_channel_t chan, float axBias, float ayBias, float azBias, float gxBias, float gyBias, float gzBias)
{
	mavlink_header_t hdr;
	mavlink_sensor_bias_t payload;
	uint16_t checksum;
	mavlink_sensor_bias_t *p = &payload;

	p->axBias = axBias; // float:Accelerometer X bias (m/s)
	p->ayBias = ayBias; // float:Accelerometer Y bias (m/s)
	p->azBias = azBias; // float:Accelerometer Z bias (m/s)
	p->gxBias = gxBias; // float:Gyro X bias (rad/s)
	p->gyBias = gyBias; // float:Gyro Y bias (rad/s)
	p->gzBias = gzBias; // float:Gyro Z bias (rad/s)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SENSOR_BIAS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SENSOR_BIAS;
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
// MESSAGE SENSOR_BIAS UNPACKING

/**
 * @brief Get field axBias from sensor_bias message
 *
 * @return Accelerometer X bias (m/s)
 */
static inline float mavlink_msg_sensor_bias_get_axBias(const mavlink_message_t* msg)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	return (float)(p->axBias);
}

/**
 * @brief Get field ayBias from sensor_bias message
 *
 * @return Accelerometer Y bias (m/s)
 */
static inline float mavlink_msg_sensor_bias_get_ayBias(const mavlink_message_t* msg)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	return (float)(p->ayBias);
}

/**
 * @brief Get field azBias from sensor_bias message
 *
 * @return Accelerometer Z bias (m/s)
 */
static inline float mavlink_msg_sensor_bias_get_azBias(const mavlink_message_t* msg)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	return (float)(p->azBias);
}

/**
 * @brief Get field gxBias from sensor_bias message
 *
 * @return Gyro X bias (rad/s)
 */
static inline float mavlink_msg_sensor_bias_get_gxBias(const mavlink_message_t* msg)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	return (float)(p->gxBias);
}

/**
 * @brief Get field gyBias from sensor_bias message
 *
 * @return Gyro Y bias (rad/s)
 */
static inline float mavlink_msg_sensor_bias_get_gyBias(const mavlink_message_t* msg)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	return (float)(p->gyBias);
}

/**
 * @brief Get field gzBias from sensor_bias message
 *
 * @return Gyro Z bias (rad/s)
 */
static inline float mavlink_msg_sensor_bias_get_gzBias(const mavlink_message_t* msg)
{
	mavlink_sensor_bias_t *p = (mavlink_sensor_bias_t *)&msg->payload[0];
	return (float)(p->gzBias);
}

/**
 * @brief Decode a sensor_bias message into a struct
 *
 * @param msg The message to decode
 * @param sensor_bias C-struct to decode the message contents into
 */
static inline void mavlink_msg_sensor_bias_decode(const mavlink_message_t* msg, mavlink_sensor_bias_t* sensor_bias)
{
	memcpy( sensor_bias, msg->payload, sizeof(mavlink_sensor_bias_t));
}
