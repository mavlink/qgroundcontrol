#pragma once
// MESSAGE MLRS_RADIO_LINK_FLOW_CONTROL PACKING

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL 60047


typedef struct __mavlink_mlrs_radio_link_flow_control_t {
 uint16_t tx_ser_rate; /*< [bytes/s] Transmitted bytes per second, UINT16_MAX: invalid/unknown.*/
 uint16_t rx_ser_rate; /*< [bytes/s] Received bytes per second, UINT16_MAX: invalid/unknown.*/
 uint8_t tx_used_ser_bandwidth; /*< [c%] Transmit bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.*/
 uint8_t rx_used_ser_bandwidth; /*< [c%] Receive bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.*/
 uint8_t txbuf; /*< [c%] For compatibility with legacy method. UINT8_MAX: unknown.*/
} mavlink_mlrs_radio_link_flow_control_t;

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN 7
#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN 7
#define MAVLINK_MSG_ID_60047_LEN 7
#define MAVLINK_MSG_ID_60047_MIN_LEN 7

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC 55
#define MAVLINK_MSG_ID_60047_CRC 55



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_MLRS_RADIO_LINK_FLOW_CONTROL { \
    60047, \
    "MLRS_RADIO_LINK_FLOW_CONTROL", \
    5, \
    {  { "tx_ser_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_mlrs_radio_link_flow_control_t, tx_ser_rate) }, \
         { "rx_ser_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_mlrs_radio_link_flow_control_t, rx_ser_rate) }, \
         { "tx_used_ser_bandwidth", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_mlrs_radio_link_flow_control_t, tx_used_ser_bandwidth) }, \
         { "rx_used_ser_bandwidth", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_mlrs_radio_link_flow_control_t, rx_used_ser_bandwidth) }, \
         { "txbuf", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_mlrs_radio_link_flow_control_t, txbuf) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_MLRS_RADIO_LINK_FLOW_CONTROL { \
    "MLRS_RADIO_LINK_FLOW_CONTROL", \
    5, \
    {  { "tx_ser_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_mlrs_radio_link_flow_control_t, tx_ser_rate) }, \
         { "rx_ser_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_mlrs_radio_link_flow_control_t, rx_ser_rate) }, \
         { "tx_used_ser_bandwidth", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_mlrs_radio_link_flow_control_t, tx_used_ser_bandwidth) }, \
         { "rx_used_ser_bandwidth", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_mlrs_radio_link_flow_control_t, rx_used_ser_bandwidth) }, \
         { "txbuf", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_mlrs_radio_link_flow_control_t, txbuf) }, \
         } \
}
#endif

