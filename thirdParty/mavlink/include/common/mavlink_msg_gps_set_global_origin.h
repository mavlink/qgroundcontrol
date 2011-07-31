// MESSAGE GPS_SET_GLOBAL_ORIGIN PACKING

#define MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN 48
#define MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN 14
#define MAVLINK_MSG_48_LEN 14

typedef struct __mavlink_gps_set_global_origin_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	int32_t latitude; ///< global position * 1E7
	int32_t longitude; ///< global position * 1E7
	int32_t altitude; ///< global position * 1000

} mavlink_gps_set_global_origin_t;

/**
 * @brief Pack a gps_set_global_origin message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param latitude global position * 1E7
 * @param longitude global position * 1E7
 * @param altitude global position * 1000
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_set_global_origin_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->latitude = latitude; // int32_t:global position * 1E7
	p->longitude = longitude; // int32_t:global position * 1E7
	p->altitude = altitude; // int32_t:global position * 1000

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN);
}

/**
 * @brief Pack a gps_set_global_origin message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param latitude global position * 1E7
 * @param longitude global position * 1E7
 * @param altitude global position * 1000
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_set_global_origin_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->latitude = latitude; // int32_t:global position * 1E7
	p->longitude = longitude; // int32_t:global position * 1E7
	p->altitude = altitude; // int32_t:global position * 1000

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN);
}

/**
 * @brief Encode a gps_set_global_origin struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gps_set_global_origin C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gps_set_global_origin_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gps_set_global_origin_t* gps_set_global_origin)
{
	return mavlink_msg_gps_set_global_origin_pack(system_id, component_id, msg, gps_set_global_origin->target_system, gps_set_global_origin->target_component, gps_set_global_origin->latitude, gps_set_global_origin->longitude, gps_set_global_origin->altitude);
}

/**
 * @brief Send a gps_set_global_origin message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param latitude global position * 1E7
 * @param longitude global position * 1E7
 * @param altitude global position * 1000
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_gps_set_global_origin_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg.payload[0];

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->latitude = latitude; // int32_t:global position * 1E7
	p->longitude = longitude; // int32_t:global position * 1E7
	p->altitude = altitude; // int32_t:global position * 1000

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN;
	msg.msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;
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
static inline void mavlink_msg_gps_set_global_origin_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_header_t hdr;
	mavlink_gps_set_global_origin_t payload;
	uint16_t checksum;
	mavlink_gps_set_global_origin_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->latitude = latitude; // int32_t:global position * 1E7
	p->longitude = longitude; // int32_t:global position * 1E7
	p->altitude = altitude; // int32_t:global position * 1000

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;
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
// MESSAGE GPS_SET_GLOBAL_ORIGIN UNPACKING

/**
 * @brief Get field target_system from gps_set_global_origin message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_gps_set_global_origin_get_target_system(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from gps_set_global_origin message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_gps_set_global_origin_get_target_component(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field latitude from gps_set_global_origin message
 *
 * @return global position * 1E7
 */
static inline int32_t mavlink_msg_gps_set_global_origin_get_latitude(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (int32_t)(p->latitude);
}

/**
 * @brief Get field longitude from gps_set_global_origin message
 *
 * @return global position * 1E7
 */
static inline int32_t mavlink_msg_gps_set_global_origin_get_longitude(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (int32_t)(p->longitude);
}

/**
 * @brief Get field altitude from gps_set_global_origin message
 *
 * @return global position * 1000
 */
static inline int32_t mavlink_msg_gps_set_global_origin_get_altitude(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (int32_t)(p->altitude);
}

/**
 * @brief Decode a gps_set_global_origin message into a struct
 *
 * @param msg The message to decode
 * @param gps_set_global_origin C-struct to decode the message contents into
 */
static inline void mavlink_msg_gps_set_global_origin_decode(const mavlink_message_t* msg, mavlink_gps_set_global_origin_t* gps_set_global_origin)
{
	memcpy( gps_set_global_origin, msg->payload, sizeof(mavlink_gps_set_global_origin_t));
}
