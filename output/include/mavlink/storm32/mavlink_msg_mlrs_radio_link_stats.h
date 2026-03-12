#pragma once
// MESSAGE MLRS_RADIO_LINK_STATS PACKING

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS 60045

MAVPACKED(
typedef struct __mavlink_mlrs_radio_link_stats_t {
 uint16_t flags; /*<  Radio link statistics flags.*/
 uint8_t target_system; /*<  System ID (ID of target system, normally flight controller).*/
 uint8_t target_component; /*<  Component ID (normally 0 for broadcast).*/
 uint8_t rx_LQ_rc; /*< [c%] Link quality of RC data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.*/
 uint8_t rx_LQ_ser; /*< [c%] Link quality of serial MAVLink data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.*/
 uint8_t rx_rssi1; /*<  Rssi of antenna 1. 0: no reception, UINT8_MAX: unknown.*/
 int8_t rx_snr1; /*<  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.*/
 uint8_t tx_LQ_ser; /*< [c%] Link quality of serial MAVLink data stream from Rx to Tx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.*/
 uint8_t tx_rssi1; /*<  Rssi of antenna 1. 0: no reception. UINT8_MAX: unknown.*/
 int8_t tx_snr1; /*<  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.*/
 uint8_t rx_rssi2; /*<  Rssi of antenna 2. 0: no reception, UINT8_MAX: use rx_rssi1 if it is known else unknown.*/
 int8_t rx_snr2; /*<  Noise on antenna 2. Radio link dependent. INT8_MAX: use rx_snr1 if it is known else unknown.*/
 uint8_t tx_rssi2; /*<  Rssi of antenna 2. 0: no reception. UINT8_MAX: use tx_rssi1 if it is known else unknown.*/
 int8_t tx_snr2; /*<  Noise on antenna 2. Radio link dependent. INT8_MAX: use tx_snr1 if it is known else unknown.*/
 float frequency1; /*< [Hz] Frequency on antenna1 in Hz. 0: unknown.*/
 float frequency2; /*< [Hz] Frequency on antenna2 in Hz. 0: unknown.*/
}) mavlink_mlrs_radio_link_stats_t;

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN 23
#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN 15
#define MAVLINK_MSG_ID_60045_LEN 23
#define MAVLINK_MSG_ID_60045_MIN_LEN 15

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC 14
#define MAVLINK_MSG_ID_60045_CRC 14



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_MLRS_RADIO_LINK_STATS { \
    60045, \
    "MLRS_RADIO_LINK_STATS", \
    16, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_mlrs_radio_link_stats_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_mlrs_radio_link_stats_t, target_component) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_mlrs_radio_link_stats_t, flags) }, \
         { "rx_LQ_rc", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_mlrs_radio_link_stats_t, rx_LQ_rc) }, \
         { "rx_LQ_ser", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_mlrs_radio_link_stats_t, rx_LQ_ser) }, \
         { "rx_rssi1", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_mlrs_radio_link_stats_t, rx_rssi1) }, \
         { "rx_snr1", NULL, MAVLINK_TYPE_INT8_T, 0, 7, offsetof(mavlink_mlrs_radio_link_stats_t, rx_snr1) }, \
         { "tx_LQ_ser", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_mlrs_radio_link_stats_t, tx_LQ_ser) }, \
         { "tx_rssi1", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_mlrs_radio_link_stats_t, tx_rssi1) }, \
         { "tx_snr1", NULL, MAVLINK_TYPE_INT8_T, 0, 10, offsetof(mavlink_mlrs_radio_link_stats_t, tx_snr1) }, \
         { "rx_rssi2", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_mlrs_radio_link_stats_t, rx_rssi2) }, \
         { "rx_snr2", NULL, MAVLINK_TYPE_INT8_T, 0, 12, offsetof(mavlink_mlrs_radio_link_stats_t, rx_snr2) }, \
         { "tx_rssi2", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_mlrs_radio_link_stats_t, tx_rssi2) }, \
         { "tx_snr2", NULL, MAVLINK_TYPE_INT8_T, 0, 14, offsetof(mavlink_mlrs_radio_link_stats_t, tx_snr2) }, \
         { "frequency1", NULL, MAVLINK_TYPE_FLOAT, 0, 15, offsetof(mavlink_mlrs_radio_link_stats_t, frequency1) }, \
         { "frequency2", NULL, MAVLINK_TYPE_FLOAT, 0, 19, offsetof(mavlink_mlrs_radio_link_stats_t, frequency2) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_MLRS_RADIO_LINK_STATS { \
    "MLRS_RADIO_LINK_STATS", \
    16, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_mlrs_radio_link_stats_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_mlrs_radio_link_stats_t, target_component) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_mlrs_radio_link_stats_t, flags) }, \
         { "rx_LQ_rc", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_mlrs_radio_link_stats_t, rx_LQ_rc) }, \
         { "rx_LQ_ser", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_mlrs_radio_link_stats_t, rx_LQ_ser) }, \
         { "rx_rssi1", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_mlrs_radio_link_stats_t, rx_rssi1) }, \
         { "rx_snr1", NULL, MAVLINK_TYPE_INT8_T, 0, 7, offsetof(mavlink_mlrs_radio_link_stats_t, rx_snr1) }, \
         { "tx_LQ_ser", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_mlrs_radio_link_stats_t, tx_LQ_ser) }, \
         { "tx_rssi1", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_mlrs_radio_link_stats_t, tx_rssi1) }, \
         { "tx_snr1", NULL, MAVLINK_TYPE_INT8_T, 0, 10, offsetof(mavlink_mlrs_radio_link_stats_t, tx_snr1) }, \
         { "rx_rssi2", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_mlrs_radio_link_stats_t, rx_rssi2) }, \
         { "rx_snr2", NULL, MAVLINK_TYPE_INT8_T, 0, 12, offsetof(mavlink_mlrs_radio_link_stats_t, rx_snr2) }, \
         { "tx_rssi2", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_mlrs_radio_link_stats_t, tx_rssi2) }, \
         { "tx_snr2", NULL, MAVLINK_TYPE_INT8_T, 0, 14, offsetof(mavlink_mlrs_radio_link_stats_t, tx_snr2) }, \
         { "frequency1", NULL, MAVLINK_TYPE_FLOAT, 0, 15, offsetof(mavlink_mlrs_radio_link_stats_t, frequency1) }, \
         { "frequency2", NULL, MAVLINK_TYPE_FLOAT, 0, 19, offsetof(mavlink_mlrs_radio_link_stats_t, frequency2) }, \
         } \
}
#endif

