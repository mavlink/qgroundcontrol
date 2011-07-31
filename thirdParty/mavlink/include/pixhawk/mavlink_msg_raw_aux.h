// MESSAGE RAW_AUX PACKING

#define MAVLINK_MSG_ID_RAW_AUX 141
#define MAVLINK_MSG_ID_RAW_AUX_LEN 16
#define MAVLINK_MSG_141_LEN 16

typedef struct __mavlink_raw_aux_t 
{
	uint16_t adc1; ///< ADC1 (J405 ADC3, LPC2148 AD0.6)
	uint16_t adc2; ///< ADC2 (J405 ADC5, LPC2148 AD0.2)
	uint16_t adc3; ///< ADC3 (J405 ADC6, LPC2148 AD0.1)
	uint16_t adc4; ///< ADC4 (J405 ADC7, LPC2148 AD1.3)
	uint16_t vbat; ///< Battery voltage
	int16_t temp; ///< Temperature (degrees celcius)
	int32_t baro; ///< Barometric pressure (hecto Pascal)

} mavlink_raw_aux_t;

/**
 * @brief Pack a raw_aux message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param adc1 ADC1 (J405 ADC3, LPC2148 AD0.6)
 * @param adc2 ADC2 (J405 ADC5, LPC2148 AD0.2)
 * @param adc3 ADC3 (J405 ADC6, LPC2148 AD0.1)
 * @param adc4 ADC4 (J405 ADC7, LPC2148 AD1.3)
 * @param vbat Battery voltage
 * @param temp Temperature (degrees celcius)
 * @param baro Barometric pressure (hecto Pascal)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_raw_aux_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t adc1, uint16_t adc2, uint16_t adc3, uint16_t adc4, uint16_t vbat, int16_t temp, int32_t baro)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RAW_AUX;

	p->adc1 = adc1; // uint16_t:ADC1 (J405 ADC3, LPC2148 AD0.6)
	p->adc2 = adc2; // uint16_t:ADC2 (J405 ADC5, LPC2148 AD0.2)
	p->adc3 = adc3; // uint16_t:ADC3 (J405 ADC6, LPC2148 AD0.1)
	p->adc4 = adc4; // uint16_t:ADC4 (J405 ADC7, LPC2148 AD1.3)
	p->vbat = vbat; // uint16_t:Battery voltage
	p->temp = temp; // int16_t:Temperature (degrees celcius)
	p->baro = baro; // int32_t:Barometric pressure (hecto Pascal)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RAW_AUX_LEN);
}

/**
 * @brief Pack a raw_aux message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param adc1 ADC1 (J405 ADC3, LPC2148 AD0.6)
 * @param adc2 ADC2 (J405 ADC5, LPC2148 AD0.2)
 * @param adc3 ADC3 (J405 ADC6, LPC2148 AD0.1)
 * @param adc4 ADC4 (J405 ADC7, LPC2148 AD1.3)
 * @param vbat Battery voltage
 * @param temp Temperature (degrees celcius)
 * @param baro Barometric pressure (hecto Pascal)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_raw_aux_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t adc1, uint16_t adc2, uint16_t adc3, uint16_t adc4, uint16_t vbat, int16_t temp, int32_t baro)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_RAW_AUX;

	p->adc1 = adc1; // uint16_t:ADC1 (J405 ADC3, LPC2148 AD0.6)
	p->adc2 = adc2; // uint16_t:ADC2 (J405 ADC5, LPC2148 AD0.2)
	p->adc3 = adc3; // uint16_t:ADC3 (J405 ADC6, LPC2148 AD0.1)
	p->adc4 = adc4; // uint16_t:ADC4 (J405 ADC7, LPC2148 AD1.3)
	p->vbat = vbat; // uint16_t:Battery voltage
	p->temp = temp; // int16_t:Temperature (degrees celcius)
	p->baro = baro; // int32_t:Barometric pressure (hecto Pascal)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RAW_AUX_LEN);
}

/**
 * @brief Encode a raw_aux struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param raw_aux C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_raw_aux_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_raw_aux_t* raw_aux)
{
	return mavlink_msg_raw_aux_pack(system_id, component_id, msg, raw_aux->adc1, raw_aux->adc2, raw_aux->adc3, raw_aux->adc4, raw_aux->vbat, raw_aux->temp, raw_aux->baro);
}

/**
 * @brief Send a raw_aux message
 * @param chan MAVLink channel to send the message
 *
 * @param adc1 ADC1 (J405 ADC3, LPC2148 AD0.6)
 * @param adc2 ADC2 (J405 ADC5, LPC2148 AD0.2)
 * @param adc3 ADC3 (J405 ADC6, LPC2148 AD0.1)
 * @param adc4 ADC4 (J405 ADC7, LPC2148 AD1.3)
 * @param vbat Battery voltage
 * @param temp Temperature (degrees celcius)
 * @param baro Barometric pressure (hecto Pascal)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_raw_aux_send(mavlink_channel_t chan, uint16_t adc1, uint16_t adc2, uint16_t adc3, uint16_t adc4, uint16_t vbat, int16_t temp, int32_t baro)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg.payload[0];

	p->adc1 = adc1; // uint16_t:ADC1 (J405 ADC3, LPC2148 AD0.6)
	p->adc2 = adc2; // uint16_t:ADC2 (J405 ADC5, LPC2148 AD0.2)
	p->adc3 = adc3; // uint16_t:ADC3 (J405 ADC6, LPC2148 AD0.1)
	p->adc4 = adc4; // uint16_t:ADC4 (J405 ADC7, LPC2148 AD1.3)
	p->vbat = vbat; // uint16_t:Battery voltage
	p->temp = temp; // int16_t:Temperature (degrees celcius)
	p->baro = baro; // int32_t:Barometric pressure (hecto Pascal)

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_RAW_AUX_LEN;
	msg.msgid = MAVLINK_MSG_ID_RAW_AUX;
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
static inline void mavlink_msg_raw_aux_send(mavlink_channel_t chan, uint16_t adc1, uint16_t adc2, uint16_t adc3, uint16_t adc4, uint16_t vbat, int16_t temp, int32_t baro)
{
	mavlink_header_t hdr;
	mavlink_raw_aux_t payload;
	uint16_t checksum;
	mavlink_raw_aux_t *p = &payload;

	p->adc1 = adc1; // uint16_t:ADC1 (J405 ADC3, LPC2148 AD0.6)
	p->adc2 = adc2; // uint16_t:ADC2 (J405 ADC5, LPC2148 AD0.2)
	p->adc3 = adc3; // uint16_t:ADC3 (J405 ADC6, LPC2148 AD0.1)
	p->adc4 = adc4; // uint16_t:ADC4 (J405 ADC7, LPC2148 AD1.3)
	p->vbat = vbat; // uint16_t:Battery voltage
	p->temp = temp; // int16_t:Temperature (degrees celcius)
	p->baro = baro; // int32_t:Barometric pressure (hecto Pascal)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_RAW_AUX_LEN;
	hdr.msgid = MAVLINK_MSG_ID_RAW_AUX;
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
// MESSAGE RAW_AUX UNPACKING

/**
 * @brief Get field adc1 from raw_aux message
 *
 * @return ADC1 (J405 ADC3, LPC2148 AD0.6)
 */
