// MESSAGE SET_CAM_SHUTTER PACKING

#define MAVLINK_MSG_ID_SET_CAM_SHUTTER 100
#define MAVLINK_MSG_ID_SET_CAM_SHUTTER_LEN 11
#define MAVLINK_MSG_100_LEN 11

typedef struct __mavlink_set_cam_shutter_t 
{
	uint8_t cam_no; ///< Camera id
	uint8_t cam_mode; ///< Camera mode: 0 = auto, 1 = manual
	uint8_t trigger_pin; ///< Trigger pin, 0-3 for PtGrey FireFly
	uint16_t interval; ///< Shutter interval, in microseconds
	uint16_t exposure; ///< Exposure time, in microseconds
	float gain; ///< Camera gain

} mavlink_set_cam_shutter_t;

/**
 * @brief Pack a set_cam_shutter message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param cam_no Camera id
 * @param cam_mode Camera mode: 0 = auto, 1 = manual
 * @param trigger_pin Trigger pin, 0-3 for PtGrey FireFly
 * @param interval Shutter interval, in microseconds
 * @param exposure Exposure time, in microseconds
 * @param gain Camera gain
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_cam_shutter_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t cam_no, uint8_t cam_mode, uint8_t trigger_pin, uint16_t interval, uint16_t exposure, float gain)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_CAM_SHUTTER;

	p->cam_no = cam_no; // uint8_t:Camera id
	p->cam_mode = cam_mode; // uint8_t:Camera mode: 0 = auto, 1 = manual
	p->trigger_pin = trigger_pin; // uint8_t:Trigger pin, 0-3 for PtGrey FireFly
	p->interval = interval; // uint16_t:Shutter interval, in microseconds
	p->exposure = exposure; // uint16_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_CAM_SHUTTER_LEN);
}

/**
 * @brief Pack a set_cam_shutter message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param cam_no Camera id
 * @param cam_mode Camera mode: 0 = auto, 1 = manual
 * @param trigger_pin Trigger pin, 0-3 for PtGrey FireFly
 * @param interval Shutter interval, in microseconds
 * @param exposure Exposure time, in microseconds
 * @param gain Camera gain
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_cam_shutter_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t cam_no, uint8_t cam_mode, uint8_t trigger_pin, uint16_t interval, uint16_t exposure, float gain)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_CAM_SHUTTER;

	p->cam_no = cam_no; // uint8_t:Camera id
	p->cam_mode = cam_mode; // uint8_t:Camera mode: 0 = auto, 1 = manual
	p->trigger_pin = trigger_pin; // uint8_t:Trigger pin, 0-3 for PtGrey FireFly
	p->interval = interval; // uint16_t:Shutter interval, in microseconds
	p->exposure = exposure; // uint16_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_CAM_SHUTTER_LEN);
}

/**
 * @brief Encode a set_cam_shutter struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_cam_shutter C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_cam_shutter_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_cam_shutter_t* set_cam_shutter)
{
	return mavlink_msg_set_cam_shutter_pack(system_id, component_id, msg, set_cam_shutter->cam_no, set_cam_shutter->cam_mode, set_cam_shutter->trigger_pin, set_cam_shutter->interval, set_cam_shutter->exposure, set_cam_shutter->gain);
}

/**
 * @brief Send a set_cam_shutter message
 * @param chan MAVLink channel to send the message
 *
 * @param cam_no Camera id
 * @param cam_mode Camera mode: 0 = auto, 1 = manual
 * @param trigger_pin Trigger pin, 0-3 for PtGrey FireFly
 * @param interval Shutter interval, in microseconds
 * @param exposure Exposure time, in microseconds
 * @param gain Camera gain
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_set_cam_shutter_send(mavlink_channel_t chan, uint8_t cam_no, uint8_t cam_mode, uint8_t trigger_pin, uint16_t interval, uint16_t exposure, float gain)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg.payload[0];

	p->cam_no = cam_no; // uint8_t:Camera id
	p->cam_mode = cam_mode; // uint8_t:Camera mode: 0 = auto, 1 = manual
	p->trigger_pin = trigger_pin; // uint8_t:Trigger pin, 0-3 for PtGrey FireFly
	p->interval = interval; // uint16_t:Shutter interval, in microseconds
	p->exposure = exposure; // uint16_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_SET_CAM_SHUTTER_LEN;
	msg.msgid = MAVLINK_MSG_ID_SET_CAM_SHUTTER;
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
static inline void mavlink_msg_set_cam_shutter_send(mavlink_channel_t chan, uint8_t cam_no, uint8_t cam_mode, uint8_t trigger_pin, uint16_t interval, uint16_t exposure, float gain)
{
	mavlink_header_t hdr;
	mavlink_set_cam_shutter_t payload;
	uint16_t checksum;
	mavlink_set_cam_shutter_t *p = &payload;

	p->cam_no = cam_no; // uint8_t:Camera id
	p->cam_mode = cam_mode; // uint8_t:Camera mode: 0 = auto, 1 = manual
	p->trigger_pin = trigger_pin; // uint8_t:Trigger pin, 0-3 for PtGrey FireFly
	p->interval = interval; // uint16_t:Shutter interval, in microseconds
	p->exposure = exposure; // uint16_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_CAM_SHUTTER_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_CAM_SHUTTER;
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
// MESSAGE SET_CAM_SHUTTER UNPACKING

/**
 * @brief Get field cam_no from set_cam_shutter message
 *
 * @return Camera id
 */
static inline uint8_t mavlink_msg_set_cam_shutter_get_cam_no(const mavlink_message_t* msg)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	return (uint8_t)(p->cam_no);
}

/**
 * @brief Get field cam_mode from set_cam_shutter message
 *
 * @return Camera mode: 0 = auto, 1 = manual
 */
static inline uint8_t mavlink_msg_set_cam_shutter_get_cam_mode(const mavlink_message_t* msg)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	return (uint8_t)(p->cam_mode);
}

/**
 * @brief Get field trigger_pin from set_cam_shutter message
 *
 * @return Trigger pin, 0-3 for PtGrey FireFly
 */
static inline uint8_t mavlink_msg_set_cam_shutter_get_trigger_pin(const mavlink_message_t* msg)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	return (uint8_t)(p->trigger_pin);
}

/**
 * @brief Get field interval from set_cam_shutter message
 *
 * @return Shutter interval, in microseconds
 */
static inline uint16_t mavlink_msg_set_cam_shutter_get_interval(const mavlink_message_t* msg)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	return (uint16_t)(p->interval);
}

/**
 * @brief Get field exposure from set_cam_shutter message
 *
 * @return Exposure time, in microseconds
 */
static inline uint16_t mavlink_msg_set_cam_shutter_get_exposure(const mavlink_message_t* msg)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	return (uint16_t)(p->exposure);
}

/**
 * @brief Get field gain from set_cam_shutter message
 *
 * @return Camera gain
 */
static inline float mavlink_msg_set_cam_shutter_get_gain(const mavlink_message_t* msg)
{
	mavlink_set_cam_shutter_t *p = (mavlink_set_cam_shutter_t *)&msg->payload[0];
	return (float)(p->gain);
}

/**
 * @brief Decode a set_cam_shutter message into a struct
 *
 * @param msg The message to decode
 * @param set_cam_shutter C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_cam_shutter_decode(const mavlink_message_t* msg, mavlink_set_cam_shutter_t* set_cam_shutter)
{
	memcpy( set_cam_shutter, msg->payload, sizeof(mavlink_set_cam_shutter_t));
}