/**
 * @brief Pack a mlrs_radio_link_stats message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param flags  Radio link statistics flags.
 * @param rx_LQ_rc [c%] Link quality of RC data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_LQ_ser [c%] Link quality of serial MAVLink data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_rssi1  Rssi of antenna 1. 0: no reception, UINT8_MAX: unknown.
 * @param rx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param tx_LQ_ser [c%] Link quality of serial MAVLink data stream from Rx to Tx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param tx_rssi1  Rssi of antenna 1. 0: no reception. UINT8_MAX: unknown.
 * @param tx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param rx_rssi2  Rssi of antenna 2. 0: no reception, UINT8_MAX: use rx_rssi1 if it is known else unknown.
 * @param rx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use rx_snr1 if it is known else unknown.
 * @param tx_rssi2  Rssi of antenna 2. 0: no reception. UINT8_MAX: use tx_rssi1 if it is known else unknown.
 * @param tx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use tx_snr1 if it is known else unknown.
 * @param frequency1 [Hz] Frequency on antenna1 in Hz. 0: unknown.
 * @param frequency2 [Hz] Frequency on antenna2 in Hz. 0: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint16_t flags, uint8_t rx_LQ_rc, uint8_t rx_LQ_ser, uint8_t rx_rssi1, int8_t rx_snr1, uint8_t tx_LQ_ser, uint8_t tx_rssi1, int8_t tx_snr1, uint8_t rx_rssi2, int8_t rx_snr2, uint8_t tx_rssi2, int8_t tx_snr2, float frequency1, float frequency2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN];
    _mav_put_uint16_t(buf, 0, flags);
    _mav_put_uint8_t(buf, 2, target_system);
    _mav_put_uint8_t(buf, 3, target_component);
    _mav_put_uint8_t(buf, 4, rx_LQ_rc);
    _mav_put_uint8_t(buf, 5, rx_LQ_ser);
    _mav_put_uint8_t(buf, 6, rx_rssi1);
    _mav_put_int8_t(buf, 7, rx_snr1);
    _mav_put_uint8_t(buf, 8, tx_LQ_ser);
    _mav_put_uint8_t(buf, 9, tx_rssi1);
    _mav_put_int8_t(buf, 10, tx_snr1);
    _mav_put_uint8_t(buf, 11, rx_rssi2);
    _mav_put_int8_t(buf, 12, rx_snr2);
    _mav_put_uint8_t(buf, 13, tx_rssi2);
    _mav_put_int8_t(buf, 14, tx_snr2);
    _mav_put_float(buf, 15, frequency1);
    _mav_put_float(buf, 19, frequency2);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#else
    mavlink_mlrs_radio_link_stats_t packet;
    packet.flags = flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.rx_LQ_rc = rx_LQ_rc;
    packet.rx_LQ_ser = rx_LQ_ser;
    packet.rx_rssi1 = rx_rssi1;
    packet.rx_snr1 = rx_snr1;
    packet.tx_LQ_ser = tx_LQ_ser;
    packet.tx_rssi1 = tx_rssi1;
    packet.tx_snr1 = tx_snr1;
    packet.rx_rssi2 = rx_rssi2;
    packet.rx_snr2 = rx_snr2;
    packet.tx_rssi2 = tx_rssi2;
    packet.tx_snr2 = tx_snr2;
    packet.frequency1 = frequency1;
    packet.frequency2 = frequency2;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
}

/**
 * @brief Pack a mlrs_radio_link_stats message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param flags  Radio link statistics flags.
 * @param rx_LQ_rc [c%] Link quality of RC data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_LQ_ser [c%] Link quality of serial MAVLink data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_rssi1  Rssi of antenna 1. 0: no reception, UINT8_MAX: unknown.
 * @param rx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param tx_LQ_ser [c%] Link quality of serial MAVLink data stream from Rx to Tx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param tx_rssi1  Rssi of antenna 1. 0: no reception. UINT8_MAX: unknown.
 * @param tx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param rx_rssi2  Rssi of antenna 2. 0: no reception, UINT8_MAX: use rx_rssi1 if it is known else unknown.
 * @param rx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use rx_snr1 if it is known else unknown.
 * @param tx_rssi2  Rssi of antenna 2. 0: no reception. UINT8_MAX: use tx_rssi1 if it is known else unknown.
 * @param tx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use tx_snr1 if it is known else unknown.
 * @param frequency1 [Hz] Frequency on antenna1 in Hz. 0: unknown.
 * @param frequency2 [Hz] Frequency on antenna2 in Hz. 0: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint16_t flags, uint8_t rx_LQ_rc, uint8_t rx_LQ_ser, uint8_t rx_rssi1, int8_t rx_snr1, uint8_t tx_LQ_ser, uint8_t tx_rssi1, int8_t tx_snr1, uint8_t rx_rssi2, int8_t rx_snr2, uint8_t tx_rssi2, int8_t tx_snr2, float frequency1, float frequency2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN];
    _mav_put_uint16_t(buf, 0, flags);
    _mav_put_uint8_t(buf, 2, target_system);
    _mav_put_uint8_t(buf, 3, target_component);
    _mav_put_uint8_t(buf, 4, rx_LQ_rc);
    _mav_put_uint8_t(buf, 5, rx_LQ_ser);
    _mav_put_uint8_t(buf, 6, rx_rssi1);
    _mav_put_int8_t(buf, 7, rx_snr1);
    _mav_put_uint8_t(buf, 8, tx_LQ_ser);
    _mav_put_uint8_t(buf, 9, tx_rssi1);
    _mav_put_int8_t(buf, 10, tx_snr1);
    _mav_put_uint8_t(buf, 11, rx_rssi2);
    _mav_put_int8_t(buf, 12, rx_snr2);
    _mav_put_uint8_t(buf, 13, tx_rssi2);
    _mav_put_int8_t(buf, 14, tx_snr2);
    _mav_put_float(buf, 15, frequency1);
    _mav_put_float(buf, 19, frequency2);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#else
    mavlink_mlrs_radio_link_stats_t packet;
    packet.flags = flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.rx_LQ_rc = rx_LQ_rc;
    packet.rx_LQ_ser = rx_LQ_ser;
    packet.rx_rssi1 = rx_rssi1;
    packet.rx_snr1 = rx_snr1;
    packet.tx_LQ_ser = tx_LQ_ser;
    packet.tx_rssi1 = tx_rssi1;
    packet.tx_snr1 = tx_snr1;
    packet.rx_rssi2 = rx_rssi2;
    packet.rx_snr2 = rx_snr2;
    packet.tx_rssi2 = tx_rssi2;
    packet.tx_snr2 = tx_snr2;
    packet.frequency1 = frequency1;
    packet.frequency2 = frequency2;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#endif
}

/**
 * @brief Pack a mlrs_radio_link_stats message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param flags  Radio link statistics flags.
 * @param rx_LQ_rc [c%] Link quality of RC data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_LQ_ser [c%] Link quality of serial MAVLink data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_rssi1  Rssi of antenna 1. 0: no reception, UINT8_MAX: unknown.
 * @param rx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param tx_LQ_ser [c%] Link quality of serial MAVLink data stream from Rx to Tx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param tx_rssi1  Rssi of antenna 1. 0: no reception. UINT8_MAX: unknown.
 * @param tx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param rx_rssi2  Rssi of antenna 2. 0: no reception, UINT8_MAX: use rx_rssi1 if it is known else unknown.
 * @param rx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use rx_snr1 if it is known else unknown.
 * @param tx_rssi2  Rssi of antenna 2. 0: no reception. UINT8_MAX: use tx_rssi1 if it is known else unknown.
 * @param tx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use tx_snr1 if it is known else unknown.
 * @param frequency1 [Hz] Frequency on antenna1 in Hz. 0: unknown.
 * @param frequency2 [Hz] Frequency on antenna2 in Hz. 0: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,uint16_t flags,uint8_t rx_LQ_rc,uint8_t rx_LQ_ser,uint8_t rx_rssi1,int8_t rx_snr1,uint8_t tx_LQ_ser,uint8_t tx_rssi1,int8_t tx_snr1,uint8_t rx_rssi2,int8_t rx_snr2,uint8_t tx_rssi2,int8_t tx_snr2,float frequency1,float frequency2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN];
    _mav_put_uint16_t(buf, 0, flags);
    _mav_put_uint8_t(buf, 2, target_system);
    _mav_put_uint8_t(buf, 3, target_component);
    _mav_put_uint8_t(buf, 4, rx_LQ_rc);
    _mav_put_uint8_t(buf, 5, rx_LQ_ser);
    _mav_put_uint8_t(buf, 6, rx_rssi1);
    _mav_put_int8_t(buf, 7, rx_snr1);
    _mav_put_uint8_t(buf, 8, tx_LQ_ser);
    _mav_put_uint8_t(buf, 9, tx_rssi1);
    _mav_put_int8_t(buf, 10, tx_snr1);
    _mav_put_uint8_t(buf, 11, rx_rssi2);
    _mav_put_int8_t(buf, 12, rx_snr2);
    _mav_put_uint8_t(buf, 13, tx_rssi2);
    _mav_put_int8_t(buf, 14, tx_snr2);
    _mav_put_float(buf, 15, frequency1);
    _mav_put_float(buf, 19, frequency2);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#else
    mavlink_mlrs_radio_link_stats_t packet;
    packet.flags = flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.rx_LQ_rc = rx_LQ_rc;
    packet.rx_LQ_ser = rx_LQ_ser;
    packet.rx_rssi1 = rx_rssi1;
    packet.rx_snr1 = rx_snr1;
    packet.tx_LQ_ser = tx_LQ_ser;
    packet.tx_rssi1 = tx_rssi1;
    packet.tx_snr1 = tx_snr1;
    packet.rx_rssi2 = rx_rssi2;
    packet.rx_snr2 = rx_snr2;
    packet.tx_rssi2 = tx_rssi2;
    packet.tx_snr2 = tx_snr2;
    packet.frequency1 = frequency1;
    packet.frequency2 = frequency2;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
}

/**
 * @brief Encode a mlrs_radio_link_stats struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_stats C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_mlrs_radio_link_stats_t* mlrs_radio_link_stats)
{
    return mavlink_msg_mlrs_radio_link_stats_pack(system_id, component_id, msg, mlrs_radio_link_stats->target_system, mlrs_radio_link_stats->target_component, mlrs_radio_link_stats->flags, mlrs_radio_link_stats->rx_LQ_rc, mlrs_radio_link_stats->rx_LQ_ser, mlrs_radio_link_stats->rx_rssi1, mlrs_radio_link_stats->rx_snr1, mlrs_radio_link_stats->tx_LQ_ser, mlrs_radio_link_stats->tx_rssi1, mlrs_radio_link_stats->tx_snr1, mlrs_radio_link_stats->rx_rssi2, mlrs_radio_link_stats->rx_snr2, mlrs_radio_link_stats->tx_rssi2, mlrs_radio_link_stats->tx_snr2, mlrs_radio_link_stats->frequency1, mlrs_radio_link_stats->frequency2);
}

/**
 * @brief Encode a mlrs_radio_link_stats struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_stats C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_mlrs_radio_link_stats_t* mlrs_radio_link_stats)
{
    return mavlink_msg_mlrs_radio_link_stats_pack_chan(system_id, component_id, chan, msg, mlrs_radio_link_stats->target_system, mlrs_radio_link_stats->target_component, mlrs_radio_link_stats->flags, mlrs_radio_link_stats->rx_LQ_rc, mlrs_radio_link_stats->rx_LQ_ser, mlrs_radio_link_stats->rx_rssi1, mlrs_radio_link_stats->rx_snr1, mlrs_radio_link_stats->tx_LQ_ser, mlrs_radio_link_stats->tx_rssi1, mlrs_radio_link_stats->tx_snr1, mlrs_radio_link_stats->rx_rssi2, mlrs_radio_link_stats->rx_snr2, mlrs_radio_link_stats->tx_rssi2, mlrs_radio_link_stats->tx_snr2, mlrs_radio_link_stats->frequency1, mlrs_radio_link_stats->frequency2);
}

/**
 * @brief Encode a mlrs_radio_link_stats struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_stats C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_mlrs_radio_link_stats_t* mlrs_radio_link_stats)
{
    return mavlink_msg_mlrs_radio_link_stats_pack_status(system_id, component_id, _status, msg,  mlrs_radio_link_stats->target_system, mlrs_radio_link_stats->target_component, mlrs_radio_link_stats->flags, mlrs_radio_link_stats->rx_LQ_rc, mlrs_radio_link_stats->rx_LQ_ser, mlrs_radio_link_stats->rx_rssi1, mlrs_radio_link_stats->rx_snr1, mlrs_radio_link_stats->tx_LQ_ser, mlrs_radio_link_stats->tx_rssi1, mlrs_radio_link_stats->tx_snr1, mlrs_radio_link_stats->rx_rssi2, mlrs_radio_link_stats->rx_snr2, mlrs_radio_link_stats->tx_rssi2, mlrs_radio_link_stats->tx_snr2, mlrs_radio_link_stats->frequency1, mlrs_radio_link_stats->frequency2);
}

/**
 * @brief Send a mlrs_radio_link_stats message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param flags  Radio link statistics flags.
 * @param rx_LQ_rc [c%] Link quality of RC data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_LQ_ser [c%] Link quality of serial MAVLink data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param rx_rssi1  Rssi of antenna 1. 0: no reception, UINT8_MAX: unknown.
 * @param rx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param tx_LQ_ser [c%] Link quality of serial MAVLink data stream from Rx to Tx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 * @param tx_rssi1  Rssi of antenna 1. 0: no reception. UINT8_MAX: unknown.
 * @param tx_snr1  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 * @param rx_rssi2  Rssi of antenna 2. 0: no reception, UINT8_MAX: use rx_rssi1 if it is known else unknown.
 * @param rx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use rx_snr1 if it is known else unknown.
 * @param tx_rssi2  Rssi of antenna 2. 0: no reception. UINT8_MAX: use tx_rssi1 if it is known else unknown.
 * @param tx_snr2  Noise on antenna 2. Radio link dependent. INT8_MAX: use tx_snr1 if it is known else unknown.
 * @param frequency1 [Hz] Frequency on antenna1 in Hz. 0: unknown.
 * @param frequency2 [Hz] Frequency on antenna2 in Hz. 0: unknown.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_mlrs_radio_link_stats_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t flags, uint8_t rx_LQ_rc, uint8_t rx_LQ_ser, uint8_t rx_rssi1, int8_t rx_snr1, uint8_t tx_LQ_ser, uint8_t tx_rssi1, int8_t tx_snr1, uint8_t rx_rssi2, int8_t rx_snr2, uint8_t tx_rssi2, int8_t tx_snr2, float frequency1, float frequency2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN];
    _mav_put_uint16_t(buf, 0, flags);
    _mav_put_uint8_t(buf, 2, target_system);
    _mav_put_uint8_t(buf, 3, target_component);
    _mav_put_uint8_t(buf, 4, rx_LQ_rc);
    _mav_put_uint8_t(buf, 5, rx_LQ_ser);
    _mav_put_uint8_t(buf, 6, rx_rssi1);
    _mav_put_int8_t(buf, 7, rx_snr1);
    _mav_put_uint8_t(buf, 8, tx_LQ_ser);
    _mav_put_uint8_t(buf, 9, tx_rssi1);
    _mav_put_int8_t(buf, 10, tx_snr1);
    _mav_put_uint8_t(buf, 11, rx_rssi2);
    _mav_put_int8_t(buf, 12, rx_snr2);
    _mav_put_uint8_t(buf, 13, tx_rssi2);
    _mav_put_int8_t(buf, 14, tx_snr2);
    _mav_put_float(buf, 15, frequency1);
    _mav_put_float(buf, 19, frequency2);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS, buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
#else
    mavlink_mlrs_radio_link_stats_t packet;
    packet.flags = flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.rx_LQ_rc = rx_LQ_rc;
    packet.rx_LQ_ser = rx_LQ_ser;
    packet.rx_rssi1 = rx_rssi1;
    packet.rx_snr1 = rx_snr1;
    packet.tx_LQ_ser = tx_LQ_ser;
    packet.tx_rssi1 = tx_rssi1;
    packet.tx_snr1 = tx_snr1;
    packet.rx_rssi2 = rx_rssi2;
    packet.rx_snr2 = rx_snr2;
    packet.tx_rssi2 = tx_rssi2;
    packet.tx_snr2 = tx_snr2;
    packet.frequency1 = frequency1;
    packet.frequency2 = frequency2;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS, (const char *)&packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
#endif
}

/**
 * @brief Send a mlrs_radio_link_stats message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_mlrs_radio_link_stats_send_struct(mavlink_channel_t chan, const mavlink_mlrs_radio_link_stats_t* mlrs_radio_link_stats)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_mlrs_radio_link_stats_send(chan, mlrs_radio_link_stats->target_system, mlrs_radio_link_stats->target_component, mlrs_radio_link_stats->flags, mlrs_radio_link_stats->rx_LQ_rc, mlrs_radio_link_stats->rx_LQ_ser, mlrs_radio_link_stats->rx_rssi1, mlrs_radio_link_stats->rx_snr1, mlrs_radio_link_stats->tx_LQ_ser, mlrs_radio_link_stats->tx_rssi1, mlrs_radio_link_stats->tx_snr1, mlrs_radio_link_stats->rx_rssi2, mlrs_radio_link_stats->rx_snr2, mlrs_radio_link_stats->tx_rssi2, mlrs_radio_link_stats->tx_snr2, mlrs_radio_link_stats->frequency1, mlrs_radio_link_stats->frequency2);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS, (const char *)mlrs_radio_link_stats, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
#endif
}

#if MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_mlrs_radio_link_stats_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint16_t flags, uint8_t rx_LQ_rc, uint8_t rx_LQ_ser, uint8_t rx_rssi1, int8_t rx_snr1, uint8_t tx_LQ_ser, uint8_t tx_rssi1, int8_t tx_snr1, uint8_t rx_rssi2, int8_t rx_snr2, uint8_t tx_rssi2, int8_t tx_snr2, float frequency1, float frequency2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint16_t(buf, 0, flags);
    _mav_put_uint8_t(buf, 2, target_system);
    _mav_put_uint8_t(buf, 3, target_component);
    _mav_put_uint8_t(buf, 4, rx_LQ_rc);
    _mav_put_uint8_t(buf, 5, rx_LQ_ser);
    _mav_put_uint8_t(buf, 6, rx_rssi1);
    _mav_put_int8_t(buf, 7, rx_snr1);
    _mav_put_uint8_t(buf, 8, tx_LQ_ser);
    _mav_put_uint8_t(buf, 9, tx_rssi1);
    _mav_put_int8_t(buf, 10, tx_snr1);
    _mav_put_uint8_t(buf, 11, rx_rssi2);
    _mav_put_int8_t(buf, 12, rx_snr2);
    _mav_put_uint8_t(buf, 13, tx_rssi2);
    _mav_put_int8_t(buf, 14, tx_snr2);
    _mav_put_float(buf, 15, frequency1);
    _mav_put_float(buf, 19, frequency2);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS, buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
#else
    mavlink_mlrs_radio_link_stats_t *packet = (mavlink_mlrs_radio_link_stats_t *)msgbuf;
    packet->flags = flags;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->rx_LQ_rc = rx_LQ_rc;
    packet->rx_LQ_ser = rx_LQ_ser;
    packet->rx_rssi1 = rx_rssi1;
    packet->rx_snr1 = rx_snr1;
    packet->tx_LQ_ser = tx_LQ_ser;
    packet->tx_rssi1 = tx_rssi1;
    packet->tx_snr1 = tx_snr1;
    packet->rx_rssi2 = rx_rssi2;
    packet->rx_snr2 = rx_snr2;
    packet->tx_rssi2 = tx_rssi2;
    packet->tx_snr2 = tx_snr2;
    packet->frequency1 = frequency1;
    packet->frequency2 = frequency2;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS, (const char *)packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_CRC);
#endif
}
#endif

#endif

// MESSAGE MLRS_RADIO_LINK_STATS UNPACKING


/**
 * @brief Get field target_system from mlrs_radio_link_stats message
 *
 * @return  System ID (ID of target system, normally flight controller).
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field target_component from mlrs_radio_link_stats message
 *
 * @return  Component ID (normally 0 for broadcast).
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field flags from mlrs_radio_link_stats message
 *
 * @return  Radio link statistics flags.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_stats_get_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field rx_LQ_rc from mlrs_radio_link_stats message
 *
 * @return [c%] Link quality of RC data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_rx_LQ_rc(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field rx_LQ_ser from mlrs_radio_link_stats message
 *
 * @return [c%] Link quality of serial MAVLink data stream from Tx to Rx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_rx_LQ_ser(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field rx_rssi1 from mlrs_radio_link_stats message
 *
 * @return  Rssi of antenna 1. 0: no reception, UINT8_MAX: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_rx_rssi1(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Get field rx_snr1 from mlrs_radio_link_stats message
 *
 * @return  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 */
