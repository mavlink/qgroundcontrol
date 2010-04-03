// MESSAGE HEARTBEAT2 PACKING

#define MESSAGE_ID_HEARTBEAT2 0

typedef struct __heartbeat2_t 
{
	uint8 type; ///< Type of the MAV (quadrotor, helicopter, etc.)

} heartbeat2_t;

/**
 * @brief Send a heartbeat2 message
 *
 * @param type Type of the MAV (quadrotor, helicopter, etc.)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_heartbeat2_pack(uint8_t system_id, CommMessage_t* msg, uint8 type)
{
	msg->msgid = MESSAGE_ID_HEARTBEAT2;
	uint16_t i = 0;

	i += put_uint8_by_index(type, i, msg->payload); //Type of the MAV (quadrotor, helicopter, etc.)

	return finalize_message(msg, system_id, i);
}

static inline uint16_t message_heartbeat2_encode(uint8_t system_id, CommMessage_t* msg, const heartbeat2_t* heartbeat2)
{
	message_heartbeat2_pack(system_id, msg, heartbeat2->type);
}
// MESSAGE HEARTBEAT2 UNPACKING

/**
 * @brief Get field type from heartbeat2 message
 *
 * @return Type of the MAV (quadrotor, helicopter, etc.)
 */
static inline uint8 message_heartbeat2_get_type(CommMessage_t* msg)
{

}

static inline void message_heartbeat2_decode(CommMessage_t* msg, heartbeat2_t* heartbeat2)
{
	heartbeat2->type = message_heartbeat2_get_type(msg);
}
