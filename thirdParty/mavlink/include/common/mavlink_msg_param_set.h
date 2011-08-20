// MESSAGE PARAM_SET PACKING

#define MAVLINK_MSG_ID_PARAM_SET 23
#define MAVLINK_MSG_ID_PARAM_SET_LEN 23
#define MAVLINK_MSG_23_LEN 23
#define MAVLINK_MSG_ID_PARAM_SET_KEY 0x37
#define MAVLINK_MSG_23_KEY 0x37

typedef struct __mavlink_param_set_t 
{
	float param_value;	///< Onboard parameter value
	uint8_t target_system;	///< System ID
	uint8_t target_component;	///< Component ID
	char param_id[16];	///< Onboard parameter id
	uint8_t param_type;	///< Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

} mavlink_param_set_t;
#define MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN 16

/**
 * @brief Pack a param_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_set_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, const char* param_id, float param_value, uint8_t param_type)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PARAM_SET;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	memcpy(p->param_id, param_id, sizeof(p->param_id));	// char[16]:Onboard parameter id
	p->param_value = param_value;	// float:Onboard parameter value
	p->param_type = param_type;	// uint8_t:Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PARAM_SET_LEN);
}

/**
 * @brief Pack a param_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_set_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, const char* param_id, float param_value, uint8_t param_type)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PARAM_SET;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	memcpy(p->param_id, param_id, sizeof(p->param_id));	// char[16]:Onboard parameter id
	p->param_value = param_value;	// float:Onboard parameter value
	p->param_type = param_type;	// uint8_t:Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PARAM_SET_LEN);
}

/**
 * @brief Encode a param_set struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_set_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_set_t* param_set)
{
	return mavlink_msg_param_set_pack(system_id, component_id, msg, param_set->target_system, param_set->target_component, param_set->param_id, param_set->param_value, param_set->param_type);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a param_set message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_value Onboard parameter value
 * @param param_type Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 */
static inline void mavlink_msg_param_set_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, const char* param_id, float param_value, uint8_t param_type)
{
	mavlink_header_t hdr;
	mavlink_param_set_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_PARAM_SET_LEN )
	payload.target_system = target_system;	// uint8_t:System ID
	payload.target_component = target_component;	// uint8_t:Component ID
	memcpy(payload.param_id, param_id, sizeof(payload.param_id));	// char[16]:Onboard parameter id
	payload.param_value = param_value;	// float:Onboard parameter value
	payload.param_type = param_type;	// uint8_t:Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_PARAM_SET_LEN;
	hdr.msgid = MAVLINK_MSG_ID_PARAM_SET;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x37, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE PARAM_SET UNPACKING

/**
 * @brief Get field target_system from param_set message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_param_set_get_target_system(const mavlink_message_t* msg)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from param_set message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_param_set_get_target_component(const mavlink_message_t* msg)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field param_id from param_set message
 *
 * @return Onboard parameter id
 */
static inline uint16_t mavlink_msg_param_set_get_param_id(const mavlink_message_t* msg, char* param_id)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];

	memcpy(param_id, p->param_id, sizeof(p->param_id));
	return sizeof(p->param_id);
}

/**
 * @brief Get field param_value from param_set message
 *
 * @return Onboard parameter value
 */
static inline float mavlink_msg_param_set_get_param_value(const mavlink_message_t* msg)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];
	return (float)(p->param_value);
}

/**
 * @brief Get field param_type from param_set message
 *
 * @return Onboard parameter type: 0: float, 1: uint8_t, 2: int8_t, 3: uint16_t, 4: int16_t, 5: uint32_t, 6: int32_t
 */
static inline uint8_t mavlink_msg_param_set_get_param_type(const mavlink_message_t* msg)
{
	mavlink_param_set_t *p = (mavlink_param_set_t *)&msg->payload[0];
	return (uint8_t)(p->param_type);
}

/**
 * @brief Decode a param_set message into a struct
 *
 * @param msg The message to decode
 * @param param_set C-struct to decode the message contents into
 */
static inline void mavlink_msg_param_set_decode(const mavlink_message_t* msg, mavlink_param_set_t* param_set)
{
	memcpy( param_set, msg->payload, sizeof(mavlink_param_set_t));
}
