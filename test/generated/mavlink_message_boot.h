// MESSAGE BOOT PACKING

#define MESSAGE_ID_BOOT 1

/**
 * @brief Send a boot message
 *
 * @param version The onboard software version
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_boot_pack(uint8_t system_id, CommMessage_t* msg, uint32 version)
{
	msg->msgid = MESSAGE_ID_BOOT;
	uint16_t i = 0;

	i += put_uint32_by_index(version, i, msg->payload); //The onboard software version

	return finalize_message(msg, system_id, i);
}

// MESSAGE BOOT UNPACKING

/**
 * @brief Get field version from boot message
 *
 * @return The onboard software version
 */
static inline uint32 message_boot_get_version(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload);
}

