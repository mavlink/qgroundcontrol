// MESSAGE CPU_LOAD PACKING

#define MAVLINK_MSG_ID_CPU_LOAD 170
#define MAVLINK_MSG_ID_CPU_LOAD_LEN 4
#define MAVLINK_MSG_170_LEN 4

typedef struct __mavlink_cpu_load_t 
{
	uint8_t sensLoad; ///< Sensor DSC Load
	uint8_t ctrlLoad; ///< Control DSC Load
	uint16_t batVolt; ///< Battery Voltage in millivolts

} mavlink_cpu_load_t;

/**
 * @brief Pack a cpu_load message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param sensLoad Sensor DSC Load
 * @param ctrlLoad Control DSC Load
 * @param batVolt Battery Voltage in millivolts
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cpu_load_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t sensLoad, uint8_t ctrlLoad, uint16_t batVolt)
{
	mavlink_cpu_load_t *p = (mavlink_cpu_load_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CPU_LOAD;

	p->sensLoad = sensLoad; // uint8_t:Sensor DSC Load
	p->ctrlLoad = ctrlLoad; // uint8_t:Control DSC Load
	p->batVolt = batVolt; // uint16_t:Battery Voltage in millivolts

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_CPU_LOAD_LEN);
}

/**
 * @brief Pack a cpu_load message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param sensLoad Sensor DSC Load
 * @param ctrlLoad Control DSC Load
 * @param batVolt Battery Voltage in millivolts
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cpu_load_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t sensLoad, uint8_t ctrlLoad, uint16_t batVolt)
{
	mavlink_cpu_load_t *p = (mavlink_cpu_load_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CPU_LOAD;

	p->sensLoad = sensLoad; // uint8_t:Sensor DSC Load
	p->ctrlLoad = ctrlLoad; // uint8_t:Control DSC Load
	p->batVolt = batVolt; // uint16_t:Battery Voltage in millivolts

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_CPU_LOAD_LEN);
}

/**
 * @brief Encode a cpu_load struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param cpu_load C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_cpu_load_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_cpu_load_t* cpu_load)
{
	return mavlink_msg_cpu_load_pack(system_id, component_id, msg, cpu_load->sensLoad, cpu_load->ctrlLoad, cpu_load->batVolt);
}

/**
 * @brief Send a cpu_load message
 * @param chan MAVLink channel to send the message
 *
 * @param sensLoad Sensor DSC Load
 * @param ctrlLoad Control DSC Load
 * @param batVolt Battery Voltage in millivolts
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_cpu_load_send(mavlink_channel_t chan, uint8_t sensLoad, uint8_t ctrlLoad, uint16_t batVolt)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_cpu_load_t *p = (mavlink_cpu_load_t *)&msg.payload[0];

	p->sensLoad = sensLoad; // uint8_t:Sensor DSC Load
	p->ctrlLoad = ctrlLoad; // uint8_t:Control DSC Load
	p->batVolt = batVolt; // uint16_t:Battery Voltage in millivolts

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_CPU_LOAD_LEN;
	msg.msgid = MAVLINK_MSG_ID_CPU_LOAD;
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
static inline void mavlink_msg_cpu_load_send(mavlink_channel_t chan, uint8_t sensLoad, uint8_t ctrlLoad, uint16_t batVolt)
{
	mavlink_header_t hdr;
	mavlink_cpu_load_t payload;
	uint16_t checksum;
	mavlink_cpu_load_t *p = &payload;

	p->sensLoad = sensLoad; // uint8_t:Sensor DSC Load
	p->ctrlLoad = ctrlLoad; // uint8_t:Control DSC Load
	p->batVolt = batVolt; // uint16_t:Battery Voltage in millivolts

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_CPU_LOAD_LEN;
	hdr.msgid = MAVLINK_MSG_ID_CPU_LOAD;
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
// MESSAGE CPU_LOAD UNPACKING

/**
 * @brief Get field sensLoad from cpu_load message
 *
 * @return Sensor DSC Load
 */
static inline uint8_t mavlink_msg_cpu_load_get_sensLoad(const mavlink_message_t* msg)
{
	mavlink_cpu_load_t *p = (mavlink_cpu_load_t *)&msg->payload[0];
	return (uint8_t)(p->sensLoad);
}

/**
 * @brief Get field ctrlLoad from cpu_load message
 *
 * @return Control DSC Load
 */
static inline uint8_t mavlink_msg_cpu_load_get_ctrlLoad(const mavlink_message_t* msg)
{
	mavlink_cpu_load_t *p = (mavlink_cpu_load_t *)&msg->payload[0];
	return (uint8_t)(p->ctrlLoad);
}

/**
 * @brief Get field batVolt from cpu_load message
 *
 * @return Battery Voltage in millivolts
 */
static inline uint16_t mavlink_msg_cpu_load_get_batVolt(const mavlink_message_t* msg)
{
	mavlink_cpu_load_t *p = (mavlink_cpu_load_t *)&msg->payload[0];
	return (uint16_t)(p->batVolt);
}

/**
 * @brief Decode a cpu_load message into a struct
 *
 * @param msg The message to decode
 * @param cpu_load C-struct to decode the message contents into
 */
static inline void mavlink_msg_cpu_load_decode(const mavlink_message_t* msg, mavlink_cpu_load_t* cpu_load)
{
	memcpy( cpu_load, msg->payload, sizeof(mavlink_cpu_load_t));
}
