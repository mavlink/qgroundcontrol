// MESSAGE AUX_STATUS PACKING

#define MAVLINK_MSG_ID_AUX_STATUS 142
#define MAVLINK_MSG_ID_AUX_STATUS_LEN 12
#define MAVLINK_MSG_142_LEN 12

typedef struct __mavlink_aux_status_t 
{
	uint16_t load; ///< Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	uint16_t i2c0_err_count; ///< Number of I2C errors since startup
	uint16_t i2c1_err_count; ///< Number of I2C errors since startup
	uint16_t spi0_err_count; ///< Number of I2C errors since startup
	uint16_t spi1_err_count; ///< Number of I2C errors since startup
	uint16_t uart_total_err_count; ///< Number of I2C errors since startup

} mavlink_aux_status_t;

/**
 * @brief Pack a aux_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param i2c0_err_count Number of I2C errors since startup
 * @param i2c1_err_count Number of I2C errors since startup
 * @param spi0_err_count Number of I2C errors since startup
 * @param spi1_err_count Number of I2C errors since startup
 * @param uart_total_err_count Number of I2C errors since startup
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_aux_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t load, uint16_t i2c0_err_count, uint16_t i2c1_err_count, uint16_t spi0_err_count, uint16_t spi1_err_count, uint16_t uart_total_err_count)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_AUX_STATUS;

	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->i2c0_err_count = i2c0_err_count; // uint16_t:Number of I2C errors since startup
	p->i2c1_err_count = i2c1_err_count; // uint16_t:Number of I2C errors since startup
	p->spi0_err_count = spi0_err_count; // uint16_t:Number of I2C errors since startup
	p->spi1_err_count = spi1_err_count; // uint16_t:Number of I2C errors since startup
	p->uart_total_err_count = uart_total_err_count; // uint16_t:Number of I2C errors since startup

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_AUX_STATUS_LEN);
}

/**
 * @brief Pack a aux_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param i2c0_err_count Number of I2C errors since startup
 * @param i2c1_err_count Number of I2C errors since startup
 * @param spi0_err_count Number of I2C errors since startup
 * @param spi1_err_count Number of I2C errors since startup
 * @param uart_total_err_count Number of I2C errors since startup
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_aux_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t load, uint16_t i2c0_err_count, uint16_t i2c1_err_count, uint16_t spi0_err_count, uint16_t spi1_err_count, uint16_t uart_total_err_count)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_AUX_STATUS;

	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->i2c0_err_count = i2c0_err_count; // uint16_t:Number of I2C errors since startup
	p->i2c1_err_count = i2c1_err_count; // uint16_t:Number of I2C errors since startup
	p->spi0_err_count = spi0_err_count; // uint16_t:Number of I2C errors since startup
	p->spi1_err_count = spi1_err_count; // uint16_t:Number of I2C errors since startup
	p->uart_total_err_count = uart_total_err_count; // uint16_t:Number of I2C errors since startup

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_AUX_STATUS_LEN);
}

/**
 * @brief Encode a aux_status struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param aux_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_aux_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_aux_status_t* aux_status)
{
	return mavlink_msg_aux_status_pack(system_id, component_id, msg, aux_status->load, aux_status->i2c0_err_count, aux_status->i2c1_err_count, aux_status->spi0_err_count, aux_status->spi1_err_count, aux_status->uart_total_err_count);
}

/**
 * @brief Send a aux_status message
 * @param chan MAVLink channel to send the message
 *
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param i2c0_err_count Number of I2C errors since startup
 * @param i2c1_err_count Number of I2C errors since startup
 * @param spi0_err_count Number of I2C errors since startup
 * @param spi1_err_count Number of I2C errors since startup
 * @param uart_total_err_count Number of I2C errors since startup
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_aux_status_send(mavlink_channel_t chan, uint16_t load, uint16_t i2c0_err_count, uint16_t i2c1_err_count, uint16_t spi0_err_count, uint16_t spi1_err_count, uint16_t uart_total_err_count)
{
	mavlink_header_t hdr;
	mavlink_aux_status_t payload;
	uint16_t checksum;
	mavlink_aux_status_t *p = &payload;

	p->load = load; // uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->i2c0_err_count = i2c0_err_count; // uint16_t:Number of I2C errors since startup
	p->i2c1_err_count = i2c1_err_count; // uint16_t:Number of I2C errors since startup
	p->spi0_err_count = spi0_err_count; // uint16_t:Number of I2C errors since startup
	p->spi1_err_count = spi1_err_count; // uint16_t:Number of I2C errors since startup
	p->uart_total_err_count = uart_total_err_count; // uint16_t:Number of I2C errors since startup

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_AUX_STATUS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_AUX_STATUS;
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
// MESSAGE AUX_STATUS UNPACKING

/**
 * @brief Get field load from aux_status message
 *
 * @return Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 */
static inline uint16_t mavlink_msg_aux_status_get_load(const mavlink_message_t* msg)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	return (uint16_t)(p->load);
}

/**
 * @brief Get field i2c0_err_count from aux_status message
 *
 * @return Number of I2C errors since startup
 */
static inline uint16_t mavlink_msg_aux_status_get_i2c0_err_count(const mavlink_message_t* msg)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	return (uint16_t)(p->i2c0_err_count);
}

/**
 * @brief Get field i2c1_err_count from aux_status message
 *
 * @return Number of I2C errors since startup
 */
static inline uint16_t mavlink_msg_aux_status_get_i2c1_err_count(const mavlink_message_t* msg)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	return (uint16_t)(p->i2c1_err_count);
}

/**
 * @brief Get field spi0_err_count from aux_status message
 *
 * @return Number of I2C errors since startup
 */
static inline uint16_t mavlink_msg_aux_status_get_spi0_err_count(const mavlink_message_t* msg)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	return (uint16_t)(p->spi0_err_count);
}

/**
 * @brief Get field spi1_err_count from aux_status message
 *
 * @return Number of I2C errors since startup
 */
static inline uint16_t mavlink_msg_aux_status_get_spi1_err_count(const mavlink_message_t* msg)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	return (uint16_t)(p->spi1_err_count);
}

/**
 * @brief Get field uart_total_err_count from aux_status message
 *
 * @return Number of I2C errors since startup
 */
static inline uint16_t mavlink_msg_aux_status_get_uart_total_err_count(const mavlink_message_t* msg)
{
	mavlink_aux_status_t *p = (mavlink_aux_status_t *)&msg->payload[0];
	return (uint16_t)(p->uart_total_err_count);
}

/**
 * @brief Decode a aux_status message into a struct
 *
 * @param msg The message to decode
 * @param aux_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_aux_status_decode(const mavlink_message_t* msg, mavlink_aux_status_t* aux_status)
{
	memcpy( aux_status, msg->payload, sizeof(mavlink_aux_status_t));
}
