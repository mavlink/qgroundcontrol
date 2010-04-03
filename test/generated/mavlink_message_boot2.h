// MESSAGE BOOT2 PACKING

#define MESSAGE_ID_BOOT2 1

typedef struct __boot2_t 
{
	uint32 version; ///< The onboard software version

} boot2_t;

/**
 * @brief Send a boot2 message
 *
 * @param version The onboard software version
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_boot2_pack(uint8_t system_id, CommMessage_t* msg, uint32 version)
{
	msg->msgid = MESSAGE_ID_BOOT2;
	uint16_t i = 0;

	i += put_uint32_by_index(version, i, msg->payload); //The onboard software version

	return finalize_message(msg, system_id, i);
}

static inline uint16_t message_boot2_encode(uint8_t system_id, CommMessage_t* msg, const boot2_t* boot2)
{
	message_boot2_pack(system_id, msg, boot2->version);
}
// MESSAGE BOOT2 UNPACKING

/**
 * @brief Get field version from boot2 message
 *
 * @return The onboard software version
 */
static inline uint32 message_boot2_get_version(CommMessage_t* msg)
{

}

static inline void message_boot2_decode(CommMessage_t* msg, boot2_t* boot2)
{
	boot2->version = message_boot2_get_version(msg);
}
