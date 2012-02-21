// MESSAGE CHANGE_OPERATOR_CONTROL_ACK PACKING

#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK 6

typedef struct __mavlink_change_operator_control_ack_t
{
 uint8_t gcs_system_id; ///< ID of the GCS this message 
 uint8_t control_request; ///< 0: request control of this MAV, 1: Release control of this MAV
 uint8_t ack; ///< 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
} mavlink_change_operator_control_ack_t;

#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_LEN 3
#define MAVLINK_MSG_ID_6_LEN 3



#define MAVLINK_MESSAGE_INFO_CHANGE_OPERATOR_CONTROL_ACK { \
	"CHANGE_OPERATOR_CONTROL_ACK", \
	3, \
	{  { "gcs_system_id", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_change_operator_control_ack_t, gcs_system_id) }, \
         { "control_request", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_change_operator_control_ack_t, control_request) }, \
         { "ack", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_change_operator_control_ack_t, ack) }, \
         } \
}


/**
 * @brief Pack a change_operator_control_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param gcs_system_id ID of the GCS this message 
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param ack 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t gcs_system_id, uint8_t control_request, uint8_t ack)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[3];
	_mav_put_uint8_t(buf, 0, gcs_system_id);
	_mav_put_uint8_t(buf, 1, control_request);
	_mav_put_uint8_t(buf, 2, ack);

        memcpy(_MAV_PAYLOAD(msg), buf, 3);
#else
	mavlink_change_operator_control_ack_t packet;
	packet.gcs_system_id = gcs_system_id;
	packet.control_request = control_request;
	packet.ack = ack;

        memcpy(_MAV_PAYLOAD(msg), &packet, 3);
#endif

	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK;
	return mavlink_finalize_message(msg, system_id, component_id, 3);
}

/**
 * @brief Pack a change_operator_control_ack message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param gcs_system_id ID of the GCS this message 
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param ack 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t gcs_system_id,uint8_t control_request,uint8_t ack)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[3];
	_mav_put_uint8_t(buf, 0, gcs_system_id);
	_mav_put_uint8_t(buf, 1, control_request);
	_mav_put_uint8_t(buf, 2, ack);

        memcpy(_MAV_PAYLOAD(msg), buf, 3);
#else
	mavlink_change_operator_control_ack_t packet;
	packet.gcs_system_id = gcs_system_id;
	packet.control_request = control_request;
	packet.ack = ack;

        memcpy(_MAV_PAYLOAD(msg), &packet, 3);
#endif

	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK;
	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 3);
}

/**
 * @brief Encode a change_operator_control_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param change_operator_control_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_change_operator_control_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_change_operator_control_ack_t* change_operator_control_ack)
{
	return mavlink_msg_change_operator_control_ack_pack(system_id, component_id, msg, change_operator_control_ack->gcs_system_id, change_operator_control_ack->control_request, change_operator_control_ack->ack);
}

/**
 * @brief Send a change_operator_control_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param gcs_system_id ID of the GCS this message 
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param ack 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_change_operator_control_ack_send(mavlink_channel_t chan, uint8_t gcs_system_id, uint8_t control_request, uint8_t ack)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[3];
	_mav_put_uint8_t(buf, 0, gcs_system_id);
	_mav_put_uint8_t(buf, 1, control_request);
	_mav_put_uint8_t(buf, 2, ack);

	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK, buf, 3);
#else
	mavlink_change_operator_control_ack_t packet;
	packet.gcs_system_id = gcs_system_id;
	packet.control_request = control_request;
	packet.ack = ack;

	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK, (const char *)&packet, 3);
#endif
}

#endif

// MESSAGE CHANGE_OPERATOR_CONTROL_ACK UNPACKING


/**
 * @brief Get field gcs_system_id from change_operator_control_ack message
 *
 * @return ID of the GCS this message 
 */
static inline uint8_t mavlink_msg_change_operator_control_ack_get_gcs_system_id(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field control_request from change_operator_control_ack message
 *
 * @return 0: request control of this MAV, 1: Release control of this MAV
 */
static inline uint8_t mavlink_msg_change_operator_control_ack_get_control_request(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field ack from change_operator_control_ack message
 *
 * @return 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 */
static inline uint8_t mavlink_msg_change_operator_control_ack_get_ack(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Decode a change_operator_control_ack message into a struct
 *
 * @param msg The message to decode
 * @param change_operator_control_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_change_operator_control_ack_decode(const mavlink_message_t* msg, mavlink_change_operator_control_ack_t* change_operator_control_ack)
{
#if MAVLINK_NEED_BYTE_SWAP
	change_operator_control_ack->gcs_system_id = mavlink_msg_change_operator_control_ack_get_gcs_system_id(msg);
	change_operator_control_ack->control_request = mavlink_msg_change_operator_control_ack_get_control_request(msg);
	change_operator_control_ack->ack = mavlink_msg_change_operator_control_ack_get_ack(msg);
#else
	memcpy(change_operator_control_ack, _MAV_PAYLOAD(msg), 3);
#endif
}
