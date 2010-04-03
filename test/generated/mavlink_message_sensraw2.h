// MESSAGE SENSRAW2 PACKING

#define MESSAGE_ID_SENSRAW2 100

typedef struct __sensraw2_t 
{
	uint32 xacc; ///< X acceleration (mg raw)
	uint32 yacc; ///< Y acceleration (mg raw)
	uint32 zacc; ///< Z acceleration (mg raw)
	uint32 xgyro; ///< Angular speed around X axis (adc units)
	uint32 ygyro; ///< Angular speed around Y axis (adc units)
	uint32 zgyro; ///< Angular speed around Z axis (adc units)
	uint32 xmag; ///< 
	uint32 ymag; ///< 
	uint32 zmag; ///< 
	int32 baro; ///< 
	uint32 gdist; ///< 
	int32 temp; ///< 

} sensraw2_t;

/**
 * @brief Send a sensraw2 message
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
static inline uint16_t message_sensraw2_pack(uint8_t system_id, CommMessage_t* msg, uint32 xacc, uint32 yacc, uint32 zacc, uint32 xgyro, uint32 ygyro, uint32 zgyro, uint32 xmag, uint32 ymag, uint32 zmag, int32 baro, uint32 gdist, int32 temp)
{
	msg->msgid = MESSAGE_ID_SENSRAW2;
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

static inline uint16_t message_sensraw2_encode(uint8_t system_id, CommMessage_t* msg, const sensraw2_t* sensraw2)
{
	message_sensraw2_pack(system_id, msg, sensraw2->xacc, sensraw2->yacc, sensraw2->zacc, sensraw2->xgyro, sensraw2->ygyro, sensraw2->zgyro, sensraw2->xmag, sensraw2->ymag, sensraw2->zmag, sensraw2->baro, sensraw2->gdist, sensraw2->temp);
}
// MESSAGE SENSRAW2 UNPACKING

/**
 * @brief Get field xacc from sensraw2 message
 *
 * @return X acceleration (mg raw)
 */
static inline uint32 message_sensraw2_get_xacc(CommMessage_t* msg)
{

}

/**
 * @brief Get field yacc from sensraw2 message
 *
 * @return Y acceleration (mg raw)
 */
static inline uint32 message_sensraw2_get_yacc(CommMessage_t* msg)
{

}

/**
 * @brief Get field zacc from sensraw2 message
 *
 * @return Z acceleration (mg raw)
 */
static inline uint32 message_sensraw2_get_zacc(CommMessage_t* msg)
{

}

/**
 * @brief Get field xgyro from sensraw2 message
 *
 * @return Angular speed around X axis (adc units)
 */
static inline uint32 message_sensraw2_get_xgyro(CommMessage_t* msg)
{

}

/**
 * @brief Get field ygyro from sensraw2 message
 *
 * @return Angular speed around Y axis (adc units)
 */
static inline uint32 message_sensraw2_get_ygyro(CommMessage_t* msg)
{

}

/**
 * @brief Get field zgyro from sensraw2 message
 *
 * @return Angular speed around Z axis (adc units)
 */
static inline uint32 message_sensraw2_get_zgyro(CommMessage_t* msg)
{

}

/**
 * @brief Get field xmag from sensraw2 message
 *
 * @return 
 */
static inline uint32 message_sensraw2_get_xmag(CommMessage_t* msg)
{

}

/**
 * @brief Get field ymag from sensraw2 message
 *
 * @return 
 */
static inline uint32 message_sensraw2_get_ymag(CommMessage_t* msg)
{

}

/**
 * @brief Get field zmag from sensraw2 message
 *
 * @return 
 */
static inline uint32 message_sensraw2_get_zmag(CommMessage_t* msg)
{

}

/**
 * @brief Get field baro from sensraw2 message
 *
 * @return 
 */
static inline int32 message_sensraw2_get_baro(CommMessage_t* msg)
{

}

/**
 * @brief Get field gdist from sensraw2 message
 *
 * @return 
 */
static inline uint32 message_sensraw2_get_gdist(CommMessage_t* msg)
{

}

/**
 * @brief Get field temp from sensraw2 message
 *
 * @return 
 */
static inline int32 message_sensraw2_get_temp(CommMessage_t* msg)
{

}

static inline void message_sensraw2_decode(CommMessage_t* msg, sensraw2_t* sensraw2)
{
	sensraw2->xacc = message_sensraw2_get_xacc(msg);
	sensraw2->yacc = message_sensraw2_get_yacc(msg);
	sensraw2->zacc = message_sensraw2_get_zacc(msg);
	sensraw2->xgyro = message_sensraw2_get_xgyro(msg);
	sensraw2->ygyro = message_sensraw2_get_ygyro(msg);
	sensraw2->zgyro = message_sensraw2_get_zgyro(msg);
	sensraw2->xmag = message_sensraw2_get_xmag(msg);
	sensraw2->ymag = message_sensraw2_get_ymag(msg);
	sensraw2->zmag = message_sensraw2_get_zmag(msg);
	sensraw2->baro = message_sensraw2_get_baro(msg);
	sensraw2->gdist = message_sensraw2_get_gdist(msg);
	sensraw2->temp = message_sensraw2_get_temp(msg);
}
