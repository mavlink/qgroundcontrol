// MESSAGE RC_INITIATE_RX_PAIRING PACKING

#define MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING 210

typedef struct __mavlink_rc_initiate_rx_pairing_t
{
 uint8_t rxtype; ///< The receiver type Spektrum:0
 uint8_t rxsubtype; ///< The receiver subtype Spektrum - DSM2:0, DSMX:1
} mavlink_rc_initiate_rx_pairing_t;

#define MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN 2
#define MAVLINK_MSG_ID_210_LEN 2

#define MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_CRC 205
#define MAVLINK_MSG_ID_210_CRC 205



#define MAVLINK_MESSAGE_INFO_RC_INITIATE_RX_PAIRING { \
	"RC_INITIATE_RX_PAIRING", \
	2, \
	{  { "rxtype", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rc_initiate_rx_pairing_t, rxtype) }, \
         { "rxsubtype", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_rc_initiate_rx_pairing_t, rxsubtype) }, \
         } \
}


/**
 * @brief Pack a rc_initiate_rx_pairing message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param rxtype The receiver type Spektrum:0
 * @param rxsubtype The receiver subtype Spektrum - DSM2:0, DSMX:1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rc_initiate_rx_pairing_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t rxtype, uint8_t rxsubtype)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN];
	_mav_put_uint8_t(buf, 0, rxtype);
	_mav_put_uint8_t(buf, 1, rxsubtype);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#else
	mavlink_rc_initiate_rx_pairing_t packet;
	packet.rxtype = rxtype;
	packet.rxsubtype = rxsubtype;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif
}

/**
 * @brief Pack a rc_initiate_rx_pairing message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rxtype The receiver type Spektrum:0
 * @param rxsubtype The receiver subtype Spektrum - DSM2:0, DSMX:1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rc_initiate_rx_pairing_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t rxtype,uint8_t rxsubtype)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN];
	_mav_put_uint8_t(buf, 0, rxtype);
	_mav_put_uint8_t(buf, 1, rxsubtype);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#else
	mavlink_rc_initiate_rx_pairing_t packet;
	packet.rxtype = rxtype;
	packet.rxsubtype = rxsubtype;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif
}

/**
 * @brief Encode a rc_initiate_rx_pairing struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rc_initiate_rx_pairing C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rc_initiate_rx_pairing_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rc_initiate_rx_pairing_t* rc_initiate_rx_pairing)
{
	return mavlink_msg_rc_initiate_rx_pairing_pack(system_id, component_id, msg, rc_initiate_rx_pairing->rxtype, rc_initiate_rx_pairing->rxsubtype);
}

/**
 * @brief Encode a rc_initiate_rx_pairing struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rc_initiate_rx_pairing C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rc_initiate_rx_pairing_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rc_initiate_rx_pairing_t* rc_initiate_rx_pairing)
{
	return mavlink_msg_rc_initiate_rx_pairing_pack_chan(system_id, component_id, chan, msg, rc_initiate_rx_pairing->rxtype, rc_initiate_rx_pairing->rxsubtype);
}

/**
 * @brief Send a rc_initiate_rx_pairing message
 * @param chan MAVLink channel to send the message
 *
 * @param rxtype The receiver type Spektrum:0
 * @param rxsubtype The receiver subtype Spektrum - DSM2:0, DSMX:1
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rc_initiate_rx_pairing_send(mavlink_channel_t chan, uint8_t rxtype, uint8_t rxsubtype)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN];
	_mav_put_uint8_t(buf, 0, rxtype);
	_mav_put_uint8_t(buf, 1, rxsubtype);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING, buf, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING, buf, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif
#else
	mavlink_rc_initiate_rx_pairing_t packet;
	packet.rxtype = rxtype;
	packet.rxsubtype = rxsubtype;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING, (const char *)&packet, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING, (const char *)&packet, MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif
#endif
}

#endif

// MESSAGE RC_INITIATE_RX_PAIRING UNPACKING


/**
 * @brief Get field rxtype from rc_initiate_rx_pairing message
 *
 * @return The receiver type Spektrum:0
 */
static inline uint8_t mavlink_msg_rc_initiate_rx_pairing_get_rxtype(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field rxsubtype from rc_initiate_rx_pairing message
 *
 * @return The receiver subtype Spektrum - DSM2:0, DSMX:1
 */
static inline uint8_t mavlink_msg_rc_initiate_rx_pairing_get_rxsubtype(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Decode a rc_initiate_rx_pairing message into a struct
 *
 * @param msg The message to decode
 * @param rc_initiate_rx_pairing C-struct to decode the message contents into
 */
static inline void mavlink_msg_rc_initiate_rx_pairing_decode(const mavlink_message_t* msg, mavlink_rc_initiate_rx_pairing_t* rc_initiate_rx_pairing)
{
#if MAVLINK_NEED_BYTE_SWAP
	rc_initiate_rx_pairing->rxtype = mavlink_msg_rc_initiate_rx_pairing_get_rxtype(msg);
	rc_initiate_rx_pairing->rxsubtype = mavlink_msg_rc_initiate_rx_pairing_get_rxsubtype(msg);
#else
	memcpy(rc_initiate_rx_pairing, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_RC_INITIATE_RX_PAIRING_LEN);
#endif
}
