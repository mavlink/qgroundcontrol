// MESSAGE SENSRAW PACKING

#define MESSAGE_ID_SENSRAW 100

/**
 * @brief Send a sensraw message
 *
 * @param xacc X acceleration (mg raw)
 * @param yacc Y acceleration (mg raw)
 * @param zacc Z acceleration (mg raw)
 * @param xgyro Angular speed around X axis (adc units)
 * @param ygyro Angular speed around Y axis (adc units)
 * @param zgyro Angular speed around Z axis (adc units)
 * @param xmag 
 * @param ymag 
 * @param zmag 
 * @param baro 
 * @param gdist 
 * @param temp 
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_sensraw_pack(uint8_t system_id, CommMessage_t* msg, uint32 xacc, uint32 yacc, uint32 zacc, uint32 xgyro, uint32 ygyro, uint32 zgyro, uint32 xmag, uint32 ymag, uint32 zmag, int32 baro, uint32 gdist, int32 temp)
{
	msg->msgid = MESSAGE_ID_SENSRAW;
	uint16_t i = 0;

	i += put_uint32_by_index(xacc, i, msg->payload); //X acceleration (mg raw)
	i += put_uint32_by_index(yacc, i, msg->payload); //Y acceleration (mg raw)
	i += put_uint32_by_index(zacc, i, msg->payload); //Z acceleration (mg raw)
	i += put_uint32_by_index(xgyro, i, msg->payload); //Angular speed around X axis (adc units)
	i += put_uint32_by_index(ygyro, i, msg->payload); //Angular speed around Y axis (adc units)
	i += put_uint32_by_index(zgyro, i, msg->payload); //Angular speed around Z axis (adc units)
	i += put_uint32_by_index(xmag, i, msg->payload); //
	i += put_uint32_by_index(ymag, i, msg->payload); //
	i += put_uint32_by_index(zmag, i, msg->payload); //
	i += put_int32_by_index(baro, i, msg->payload); //
	i += put_uint32_by_index(gdist, i, msg->payload); //
	i += put_int32_by_index(temp, i, msg->payload); //

	return finalize_message(msg, system_id, i);
}

// MESSAGE SENSRAW UNPACKING

/**
 * @brief Get field xacc from sensraw message
 *
 * @return X acceleration (mg raw)
 */
static inline uint32 message_sensraw_get_xacc(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload);
}

/**
 * @brief Get field yacc from sensraw message
 *
 * @return Y acceleration (mg raw)
 */
static inline uint32 message_sensraw_get_yacc(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32));
}

/**
 * @brief Get field zacc from sensraw message
 *
 * @return Z acceleration (mg raw)
 */
static inline uint32 message_sensraw_get_zacc(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field xgyro from sensraw message
 *
 * @return Angular speed around X axis (adc units)
 */
static inline uint32 message_sensraw_get_xgyro(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field ygyro from sensraw message
 *
 * @return Angular speed around Y axis (adc units)
 */
static inline uint32 message_sensraw_get_ygyro(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field zgyro from sensraw message
 *
 * @return Angular speed around Z axis (adc units)
 */
static inline uint32 message_sensraw_get_zgyro(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field xmag from sensraw message
 *
 * @return 
 */
static inline uint32 message_sensraw_get_xmag(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field ymag from sensraw message
 *
 * @return 
 */
static inline uint32 message_sensraw_get_ymag(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field zmag from sensraw message
 *
 * @return 
 */
static inline uint32 message_sensraw_get_zmag(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field baro from sensraw message
 *
 * @return 
 */
static inline int32 message_sensraw_get_baro(CommMessage_t* msg)
{
	return *((int32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32));
}

/**
 * @brief Get field gdist from sensraw message
 *
 * @return 
 */
static inline uint32 message_sensraw_get_gdist(CommMessage_t* msg)
{
	return *((uint32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(int32));
}

/**
 * @brief Get field temp from sensraw message
 *
 * @return 
 */
static inline int32 message_sensraw_get_temp(CommMessage_t* msg)
{
	return *((int32*) (void*)msg->payload+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(uint32)+sizeof(int32)+sizeof(uint32));
}