static inline uint16_t mavlink_msg_raw_aux_get_adc1(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (uint16_t)(p->adc1);
}

/**
 * @brief Get field adc2 from raw_aux message
 *
 * @return ADC2 (J405 ADC5, LPC2148 AD0.2)
 */
static inline uint16_t mavlink_msg_raw_aux_get_adc2(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (uint16_t)(p->adc2);
}

/**
 * @brief Get field adc3 from raw_aux message
 *
 * @return ADC3 (J405 ADC6, LPC2148 AD0.1)
 */
static inline uint16_t mavlink_msg_raw_aux_get_adc3(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (uint16_t)(p->adc3);
}

/**
 * @brief Get field adc4 from raw_aux message
 *
 * @return ADC4 (J405 ADC7, LPC2148 AD1.3)
 */
static inline uint16_t mavlink_msg_raw_aux_get_adc4(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (uint16_t)(p->adc4);
}

/**
 * @brief Get field vbat from raw_aux message
 *
 * @return Battery voltage
 */
static inline uint16_t mavlink_msg_raw_aux_get_vbat(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (uint16_t)(p->vbat);
}

/**
 * @brief Get field temp from raw_aux message
 *
 * @return Temperature (degrees celcius)
 */
static inline int16_t mavlink_msg_raw_aux_get_temp(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (int16_t)(p->temp);
}

/**
 * @brief Get field baro from raw_aux message
 *
 * @return Barometric pressure (hecto Pascal)
 */
static inline int32_t mavlink_msg_raw_aux_get_baro(const mavlink_message_t* msg)
{
	mavlink_raw_aux_t *p = (mavlink_raw_aux_t *)&msg->payload[0];
	return (int32_t)(p->baro);
}

/**
 * @brief Decode a raw_aux message into a struct
 *
 * @param msg The message to decode
 * @param raw_aux C-struct to decode the message contents into
 */
static inline void mavlink_msg_raw_aux_decode(const mavlink_message_t* msg, mavlink_raw_aux_t* raw_aux)
{
	memcpy( raw_aux, msg->payload, sizeof(mavlink_raw_aux_t));
}
