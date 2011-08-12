// MESSAGE NAV_FILTER_BIAS PACKING

#define MAVLINK_MSG_ID_NAV_FILTER_BIAS 220
#define MAVLINK_MSG_ID_NAV_FILTER_BIAS_LEN 32
#define MAVLINK_MSG_220_LEN 32

typedef struct __mavlink_nav_filter_bias_t 
{
	uint64_t usec; ///< Timestamp (microseconds)
	float accel_0; ///< b_f[0]
	float accel_1; ///< b_f[1]
	float accel_2; ///< b_f[2]
	float gyro_0; ///< b_f[0]
	float gyro_1; ///< b_f[1]
	float gyro_2; ///< b_f[2]

} mavlink_nav_filter_bias_t;

/**
 * @brief Pack a nav_filter_bias message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds)
 * @param accel_0 b_f[0]
 * @param accel_1 b_f[1]
 * @param accel_2 b_f[2]
 * @param gyro_0 b_f[0]
 * @param gyro_1 b_f[1]
 * @param gyro_2 b_f[2]
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_nav_filter_bias_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float accel_0, float accel_1, float accel_2, float gyro_0, float gyro_1, float gyro_2)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_NAV_FILTER_BIAS;

	p->usec = usec; // uint64_t:Timestamp (microseconds)
	p->accel_0 = accel_0; // float:b_f[0]
	p->accel_1 = accel_1; // float:b_f[1]
	p->accel_2 = accel_2; // float:b_f[2]
	p->gyro_0 = gyro_0; // float:b_f[0]
	p->gyro_1 = gyro_1; // float:b_f[1]
	p->gyro_2 = gyro_2; // float:b_f[2]

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NAV_FILTER_BIAS_LEN);
}

/**
 * @brief Pack a nav_filter_bias message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds)
 * @param accel_0 b_f[0]
 * @param accel_1 b_f[1]
 * @param accel_2 b_f[2]
 * @param gyro_0 b_f[0]
 * @param gyro_1 b_f[1]
 * @param gyro_2 b_f[2]
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_nav_filter_bias_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float accel_0, float accel_1, float accel_2, float gyro_0, float gyro_1, float gyro_2)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_NAV_FILTER_BIAS;

	p->usec = usec; // uint64_t:Timestamp (microseconds)
	p->accel_0 = accel_0; // float:b_f[0]
	p->accel_1 = accel_1; // float:b_f[1]
	p->accel_2 = accel_2; // float:b_f[2]
	p->gyro_0 = gyro_0; // float:b_f[0]
	p->gyro_1 = gyro_1; // float:b_f[1]
	p->gyro_2 = gyro_2; // float:b_f[2]

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NAV_FILTER_BIAS_LEN);
}

/**
 * @brief Encode a nav_filter_bias struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param nav_filter_bias C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_nav_filter_bias_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_nav_filter_bias_t* nav_filter_bias)
{
	return mavlink_msg_nav_filter_bias_pack(system_id, component_id, msg, nav_filter_bias->usec, nav_filter_bias->accel_0, nav_filter_bias->accel_1, nav_filter_bias->accel_2, nav_filter_bias->gyro_0, nav_filter_bias->gyro_1, nav_filter_bias->gyro_2);
}

/**
 * @brief Send a nav_filter_bias message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds)
 * @param accel_0 b_f[0]
 * @param accel_1 b_f[1]
 * @param accel_2 b_f[2]
 * @param gyro_0 b_f[0]
 * @param gyro_1 b_f[1]
 * @param gyro_2 b_f[2]
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_nav_filter_bias_send(mavlink_channel_t chan, uint64_t usec, float accel_0, float accel_1, float accel_2, float gyro_0, float gyro_1, float gyro_2)
{
	mavlink_header_t hdr;
	mavlink_nav_filter_bias_t payload;
	uint16_t checksum;
	mavlink_nav_filter_bias_t *p = &payload;

	p->usec = usec; // uint64_t:Timestamp (microseconds)
	p->accel_0 = accel_0; // float:b_f[0]
	p->accel_1 = accel_1; // float:b_f[1]
	p->accel_2 = accel_2; // float:b_f[2]
	p->gyro_0 = gyro_0; // float:b_f[0]
	p->gyro_1 = gyro_1; // float:b_f[1]
	p->gyro_2 = gyro_2; // float:b_f[2]

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_NAV_FILTER_BIAS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_NAV_FILTER_BIAS;
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
// MESSAGE NAV_FILTER_BIAS UNPACKING

/**
 * @brief Get field usec from nav_filter_bias message
 *
 * @return Timestamp (microseconds)
 */
static inline uint64_t mavlink_msg_nav_filter_bias_get_usec(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field accel_0 from nav_filter_bias message
 *
 * @return b_f[0]
 */
static inline float mavlink_msg_nav_filter_bias_get_accel_0(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (float)(p->accel_0);
}

/**
 * @brief Get field accel_1 from nav_filter_bias message
 *
 * @return b_f[1]
 */
static inline float mavlink_msg_nav_filter_bias_get_accel_1(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (float)(p->accel_1);
}

/**
 * @brief Get field accel_2 from nav_filter_bias message
 *
 * @return b_f[2]
 */
static inline float mavlink_msg_nav_filter_bias_get_accel_2(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (float)(p->accel_2);
}

/**
 * @brief Get field gyro_0 from nav_filter_bias message
 *
 * @return b_f[0]
 */
static inline float mavlink_msg_nav_filter_bias_get_gyro_0(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (float)(p->gyro_0);
}

/**
 * @brief Get field gyro_1 from nav_filter_bias message
 *
 * @return b_f[1]
 */
static inline float mavlink_msg_nav_filter_bias_get_gyro_1(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (float)(p->gyro_1);
}

/**
 * @brief Get field gyro_2 from nav_filter_bias message
 *
 * @return b_f[2]
 */
static inline float mavlink_msg_nav_filter_bias_get_gyro_2(const mavlink_message_t* msg)
{
	mavlink_nav_filter_bias_t *p = (mavlink_nav_filter_bias_t *)&msg->payload[0];
	return (float)(p->gyro_2);
}

/**
 * @brief Decode a nav_filter_bias message into a struct
 *
 * @param msg The message to decode
 * @param nav_filter_bias C-struct to decode the message contents into
 */
static inline void mavlink_msg_nav_filter_bias_decode(const mavlink_message_t* msg, mavlink_nav_filter_bias_t* nav_filter_bias)
{
	memcpy( nav_filter_bias, msg->payload, sizeof(mavlink_nav_filter_bias_t));
}