/**
 * @brief Pack a mlrs_radio_link_flow_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param tx_ser_rate [bytes/s] Transmitted bytes per second, UINT16_MAX: invalid/unknown.
 * @param rx_ser_rate [bytes/s] Received bytes per second, UINT16_MAX: invalid/unknown.
 * @param tx_used_ser_bandwidth [c%] Transmit bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param rx_used_ser_bandwidth [c%] Receive bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param txbuf [c%] For compatibility with legacy method. UINT8_MAX: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint16_t tx_ser_rate, uint16_t rx_ser_rate, uint8_t tx_used_ser_bandwidth, uint8_t rx_used_ser_bandwidth, uint8_t txbuf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN];
    _mav_put_uint16_t(buf, 0, tx_ser_rate);
    _mav_put_uint16_t(buf, 2, rx_ser_rate);
    _mav_put_uint8_t(buf, 4, tx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 5, rx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 6, txbuf);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#else
    mavlink_mlrs_radio_link_flow_control_t packet;
    packet.tx_ser_rate = tx_ser_rate;
    packet.rx_ser_rate = rx_ser_rate;
    packet.tx_used_ser_bandwidth = tx_used_ser_bandwidth;
    packet.rx_used_ser_bandwidth = rx_used_ser_bandwidth;
    packet.txbuf = txbuf;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
}

/**
 * @brief Pack a mlrs_radio_link_flow_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param tx_ser_rate [bytes/s] Transmitted bytes per second, UINT16_MAX: invalid/unknown.
 * @param rx_ser_rate [bytes/s] Received bytes per second, UINT16_MAX: invalid/unknown.
 * @param tx_used_ser_bandwidth [c%] Transmit bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param rx_used_ser_bandwidth [c%] Receive bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param txbuf [c%] For compatibility with legacy method. UINT8_MAX: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint16_t tx_ser_rate, uint16_t rx_ser_rate, uint8_t tx_used_ser_bandwidth, uint8_t rx_used_ser_bandwidth, uint8_t txbuf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN];
    _mav_put_uint16_t(buf, 0, tx_ser_rate);
    _mav_put_uint16_t(buf, 2, rx_ser_rate);
    _mav_put_uint8_t(buf, 4, tx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 5, rx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 6, txbuf);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#else
    mavlink_mlrs_radio_link_flow_control_t packet;
    packet.tx_ser_rate = tx_ser_rate;
    packet.rx_ser_rate = rx_ser_rate;
    packet.tx_used_ser_bandwidth = tx_used_ser_bandwidth;
    packet.rx_used_ser_bandwidth = rx_used_ser_bandwidth;
    packet.txbuf = txbuf;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#endif
}

/**
 * @brief Pack a mlrs_radio_link_flow_control message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param tx_ser_rate [bytes/s] Transmitted bytes per second, UINT16_MAX: invalid/unknown.
 * @param rx_ser_rate [bytes/s] Received bytes per second, UINT16_MAX: invalid/unknown.
 * @param tx_used_ser_bandwidth [c%] Transmit bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param rx_used_ser_bandwidth [c%] Receive bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param txbuf [c%] For compatibility with legacy method. UINT8_MAX: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint16_t tx_ser_rate,uint16_t rx_ser_rate,uint8_t tx_used_ser_bandwidth,uint8_t rx_used_ser_bandwidth,uint8_t txbuf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN];
    _mav_put_uint16_t(buf, 0, tx_ser_rate);
    _mav_put_uint16_t(buf, 2, rx_ser_rate);
    _mav_put_uint8_t(buf, 4, tx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 5, rx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 6, txbuf);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#else
    mavlink_mlrs_radio_link_flow_control_t packet;
    packet.tx_ser_rate = tx_ser_rate;
    packet.rx_ser_rate = rx_ser_rate;
    packet.tx_used_ser_bandwidth = tx_used_ser_bandwidth;
    packet.rx_used_ser_bandwidth = rx_used_ser_bandwidth;
    packet.txbuf = txbuf;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
}

/**
 * @brief Encode a mlrs_radio_link_flow_control struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_flow_control C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_mlrs_radio_link_flow_control_t* mlrs_radio_link_flow_control)
{
    return mavlink_msg_mlrs_radio_link_flow_control_pack(system_id, component_id, msg, mlrs_radio_link_flow_control->tx_ser_rate, mlrs_radio_link_flow_control->rx_ser_rate, mlrs_radio_link_flow_control->tx_used_ser_bandwidth, mlrs_radio_link_flow_control->rx_used_ser_bandwidth, mlrs_radio_link_flow_control->txbuf);
}

/**
 * @brief Encode a mlrs_radio_link_flow_control struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_flow_control C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_mlrs_radio_link_flow_control_t* mlrs_radio_link_flow_control)
{
    return mavlink_msg_mlrs_radio_link_flow_control_pack_chan(system_id, component_id, chan, msg, mlrs_radio_link_flow_control->tx_ser_rate, mlrs_radio_link_flow_control->rx_ser_rate, mlrs_radio_link_flow_control->tx_used_ser_bandwidth, mlrs_radio_link_flow_control->rx_used_ser_bandwidth, mlrs_radio_link_flow_control->txbuf);
}

/**
 * @brief Encode a mlrs_radio_link_flow_control struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_flow_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_mlrs_radio_link_flow_control_t* mlrs_radio_link_flow_control)
{
    return mavlink_msg_mlrs_radio_link_flow_control_pack_status(system_id, component_id, _status, msg,  mlrs_radio_link_flow_control->tx_ser_rate, mlrs_radio_link_flow_control->rx_ser_rate, mlrs_radio_link_flow_control->tx_used_ser_bandwidth, mlrs_radio_link_flow_control->rx_used_ser_bandwidth, mlrs_radio_link_flow_control->txbuf);
}

/**
 * @brief Send a mlrs_radio_link_flow_control message
 * @param chan MAVLink channel to send the message
 *
 * @param tx_ser_rate [bytes/s] Transmitted bytes per second, UINT16_MAX: invalid/unknown.
 * @param rx_ser_rate [bytes/s] Received bytes per second, UINT16_MAX: invalid/unknown.
 * @param tx_used_ser_bandwidth [c%] Transmit bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param rx_used_ser_bandwidth [c%] Receive bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 * @param txbuf [c%] For compatibility with legacy method. UINT8_MAX: unknown.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_mlrs_radio_link_flow_control_send(mavlink_channel_t chan, uint16_t tx_ser_rate, uint16_t rx_ser_rate, uint8_t tx_used_ser_bandwidth, uint8_t rx_used_ser_bandwidth, uint8_t txbuf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN];
    _mav_put_uint16_t(buf, 0, tx_ser_rate);
    _mav_put_uint16_t(buf, 2, rx_ser_rate);
    _mav_put_uint8_t(buf, 4, tx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 5, rx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 6, txbuf);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL, buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
#else
    mavlink_mlrs_radio_link_flow_control_t packet;
    packet.tx_ser_rate = tx_ser_rate;
    packet.rx_ser_rate = rx_ser_rate;
    packet.tx_used_ser_bandwidth = tx_used_ser_bandwidth;
    packet.rx_used_ser_bandwidth = rx_used_ser_bandwidth;
    packet.txbuf = txbuf;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL, (const char *)&packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
#endif
}

/**
 * @brief Send a mlrs_radio_link_flow_control message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_mlrs_radio_link_flow_control_send_struct(mavlink_channel_t chan, const mavlink_mlrs_radio_link_flow_control_t* mlrs_radio_link_flow_control)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_mlrs_radio_link_flow_control_send(chan, mlrs_radio_link_flow_control->tx_ser_rate, mlrs_radio_link_flow_control->rx_ser_rate, mlrs_radio_link_flow_control->tx_used_ser_bandwidth, mlrs_radio_link_flow_control->rx_used_ser_bandwidth, mlrs_radio_link_flow_control->txbuf);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL, (const char *)mlrs_radio_link_flow_control, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
#endif
}

#if MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_mlrs_radio_link_flow_control_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint16_t tx_ser_rate, uint16_t rx_ser_rate, uint8_t tx_used_ser_bandwidth, uint8_t rx_used_ser_bandwidth, uint8_t txbuf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint16_t(buf, 0, tx_ser_rate);
    _mav_put_uint16_t(buf, 2, rx_ser_rate);
    _mav_put_uint8_t(buf, 4, tx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 5, rx_used_ser_bandwidth);
    _mav_put_uint8_t(buf, 6, txbuf);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL, buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
#else
    mavlink_mlrs_radio_link_flow_control_t *packet = (mavlink_mlrs_radio_link_flow_control_t *)msgbuf;
    packet->tx_ser_rate = tx_ser_rate;
    packet->rx_ser_rate = rx_ser_rate;
    packet->tx_used_ser_bandwidth = tx_used_ser_bandwidth;
    packet->rx_used_ser_bandwidth = rx_used_ser_bandwidth;
    packet->txbuf = txbuf;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL, (const char *)packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_CRC);
#endif
}
#endif

#endif

// MESSAGE MLRS_RADIO_LINK_FLOW_CONTROL UNPACKING


/**
 * @brief Get field tx_ser_rate from mlrs_radio_link_flow_control message
 *
 * @return [bytes/s] Transmitted bytes per second, UINT16_MAX: invalid/unknown.
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_get_tx_ser_rate(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field rx_ser_rate from mlrs_radio_link_flow_control message
 *
 * @return [bytes/s] Received bytes per second, UINT16_MAX: invalid/unknown.
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_mlrs_radio_link_flow_control_get_rx_ser_rate(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  2);
}

/**
 * @brief Get field tx_used_ser_bandwidth from mlrs_radio_link_flow_control message
 *
 * @return [c%] Transmit bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_mlrs_radio_link_flow_control_get_tx_used_ser_bandwidth(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field rx_used_ser_bandwidth from mlrs_radio_link_flow_control message
 *
 * @return [c%] Receive bandwidth consumption. Values: 0..100, UINT8_MAX: invalid/unknown.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_mlrs_radio_link_flow_control_get_rx_used_ser_bandwidth(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field txbuf from mlrs_radio_link_flow_control message
 *
 * @return [c%] For compatibility with legacy method. UINT8_MAX: unknown.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_mlrs_radio_link_flow_control_get_txbuf(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Decode a mlrs_radio_link_flow_control message into a struct
 *
 * @param msg The message to decode
 * @param mlrs_radio_link_flow_control C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_mlrs_radio_link_flow_control_decode(const mavlink_message_t* msg, mavlink_mlrs_radio_link_flow_control_t* mlrs_radio_link_flow_control)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mlrs_radio_link_flow_control->tx_ser_rate = mavlink_msg_mlrs_radio_link_flow_control_get_tx_ser_rate(msg);
    mlrs_radio_link_flow_control->rx_ser_rate = mavlink_msg_mlrs_radio_link_flow_control_get_rx_ser_rate(msg);
    mlrs_radio_link_flow_control->tx_used_ser_bandwidth = mavlink_msg_mlrs_radio_link_flow_control_get_tx_used_ser_bandwidth(msg);
    mlrs_radio_link_flow_control->rx_used_ser_bandwidth = mavlink_msg_mlrs_radio_link_flow_control_get_rx_used_ser_bandwidth(msg);
    mlrs_radio_link_flow_control->txbuf = mavlink_msg_mlrs_radio_link_flow_control_get_txbuf(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN? msg->len : MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN;
        memset(mlrs_radio_link_flow_control, 0, MAVLINK_MSG_ID_MLRS_RADIO_LINK_FLOW_CONTROL_LEN);
    memcpy(mlrs_radio_link_flow_control, _MAV_PAYLOAD(msg), len);
#endif
}
