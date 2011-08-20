// MESSAGE CPU_LOAD PACKING

#define MAVLINK_MSG_ID_CPU_LOAD 170
#define MAVLINK_MSG_ID_CPU_LOAD_LEN 4
#define MAVLINK_MSG_170_LEN 4
#define MAVLINK_MSG_ID_CPU_LOAD_KEY 0xCA
#define MAVLINK_MSG_170_KEY 0xCA

typedef struct __mavlink_cpu_load_t 
{
	uint16_t batVolt;	///< Battery Voltage in millivolts
	uint8_t sensLoad;	///< Sensor DSC Load
	uint8_t ctrlLoad;	///< Control DSC Load

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

	p->sensLoad = sensLoad;	// uint8_t:Sensor DSC Load
	p->ctrlLoad = ctrlLoad;	// uint8_t:Control DSC Load
	p->batVolt = batVolt;	// uint16_t:Battery Voltage in millivolts

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

	p->sensLoad = sensLoad;	// uint8_t:Sensor DSC Load
	p->ctrlLoad = ctrlLoad;	// uint8_t:Control DSC Load
	p->batVolt = batVolt;	// uint16_t:Battery Voltage in millivolts

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


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a cpu_load message
 * @param chan MAVLink channel to send the message
 *
 * @param sensLoad Sensor DSC Load
 * @param ctrlLoad Control DSC Load
 * @param batVolt Battery Voltage in millivolts
 */
static inline void mavlink_msg_cpu_load_send(mavlink_channel_t chan, uint8_t sensLoad, uint8_t ctrlLoad, uint16_t batVolt)
{
	mavlink_header_t hdr;
	mavlink_cpu_load_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_CPU_LOAD_LEN )
	payload.sensLoad = sensLoad;	// uint8_t:Sensor DSC Load
	payload.ctrlLoad = ctrlLoad;	// uint8_t:Control DSC Load
	payload.batVolt = batVolt;	// uint16_t:Battery Voltage in millivolts

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_CPU_LOAD_LEN;
	hdr.msgid = MAVLINK_MSG_ID_CPU_LOAD;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xCA, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
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
