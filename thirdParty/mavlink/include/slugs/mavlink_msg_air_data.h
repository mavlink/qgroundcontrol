// MESSAGE AIR_DATA PACKING

#define MAVLINK_MSG_ID_AIR_DATA 171
#define MAVLINK_MSG_ID_AIR_DATA_LEN 10
#define MAVLINK_MSG_171_LEN 10
#define MAVLINK_MSG_ID_AIR_DATA_KEY 0x90
#define MAVLINK_MSG_171_KEY 0x90

typedef struct __mavlink_air_data_t 
{
	float dynamicPressure;	///< Dynamic pressure (Pa)
	float staticPressure;	///< Static pressure (Pa)
	uint16_t temperature;	///< Board temperature

} mavlink_air_data_t;

/**
 * @brief Pack a air_data message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param dynamicPressure Dynamic pressure (Pa)
 * @param staticPressure Static pressure (Pa)
 * @param temperature Board temperature
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_air_data_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float dynamicPressure, float staticPressure, uint16_t temperature)
{
	mavlink_air_data_t *p = (mavlink_air_data_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_AIR_DATA;

	p->dynamicPressure = dynamicPressure;	// float:Dynamic pressure (Pa)
	p->staticPressure = staticPressure;	// float:Static pressure (Pa)
	p->temperature = temperature;	// uint16_t:Board temperature

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_AIR_DATA_LEN);
}

/**
 * @brief Pack a air_data message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param dynamicPressure Dynamic pressure (Pa)
 * @param staticPressure Static pressure (Pa)
 * @param temperature Board temperature
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_air_data_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float dynamicPressure, float staticPressure, uint16_t temperature)
{
	mavlink_air_data_t *p = (mavlink_air_data_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_AIR_DATA;

	p->dynamicPressure = dynamicPressure;	// float:Dynamic pressure (Pa)
	p->staticPressure = staticPressure;	// float:Static pressure (Pa)
	p->temperature = temperature;	// uint16_t:Board temperature

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_AIR_DATA_LEN);
}

/**
 * @brief Encode a air_data struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param air_data C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_air_data_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_air_data_t* air_data)
{
	return mavlink_msg_air_data_pack(system_id, component_id, msg, air_data->dynamicPressure, air_data->staticPressure, air_data->temperature);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a air_data message
 * @param chan MAVLink channel to send the message
 *
 * @param dynamicPressure Dynamic pressure (Pa)
 * @param staticPressure Static pressure (Pa)
 * @param temperature Board temperature
 */
static inline void mavlink_msg_air_data_send(mavlink_channel_t chan, float dynamicPressure, float staticPressure, uint16_t temperature)
{
	mavlink_header_t hdr;
	mavlink_air_data_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_AIR_DATA_LEN )
	payload.dynamicPressure = dynamicPressure;	// float:Dynamic pressure (Pa)
	payload.staticPressure = staticPressure;	// float:Static pressure (Pa)
	payload.temperature = temperature;	// uint16_t:Board temperature

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_AIR_DATA_LEN;
	hdr.msgid = MAVLINK_MSG_ID_AIR_DATA;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x90, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE AIR_DATA UNPACKING

/**
 * @brief Get field dynamicPressure from air_data message
 *
 * @return Dynamic pressure (Pa)
 */
static inline float mavlink_msg_air_data_get_dynamicPressure(const mavlink_message_t* msg)
{
	mavlink_air_data_t *p = (mavlink_air_data_t *)&msg->payload[0];
	return (float)(p->dynamicPressure);
}

/**
 * @brief Get field staticPressure from air_data message
 *
 * @return Static pressure (Pa)
 */
static inline float mavlink_msg_air_data_get_staticPressure(const mavlink_message_t* msg)
{
	mavlink_air_data_t *p = (mavlink_air_data_t *)&msg->payload[0];
	return (float)(p->staticPressure);
}

/**
 * @brief Get field temperature from air_data message
 *
 * @return Board temperature
 */
static inline uint16_t mavlink_msg_air_data_get_temperature(const mavlink_message_t* msg)
{
	mavlink_air_data_t *p = (mavlink_air_data_t *)&msg->payload[0];
	return (uint16_t)(p->temperature);
}

/**
 * @brief Decode a air_data message into a struct
 *
 * @param msg The message to decode
 * @param air_data C-struct to decode the message contents into
 */
static inline void mavlink_msg_air_data_decode(const mavlink_message_t* msg, mavlink_air_data_t* air_data)
{
	memcpy( air_data, msg->payload, sizeof(mavlink_air_data_t));
}
