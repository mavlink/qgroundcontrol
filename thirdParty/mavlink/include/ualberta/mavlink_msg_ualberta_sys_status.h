// MESSAGE UALBERTA_SYS_STATUS PACKING

#define MAVLINK_MSG_ID_UALBERTA_SYS_STATUS 222
#define MAVLINK_MSG_ID_UALBERTA_SYS_STATUS_LEN 3
#define MAVLINK_MSG_222_LEN 3

typedef struct __mavlink_ualberta_sys_status_t 
{
	uint8_t mode; ///< System mode, see UALBERTA_AUTOPILOT_MODE ENUM
	uint8_t nav_mode; ///< Navigation mode, see UALBERTA_NAV_MODE ENUM
	uint8_t pilot; ///< Pilot mode, see UALBERTA_PILOT_MODE

} mavlink_ualberta_sys_status_t;

/**
 * @brief Pack a ualberta_sys_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param mode System mode, see UALBERTA_AUTOPILOT_MODE ENUM
 * @param nav_mode Navigation mode, see UALBERTA_NAV_MODE ENUM
 * @param pilot Pilot mode, see UALBERTA_PILOT_MODE
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_ualberta_sys_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t mode, uint8_t nav_mode, uint8_t pilot)
{
	mavlink_ualberta_sys_status_t *p = (mavlink_ualberta_sys_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_UALBERTA_SYS_STATUS;

	p->mode = mode; // uint8_t:System mode, see UALBERTA_AUTOPILOT_MODE ENUM
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see UALBERTA_NAV_MODE ENUM
	p->pilot = pilot; // uint8_t:Pilot mode, see UALBERTA_PILOT_MODE

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_UALBERTA_SYS_STATUS_LEN);
}

/**
 * @brief Pack a ualberta_sys_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param mode System mode, see UALBERTA_AUTOPILOT_MODE ENUM
 * @param nav_mode Navigation mode, see UALBERTA_NAV_MODE ENUM
 * @param pilot Pilot mode, see UALBERTA_PILOT_MODE
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_ualberta_sys_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t mode, uint8_t nav_mode, uint8_t pilot)
{
	mavlink_ualberta_sys_status_t *p = (mavlink_ualberta_sys_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_UALBERTA_SYS_STATUS;

	p->mode = mode; // uint8_t:System mode, see UALBERTA_AUTOPILOT_MODE ENUM
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see UALBERTA_NAV_MODE ENUM
	p->pilot = pilot; // uint8_t:Pilot mode, see UALBERTA_PILOT_MODE

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_UALBERTA_SYS_STATUS_LEN);
}

/**
 * @brief Encode a ualberta_sys_status struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param ualberta_sys_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_ualberta_sys_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_ualberta_sys_status_t* ualberta_sys_status)
{
	return mavlink_msg_ualberta_sys_status_pack(system_id, component_id, msg, ualberta_sys_status->mode, ualberta_sys_status->nav_mode, ualberta_sys_status->pilot);
}

/**
 * @brief Send a ualberta_sys_status message
 * @param chan MAVLink channel to send the message
 *
 * @param mode System mode, see UALBERTA_AUTOPILOT_MODE ENUM
 * @param nav_mode Navigation mode, see UALBERTA_NAV_MODE ENUM
 * @param pilot Pilot mode, see UALBERTA_PILOT_MODE
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_ualberta_sys_status_send(mavlink_channel_t chan, uint8_t mode, uint8_t nav_mode, uint8_t pilot)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_ualberta_sys_status_t *p = (mavlink_ualberta_sys_status_t *)&msg.payload[0];

	p->mode = mode; // uint8_t:System mode, see UALBERTA_AUTOPILOT_MODE ENUM
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see UALBERTA_NAV_MODE ENUM
	p->pilot = pilot; // uint8_t:Pilot mode, see UALBERTA_PILOT_MODE

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_UALBERTA_SYS_STATUS_LEN;
	msg.msgid = MAVLINK_MSG_ID_UALBERTA_SYS_STATUS;
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
static inline void mavlink_msg_ualberta_sys_status_send(mavlink_channel_t chan, uint8_t mode, uint8_t nav_mode, uint8_t pilot)
{
	mavlink_header_t hdr;
	mavlink_ualberta_sys_status_t payload;
	uint16_t checksum;
	mavlink_ualberta_sys_status_t *p = &payload;

	p->mode = mode; // uint8_t:System mode, see UALBERTA_AUTOPILOT_MODE ENUM
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see UALBERTA_NAV_MODE ENUM
	p->pilot = pilot; // uint8_t:Pilot mode, see UALBERTA_PILOT_MODE

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_UALBERTA_SYS_STATUS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_UALBERTA_SYS_STATUS;
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
// MESSAGE UALBERTA_SYS_STATUS UNPACKING

/**
 * @brief Get field mode from ualberta_sys_status message
 *
 * @return System mode, see UALBERTA_AUTOPILOT_MODE ENUM
 */
static inline uint8_t mavlink_msg_ualberta_sys_status_get_mode(const mavlink_message_t* msg)
{
	mavlink_ualberta_sys_status_t *p = (mavlink_ualberta_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->mode);
}

/**
 * @brief Get field nav_mode from ualberta_sys_status message
 *
 * @return Navigation mode, see UALBERTA_NAV_MODE ENUM
 */
static inline uint8_t mavlink_msg_ualberta_sys_status_get_nav_mode(const mavlink_message_t* msg)
{
	mavlink_ualberta_sys_status_t *p = (mavlink_ualberta_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->nav_mode);
}

/**
 * @brief Get field pilot from ualberta_sys_status message
 *
 * @return Pilot mode, see UALBERTA_PILOT_MODE
 */
static inline uint8_t mavlink_msg_ualberta_sys_status_get_pilot(const mavlink_message_t* msg)
{
	mavlink_ualberta_sys_status_t *p = (mavlink_ualberta_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->pilot);
}

/**
 * @brief Decode a ualberta_sys_status message into a struct
 *
 * @param msg The message to decode
 * @param ualberta_sys_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_ualberta_sys_status_decode(const mavlink_message_t* msg, mavlink_ualberta_sys_status_t* ualberta_sys_status)
{
	memcpy( ualberta_sys_status, msg->payload, sizeof(mavlink_ualberta_sys_status_t));
}
