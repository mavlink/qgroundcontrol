// MESSAGE HEARTBEAT PACKING

#define MESSAGE_ID_HEARTBEAT 0

/**
 * @brief Send a heartbeat message
 *
 * @param type Type of the MAV (quadrotor, helicopter, etc.)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_heartbeat_pack(uint8_t system_id, CommMessage_t* msg, uint8 type)
{
	msg->msgid = MESSAGE_ID_HEARTBEAT;
	uint16_t i = 0;

	i += put_uint8_by_index(type, i, msg->payload); //Type of the MAV (quadrotor, helicopter, etc.)

	return finalize_message(msg, system_id, i);
}

// MESSAGE HEARTBEAT UNPACKING

/**
 * @brief Get field type from heartbeat message
 *
 * @return Type of the MAV (quadrotor, helicopter, etc.)
 */
static inline uint8 message_heartbeat_get_type(CommMessage_t* msg)
{
	return *((uint8*) (void*)msg->payload);
}

