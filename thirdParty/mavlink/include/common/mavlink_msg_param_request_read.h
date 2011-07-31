// MESSAGE PARAM_REQUEST_READ PACKING

#define MAVLINK_MSG_ID_PARAM_REQUEST_READ 20
#define MAVLINK_MSG_ID_PARAM_REQUEST_READ_LEN 19
#define MAVLINK_MSG_20_LEN 19

typedef struct __mavlink_param_request_read_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	char param_id[15]; ///< Onboard parameter id
	int16_t param_index; ///< Parameter index. Send -1 to use the param ID field as identifier

} mavlink_param_request_read_t;
#define MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN 15

/**
 * @brief Pack a param_request_read message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_request_read_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, const char* param_id, int16_t param_index)
{
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	memcpy(p->param_id, param_id, sizeof(p->param_id)); // char[15]:Onboard parameter id
	p->param_index = param_index; // int16_t:Parameter index. Send -1 to use the param ID field as identifier

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PARAM_REQUEST_READ_LEN);
}

/**
 * @brief Pack a param_request_read message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_request_read_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, const char* param_id, int16_t param_index)
{
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	memcpy(p->param_id, param_id, sizeof(p->param_id)); // char[15]:Onboard parameter id
	p->param_index = param_index; // int16_t:Parameter index. Send -1 to use the param ID field as identifier

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PARAM_REQUEST_READ_LEN);
}

/**
 * @brief Encode a param_request_read struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_request_read C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_request_read_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_request_read_t* param_request_read)
{
	return mavlink_msg_param_request_read_pack(system_id, component_id, msg, param_request_read->target_system, param_request_read->target_component, param_request_read->param_id, param_request_read->param_index);
}

/**
 * @brief Send a param_request_read message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param param_id Onboard parameter id
 * @param param_index Parameter index. Send -1 to use the param ID field as identifier
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_param_request_read_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, const char* param_id, int16_t param_index)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg.payload[0];

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	memcpy(p->param_id, param_id, sizeof(p->param_id)); // char[15]:Onboard parameter id
	p->param_index = param_index; // int16_t:Parameter index. Send -1 to use the param ID field as identifier

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_PARAM_REQUEST_READ_LEN;
	msg.msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;
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
static inline void mavlink_msg_param_request_read_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, const char* param_id, int16_t param_index)
{
	mavlink_header_t hdr;
	mavlink_param_request_read_t payload;
	uint16_t checksum;
	mavlink_param_request_read_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	memcpy(p->param_id, param_id, sizeof(p->param_id)); // char[15]:Onboard parameter id
	p->param_index = param_index; // int16_t:Parameter index. Send -1 to use the param ID field as identifier

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_PARAM_REQUEST_READ_LEN;
	hdr.msgid = MAVLINK_MSG_ID_PARAM_REQUEST_READ;
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
// MESSAGE PARAM_REQUEST_READ UNPACKING

/**
 * @brief Get field target_system from param_request_read message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_param_request_read_get_target_system(const mavlink_message_t* msg)
{
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from param_request_read message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_param_request_read_get_target_component(const mavlink_message_t* msg)
{
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field param_id from param_request_read message
 *
 * @return Onboard parameter id
 */
static inline uint16_t mavlink_msg_param_request_read_get_param_id(const mavlink_message_t* msg, char* param_id)
{
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg->payload[0];

	memcpy(param_id, p->param_id, sizeof(p->param_id));
	return sizeof(p->param_id);
}

/**
 * @brief Get field param_index from param_request_read message
 *
 * @return Parameter index. Send -1 to use the param ID field as identifier
 */
static inline int16_t mavlink_msg_param_request_read_get_param_index(const mavlink_message_t* msg)
{
	mavlink_param_request_read_t *p = (mavlink_param_request_read_t *)&msg->payload[0];
	return (int16_t)(p->param_index);
}

/**
 * @brief Decode a param_request_read message into a struct
 *
 * @param msg The message to decode
 * @param param_request_read C-struct to decode the message contents into
 */
static inline void mavlink_msg_param_request_read_decode(const mavlink_message_t* msg, mavlink_param_request_read_t* param_request_read)
{
	memcpy( param_request_read, msg->payload, sizeof(mavlink_param_request_read_t));
}
