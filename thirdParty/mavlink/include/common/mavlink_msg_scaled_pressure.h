// MESSAGE SCALED_PRESSURE PACKING

#define MAVLINK_MSG_ID_SCALED_PRESSURE 38
#define MAVLINK_MSG_ID_SCALED_PRESSURE_LEN 18
#define MAVLINK_MSG_38_LEN 18

typedef struct __mavlink_scaled_pressure_t 
{
	uint64_t usec; ///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	float press_abs; ///< Absolute pressure (hectopascal)
	float press_diff; ///< Differential pressure 1 (hectopascal)
	int16_t temperature; ///< Temperature measurement (0.01 degrees celsius)

} mavlink_scaled_pressure_t;

/**
 * @brief Pack a scaled_pressure message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param press_abs Absolute pressure (hectopascal)
 * @param press_diff Differential pressure 1 (hectopascal)
 * @param temperature Temperature measurement (0.01 degrees celsius)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_scaled_pressure_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float press_abs, float press_diff, int16_t temperature)
{
	mavlink_scaled_pressure_t *p = (mavlink_scaled_pressure_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SCALED_PRESSURE;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->press_abs = press_abs; // float:Absolute pressure (hectopascal)
	p->press_diff = press_diff; // float:Differential pressure 1 (hectopascal)
	p->temperature = temperature; // int16_t:Temperature measurement (0.01 degrees celsius)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SCALED_PRESSURE_LEN);
}

/**
 * @brief Pack a scaled_pressure message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param press_abs Absolute pressure (hectopascal)
 * @param press_diff Differential pressure 1 (hectopascal)
 * @param temperature Temperature measurement (0.01 degrees celsius)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_scaled_pressure_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float press_abs, float press_diff, int16_t temperature)
{
	mavlink_scaled_pressure_t *p = (mavlink_scaled_pressure_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SCALED_PRESSURE;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->press_abs = press_abs; // float:Absolute pressure (hectopascal)
	p->press_diff = press_diff; // float:Differential pressure 1 (hectopascal)
	p->temperature = temperature; // int16_t:Temperature measurement (0.01 degrees celsius)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SCALED_PRESSURE_LEN);
}

/**
 * @brief Encode a scaled_pressure struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param scaled_pressure C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_scaled_pressure_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_scaled_pressure_t* scaled_pressure)
{
	return mavlink_msg_scaled_pressure_pack(system_id, component_id, msg, scaled_pressure->usec, scaled_pressure->press_abs, scaled_pressure->press_diff, scaled_pressure->temperature);
}

/**
 * @brief Send a scaled_pressure message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param press_abs Absolute pressure (hectopascal)
 * @param press_diff Differential pressure 1 (hectopascal)
 * @param temperature Temperature measurement (0.01 degrees celsius)
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_scaled_pressure_send(mavlink_channel_t chan, uint64_t usec, float press_abs, float press_diff, int16_t temperature)
{
	mavlink_header_t hdr;
	mavlink_scaled_pressure_t payload;
	uint16_t checksum;
	mavlink_scaled_pressure_t *p = &payload;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->press_abs = press_abs; // float:Absolute pressure (hectopascal)
	p->press_diff = press_diff; // float:Differential pressure 1 (hectopascal)
	p->temperature = temperature; // int16_t:Temperature measurement (0.01 degrees celsius)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SCALED_PRESSURE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SCALED_PRESSURE;
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
// MESSAGE SCALED_PRESSURE UNPACKING

/**
 * @brief Get field usec from scaled_pressure message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_scaled_pressure_get_usec(const mavlink_message_t* msg)
{
	mavlink_scaled_pressure_t *p = (mavlink_scaled_pressure_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field press_abs from scaled_pressure message
 *
 * @return Absolute pressure (hectopascal)
 */
static inline float mavlink_msg_scaled_pressure_get_press_abs(const mavlink_message_t* msg)
{
	mavlink_scaled_pressure_t *p = (mavlink_scaled_pressure_t *)&msg->payload[0];
	return (float)(p->press_abs);
}

/**
 * @brief Get field press_diff from scaled_pressure message
 *
 * @return Differential pressure 1 (hectopascal)
 */
static inline float mavlink_msg_scaled_pressure_get_press_diff(const mavlink_message_t* msg)
{
	mavlink_scaled_pressure_t *p = (mavlink_scaled_pressure_t *)&msg->payload[0];
	return (float)(p->press_diff);
}

/**
 * @brief Get field temperature from scaled_pressure message
 *
 * @return Temperature measurement (0.01 degrees celsius)
 */
static inline int16_t mavlink_msg_scaled_pressure_get_temperature(const mavlink_message_t* msg)
{
	mavlink_scaled_pressure_t *p = (mavlink_scaled_pressure_t *)&msg->payload[0];
	return (int16_t)(p->temperature);
}

/**
 * @brief Decode a scaled_pressure message into a struct
 *
 * @param msg The message to decode
 * @param scaled_pressure C-struct to decode the message contents into
 */
static inline void mavlink_msg_scaled_pressure_decode(const mavlink_message_t* msg, mavlink_scaled_pressure_t* scaled_pressure)
{
	memcpy( scaled_pressure, msg->payload, sizeof(mavlink_scaled_pressure_t));
}