static inline int8_t mavlink_msg_mlrs_radio_link_stats_get_rx_snr1(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  7);
}

/**
 * @brief Get field tx_LQ_ser from mlrs_radio_link_stats message
 *
 * @return [c%] Link quality of serial MAVLink data stream from Rx to Tx. Values: 1..100, 0: no link connection, UINT8_MAX: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_tx_LQ_ser(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Get field tx_rssi1 from mlrs_radio_link_stats message
 *
 * @return  Rssi of antenna 1. 0: no reception. UINT8_MAX: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_tx_rssi1(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  9);
}

/**
 * @brief Get field tx_snr1 from mlrs_radio_link_stats message
 *
 * @return  Noise on antenna 1. Radio link dependent. INT8_MAX: unknown.
 */
static inline int8_t mavlink_msg_mlrs_radio_link_stats_get_tx_snr1(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  10);
}

/**
 * @brief Get field rx_rssi2 from mlrs_radio_link_stats message
 *
 * @return  Rssi of antenna 2. 0: no reception, UINT8_MAX: use rx_rssi1 if it is known else unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_rx_rssi2(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  11);
}

/**
 * @brief Get field rx_snr2 from mlrs_radio_link_stats message
 *
 * @return  Noise on antenna 2. Radio link dependent. INT8_MAX: use rx_snr1 if it is known else unknown.
 */
