// MESSAGE OPTICAL_FLOW PACKING

#define MAVLINK_MSG_ID_OPTICAL_FLOW 100
#define MAVLINK_MSG_ID_OPTICAL_FLOW_LEN 18
#define MAVLINK_MSG_100_LEN 18
#define MAVLINK_MSG_ID_OPTICAL_FLOW_KEY 0x4A
#define MAVLINK_MSG_100_KEY 0x4A

typedef struct __mavlink_optical_flow_t 
{
	uint64_t time;	///< Timestamp (UNIX)
	float ground_distance;	///< Ground distance in meters
	int16_t flow_x;	///< Flow in pixels in x-sensor direction
	int16_t flow_y;	///< Flow in pixels in y-sensor direction
	uint8_t sensor_id;	///< Sensor ID
	uint8_t quality;	///< Optical flow quality / confidence. 0: bad, 255: maximum quality

} mavlink_optical_flow_t;

/**
 * @brief Pack a optical_flow message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_optical_flow_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	p->time = time;	// uint64_t:Timestamp (UNIX)
	p->sensor_id = sensor_id;	// uint8_t:Sensor ID
	p->flow_x = flow_x;	// int16_t:Flow in pixels in x-sensor direction
	p->flow_y = flow_y;	// int16_t:Flow in pixels in y-sensor direction
	p->quality = quality;	// uint8_t:Optical flow quality / confidence. 0: bad, 255: maximum quality
	p->ground_distance = ground_distance;	// float:Ground distance in meters

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_OPTICAL_FLOW_LEN);
}

/**
 * @brief Pack a optical_flow message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_optical_flow_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	p->time = time;	// uint64_t:Timestamp (UNIX)
	p->sensor_id = sensor_id;	// uint8_t:Sensor ID
	p->flow_x = flow_x;	// int16_t:Flow in pixels in x-sensor direction
	p->flow_y = flow_y;	// int16_t:Flow in pixels in y-sensor direction
	p->quality = quality;	// uint8_t:Optical flow quality / confidence. 0: bad, 255: maximum quality
	p->ground_distance = ground_distance;	// float:Ground distance in meters

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_OPTICAL_FLOW_LEN);
}

/**
 * @brief Encode a optical_flow struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param optical_flow C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_optical_flow_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_optical_flow_t* optical_flow)
{
	return mavlink_msg_optical_flow_pack(system_id, component_id, msg, optical_flow->time, optical_flow->sensor_id, optical_flow->flow_x, optical_flow->flow_y, optical_flow->quality, optical_flow->ground_distance);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a optical_flow message
 * @param chan MAVLink channel to send the message
 *
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 */
static inline void mavlink_msg_optical_flow_send(mavlink_channel_t chan, uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	mavlink_header_t hdr;
	mavlink_optical_flow_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_OPTICAL_FLOW_LEN )
	payload.time = time;	// uint64_t:Timestamp (UNIX)
	payload.sensor_id = sensor_id;	// uint8_t:Sensor ID
	payload.flow_x = flow_x;	// int16_t:Flow in pixels in x-sensor direction
	payload.flow_y = flow_y;	// int16_t:Flow in pixels in y-sensor direction
	payload.quality = quality;	// uint8_t:Optical flow quality / confidence. 0: bad, 255: maximum quality
	payload.ground_distance = ground_distance;	// float:Ground distance in meters

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_OPTICAL_FLOW_LEN;
	hdr.msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x4A, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE OPTICAL_FLOW UNPACKING

/**
 * @brief Get field time from optical_flow message
 *
 * @return Timestamp (UNIX)
 */
static inline uint64_t mavlink_msg_optical_flow_get_time(const mavlink_message_t* msg)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	return (uint64_t)(p->time);
}

/**
 * @brief Get field sensor_id from optical_flow message
 *
 * @return Sensor ID
 */
static inline uint8_t mavlink_msg_optical_flow_get_sensor_id(const mavlink_message_t* msg)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	return (uint8_t)(p->sensor_id);
}

/**
 * @brief Get field flow_x from optical_flow message
 *
 * @return Flow in pixels in x-sensor direction
 */
static inline int16_t mavlink_msg_optical_flow_get_flow_x(const mavlink_message_t* msg)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	return (int16_t)(p->flow_x);
}

/**
 * @brief Get field flow_y from optical_flow message
 *
 * @return Flow in pixels in y-sensor direction
 */
static inline int16_t mavlink_msg_optical_flow_get_flow_y(const mavlink_message_t* msg)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	return (int16_t)(p->flow_y);
}

/**
 * @brief Get field quality from optical_flow message
 *
 * @return Optical flow quality / confidence. 0: bad, 255: maximum quality
 */
static inline uint8_t mavlink_msg_optical_flow_get_quality(const mavlink_message_t* msg)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	return (uint8_t)(p->quality);
}

/**
 * @brief Get field ground_distance from optical_flow message
 *
 * @return Ground distance in meters
 */
static inline float mavlink_msg_optical_flow_get_ground_distance(const mavlink_message_t* msg)
{
	mavlink_optical_flow_t *p = (mavlink_optical_flow_t *)&msg->payload[0];
	return (float)(p->ground_distance);
}

/**
 * @brief Decode a optical_flow message into a struct
 *
 * @param msg The message to decode
 * @param optical_flow C-struct to decode the message contents into
 */
static inline void mavlink_msg_optical_flow_decode(const mavlink_message_t* msg, mavlink_optical_flow_t* optical_flow)
{
	memcpy( optical_flow, msg->payload, sizeof(mavlink_optical_flow_t));
}
