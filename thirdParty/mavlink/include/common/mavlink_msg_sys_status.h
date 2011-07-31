// MESSAGE SYS_STATUS PACKING

#define MAVLINK_MSG_ID_SYS_STATUS 34
#define MAVLINK_MSG_ID_SYS_STATUS_LEN 11
#define MAVLINK_MSG_34_LEN 11

typedef struct __mavlink_sys_status_t 
{
	uint8_t mode; ///< System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	uint8_t nav_mode; ///< Navigation mode, see MAV_NAV_MODE ENUM
	uint8_t status; ///< System status flag, see MAV_STATUS ENUM
	uint16_t load; ///< Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	uint16_t vbat; ///< Battery voltage, in millivolts (1 = 1 millivolt)
	uint16_t battery_remaining; ///< Remaining battery energy: (0%: 0, 100%: 1000)
	uint16_t packet_drop; ///< Dropped packets (packets that were corrupted on reception on the MAV)

} mavlink_sys_status_t;

/**
 * @brief Pack a sys_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param mode System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 * @param nav_mode Navigation mode, see MAV_NAV_MODE ENUM
 * @param status System status flag, see MAV_STATUS ENUM
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param vbat Battery voltage, in millivolts (1 = 1 millivolt)
 * @param battery_remaining Remaining battery energy: (0%: 0, 100%: 1000)
 * @param packet_drop Dropped packets (packets that were corrupted on reception on the MAV)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t mode, uint8_t nav_mode, uint8_t status, uint16_t load, uint16_t vbat, uint16_t battery_remaining, uint16_t packet_drop)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	p->mode = mode; // uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see MAV_NAV_MODE ENUM
	p->status = status; // uint8_t:System status flag, see MAV_STATUS ENUM
	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->vbat = vbat; // uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	p->battery_remaining = battery_remaining; // uint16_t:Remaining battery energy: (0%: 0, 100%: 1000)
	p->packet_drop = packet_drop; // uint16_t:Dropped packets (packets that were corrupted on reception on the MAV)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYS_STATUS_LEN);
}

/**
 * @brief Pack a sys_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param mode System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 * @param nav_mode Navigation mode, see MAV_NAV_MODE ENUM
 * @param status System status flag, see MAV_STATUS ENUM
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param vbat Battery voltage, in millivolts (1 = 1 millivolt)
 * @param battery_remaining Remaining battery energy: (0%: 0, 100%: 1000)
 * @param packet_drop Dropped packets (packets that were corrupted on reception on the MAV)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t mode, uint8_t nav_mode, uint8_t status, uint16_t load, uint16_t vbat, uint16_t battery_remaining, uint16_t packet_drop)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	p->mode = mode; // uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see MAV_NAV_MODE ENUM
	p->status = status; // uint8_t:System status flag, see MAV_STATUS ENUM
	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->vbat = vbat; // uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	p->battery_remaining = battery_remaining; // uint16_t:Remaining battery energy: (0%: 0, 100%: 1000)
	p->packet_drop = packet_drop; // uint16_t:Dropped packets (packets that were corrupted on reception on the MAV)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYS_STATUS_LEN);
}

/**
 * @brief Encode a sys_status struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sys_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sys_status_t* sys_status)
{
	return mavlink_msg_sys_status_pack(system_id, component_id, msg, sys_status->mode, sys_status->nav_mode, sys_status->status, sys_status->load, sys_status->vbat, sys_status->battery_remaining, sys_status->packet_drop);
}

/**
 * @brief Send a sys_status message
 * @param chan MAVLink channel to send the message
 *
 * @param mode System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 * @param nav_mode Navigation mode, see MAV_NAV_MODE ENUM
 * @param status System status flag, see MAV_STATUS ENUM
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param vbat Battery voltage, in millivolts (1 = 1 millivolt)
 * @param battery_remaining Remaining battery energy: (0%: 0, 100%: 1000)
 * @param packet_drop Dropped packets (packets that were corrupted on reception on the MAV)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_sys_status_send(mavlink_channel_t chan, uint8_t mode, uint8_t nav_mode, uint8_t status, uint16_t load, uint16_t vbat, uint16_t battery_remaining, uint16_t packet_drop)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg.payload[0];

	p->mode = mode; // uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see MAV_NAV_MODE ENUM
	p->status = status; // uint8_t:System status flag, see MAV_STATUS ENUM
	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->vbat = vbat; // uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	p->battery_remaining = battery_remaining; // uint16_t:Remaining battery energy: (0%: 0, 100%: 1000)
	p->packet_drop = packet_drop; // uint16_t:Dropped packets (packets that were corrupted on reception on the MAV)

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_SYS_STATUS_LEN;
	msg.msgid = MAVLINK_MSG_ID_SYS_STATUS;
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
static inline void mavlink_msg_sys_status_send(mavlink_channel_t chan, uint8_t mode, uint8_t nav_mode, uint8_t status, uint16_t load, uint16_t vbat, uint16_t battery_remaining, uint16_t packet_drop)
{
	mavlink_header_t hdr;
	mavlink_sys_status_t payload;
	uint16_t checksum;
	mavlink_sys_status_t *p = &payload;

	p->mode = mode; // uint8_t:System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
	p->nav_mode = nav_mode; // uint8_t:Navigation mode, see MAV_NAV_MODE ENUM
	p->status = status; // uint8_t:System status flag, see MAV_STATUS ENUM
	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->vbat = vbat; // uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	p->battery_remaining = battery_remaining; // uint16_t:Remaining battery energy: (0%: 0, 100%: 1000)
	p->packet_drop = packet_drop; // uint16_t:Dropped packets (packets that were corrupted on reception on the MAV)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SYS_STATUS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SYS_STATUS;
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
// MESSAGE SYS_STATUS UNPACKING

/**
 * @brief Get field mode from sys_status message
 *
 * @return System mode, see MAV_MODE ENUM in mavlink/include/mavlink_types.h
 */
static inline uint8_t mavlink_msg_sys_status_get_mode(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->mode);
}

/**
 * @brief Get field nav_mode from sys_status message
 *
 * @return Navigation mode, see MAV_NAV_MODE ENUM
 */
static inline uint8_t mavlink_msg_sys_status_get_nav_mode(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->nav_mode);
}

/**
 * @brief Get field status from sys_status message
 *
 * @return System status flag, see MAV_STATUS ENUM
 */
static inline uint8_t mavlink_msg_sys_status_get_status(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->status);
}

/**
 * @brief Get field load from sys_status message
 *
 * @return Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 */
static inline uint16_t mavlink_msg_sys_status_get_load(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->load);
}

/**
 * @brief Get field vbat from sys_status message
 *
 * @return Battery voltage, in millivolts (1 = 1 millivolt)
 */
static inline uint16_t mavlink_msg_sys_status_get_vbat(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->vbat);
}

/**
 * @brief Get field battery_remaining from sys_status message
 *
 * @return Remaining battery energy: (0%: 0, 100%: 1000)
 */
static inline uint16_t mavlink_msg_sys_status_get_battery_remaining(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->battery_remaining);
}

/**
 * @brief Get field packet_drop from sys_status message
 *
 * @return Dropped packets (packets that were corrupted on reception on the MAV)
 */
static inline uint16_t mavlink_msg_sys_status_get_packet_drop(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->packet_drop);
}

/**
 * @brief Decode a sys_status message into a struct
 *
 * @param msg The message to decode
 * @param sys_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_sys_status_decode(const mavlink_message_t* msg, mavlink_sys_status_t* sys_status)
{
	memcpy( sys_status, msg->payload, sizeof(mavlink_sys_status_t));
}