static inline int8_t mavlink_msg_mlrs_radio_link_stats_get_rx_snr2(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  12);
}

/**
 * @brief Get field tx_rssi2 from mlrs_radio_link_stats message
 *
 * @return  Rssi of antenna 2. 0: no reception. UINT8_MAX: use tx_rssi1 if it is known else unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_stats_get_tx_rssi2(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Get field tx_snr2 from mlrs_radio_link_stats message
 *
 * @return  Noise on antenna 2. Radio link dependent. INT8_MAX: use tx_snr1 if it is known else unknown.
 */
static inline int8_t mavlink_msg_mlrs_radio_link_stats_get_tx_snr2(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  14);
}

/**
 * @brief Get field frequency1 from mlrs_radio_link_stats message
 *
 * @return [Hz] Frequency on antenna1 in Hz. 0: unknown.
 */
static inline float mavlink_msg_mlrs_radio_link_stats_get_frequency1(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  15);
}

/**
 * @brief Get field frequency2 from mlrs_radio_link_stats message
 *
 * @return [Hz] Frequency on antenna2 in Hz. 0: unknown.
 */
static inline float mavlink_msg_mlrs_radio_link_stats_get_frequency2(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  19);
}

/**
 * @brief Decode a mlrs_radio_link_stats message into a struct
 *
 * @param msg The message to decode
 * @param mlrs_radio_link_stats C-struct to decode the message contents into
 */
static inline void mavlink_msg_mlrs_radio_link_stats_decode(const mavlink_message_t* msg, mavlink_mlrs_radio_link_stats_t* mlrs_radio_link_stats)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mlrs_radio_link_stats->flags = mavlink_msg_mlrs_radio_link_stats_get_flags(msg);
    mlrs_radio_link_stats->target_system = mavlink_msg_mlrs_radio_link_stats_get_target_system(msg);
    mlrs_radio_link_stats->target_component = mavlink_msg_mlrs_radio_link_stats_get_target_component(msg);
    mlrs_radio_link_stats->rx_LQ_rc = mavlink_msg_mlrs_radio_link_stats_get_rx_LQ_rc(msg);
    mlrs_radio_link_stats->rx_LQ_ser = mavlink_msg_mlrs_radio_link_stats_get_rx_LQ_ser(msg);
    mlrs_radio_link_stats->rx_rssi1 = mavlink_msg_mlrs_radio_link_stats_get_rx_rssi1(msg);
    mlrs_radio_link_stats->rx_snr1 = mavlink_msg_mlrs_radio_link_stats_get_rx_snr1(msg);
    mlrs_radio_link_stats->tx_LQ_ser = mavlink_msg_mlrs_radio_link_stats_get_tx_LQ_ser(msg);
    mlrs_radio_link_stats->tx_rssi1 = mavlink_msg_mlrs_radio_link_stats_get_tx_rssi1(msg);
    mlrs_radio_link_stats->tx_snr1 = mavlink_msg_mlrs_radio_link_stats_get_tx_snr1(msg);
    mlrs_radio_link_stats->rx_rssi2 = mavlink_msg_mlrs_radio_link_stats_get_rx_rssi2(msg);
    mlrs_radio_link_stats->rx_snr2 = mavlink_msg_mlrs_radio_link_stats_get_rx_snr2(msg);
    mlrs_radio_link_stats->tx_rssi2 = mavlink_msg_mlrs_radio_link_stats_get_tx_rssi2(msg);
    mlrs_radio_link_stats->tx_snr2 = mavlink_msg_mlrs_radio_link_stats_get_tx_snr2(msg);
    mlrs_radio_link_stats->frequency1 = mavlink_msg_mlrs_radio_link_stats_get_frequency1(msg);
    mlrs_radio_link_stats->frequency2 = mavlink_msg_mlrs_radio_link_stats_get_frequency2(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN? msg->len : MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN;
        memset(mlrs_radio_link_stats, 0, MAVLINK_MSG_ID_MLRS_RADIO_LINK_STATS_LEN);
    memcpy(mlrs_radio_link_stats, _MAV_PAYLOAD(msg), len);
#endif
}
