#pragma once
// MESSAGE MLRS_RADIO_LINK_INFORMATION PACKING

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION 60046


typedef struct __mavlink_mlrs_radio_link_information_t {
 uint16_t tx_frame_rate; /*< [Hz] Frame rate in Hz (frames per second) for Tx to Rx transmission. 0: unknown.*/
 uint16_t rx_frame_rate; /*< [Hz] Frame rate in Hz (frames per second) for Rx to Tx transmission. Normally equal to tx_packet_rate. 0: unknown.*/
 uint16_t tx_ser_data_rate; /*<  Maximum data rate of serial stream in bytes/s for Tx to Rx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.*/
 uint16_t rx_ser_data_rate; /*<  Maximum data rate of serial stream in bytes/s for Rx to Tx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.*/
 uint8_t target_system; /*<  System ID (ID of target system, normally flight controller).*/
 uint8_t target_component; /*<  Component ID (normally 0 for broadcast).*/
 uint8_t type; /*<  Radio link type. 0: unknown/generic type.*/
 uint8_t mode; /*<  Operation mode. Radio link dependent. UINT8_MAX: ignore/unknown.*/
 int8_t tx_power; /*< [dBm] Tx transmit power in dBm. INT8_MAX: unknown.*/
 int8_t rx_power; /*< [dBm] Rx transmit power in dBm. INT8_MAX: unknown.*/
 char mode_str[6]; /*<  Operation mode as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.*/
 char band_str[6]; /*<  Frequency band as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.*/
 uint8_t tx_receive_sensitivity; /*<  Receive sensitivity of Tx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.*/
 uint8_t rx_receive_sensitivity; /*<  Receive sensitivity of Rx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.*/
} mavlink_mlrs_radio_link_information_t;

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN 28
#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN 28
#define MAVLINK_MSG_ID_60046_LEN 28
#define MAVLINK_MSG_ID_60046_MIN_LEN 28

#define MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC 171
#define MAVLINK_MSG_ID_60046_CRC 171

#define MAVLINK_MSG_MLRS_RADIO_LINK_INFORMATION_FIELD_MODE_STR_LEN 6
#define MAVLINK_MSG_MLRS_RADIO_LINK_INFORMATION_FIELD_BAND_STR_LEN 6

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_MLRS_RADIO_LINK_INFORMATION { \
    60046, \
    "MLRS_RADIO_LINK_INFORMATION", \
    14, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_mlrs_radio_link_information_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_mlrs_radio_link_information_t, target_component) }, \
         { "type", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_mlrs_radio_link_information_t, type) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_mlrs_radio_link_information_t, mode) }, \
         { "tx_power", NULL, MAVLINK_TYPE_INT8_T, 0, 12, offsetof(mavlink_mlrs_radio_link_information_t, tx_power) }, \
         { "rx_power", NULL, MAVLINK_TYPE_INT8_T, 0, 13, offsetof(mavlink_mlrs_radio_link_information_t, rx_power) }, \
         { "tx_frame_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_mlrs_radio_link_information_t, tx_frame_rate) }, \
         { "rx_frame_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_mlrs_radio_link_information_t, rx_frame_rate) }, \
         { "mode_str", NULL, MAVLINK_TYPE_CHAR, 6, 14, offsetof(mavlink_mlrs_radio_link_information_t, mode_str) }, \
         { "band_str", NULL, MAVLINK_TYPE_CHAR, 6, 20, offsetof(mavlink_mlrs_radio_link_information_t, band_str) }, \
         { "tx_ser_data_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_mlrs_radio_link_information_t, tx_ser_data_rate) }, \
         { "rx_ser_data_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_mlrs_radio_link_information_t, rx_ser_data_rate) }, \
         { "tx_receive_sensitivity", NULL, MAVLINK_TYPE_UINT8_T, 0, 26, offsetof(mavlink_mlrs_radio_link_information_t, tx_receive_sensitivity) }, \
         { "rx_receive_sensitivity", NULL, MAVLINK_TYPE_UINT8_T, 0, 27, offsetof(mavlink_mlrs_radio_link_information_t, rx_receive_sensitivity) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_MLRS_RADIO_LINK_INFORMATION { \
    "MLRS_RADIO_LINK_INFORMATION", \
    14, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_mlrs_radio_link_information_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_mlrs_radio_link_information_t, target_component) }, \
         { "type", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_mlrs_radio_link_information_t, type) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_mlrs_radio_link_information_t, mode) }, \
         { "tx_power", NULL, MAVLINK_TYPE_INT8_T, 0, 12, offsetof(mavlink_mlrs_radio_link_information_t, tx_power) }, \
         { "rx_power", NULL, MAVLINK_TYPE_INT8_T, 0, 13, offsetof(mavlink_mlrs_radio_link_information_t, rx_power) }, \
         { "tx_frame_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_mlrs_radio_link_information_t, tx_frame_rate) }, \
         { "rx_frame_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_mlrs_radio_link_information_t, rx_frame_rate) }, \
         { "mode_str", NULL, MAVLINK_TYPE_CHAR, 6, 14, offsetof(mavlink_mlrs_radio_link_information_t, mode_str) }, \
         { "band_str", NULL, MAVLINK_TYPE_CHAR, 6, 20, offsetof(mavlink_mlrs_radio_link_information_t, band_str) }, \
         { "tx_ser_data_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_mlrs_radio_link_information_t, tx_ser_data_rate) }, \
         { "rx_ser_data_rate", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_mlrs_radio_link_information_t, rx_ser_data_rate) }, \
         { "tx_receive_sensitivity", NULL, MAVLINK_TYPE_UINT8_T, 0, 26, offsetof(mavlink_mlrs_radio_link_information_t, tx_receive_sensitivity) }, \
         { "rx_receive_sensitivity", NULL, MAVLINK_TYPE_UINT8_T, 0, 27, offsetof(mavlink_mlrs_radio_link_information_t, rx_receive_sensitivity) }, \
         } \
}
#endif

/**
 * @brief Pack a mlrs_radio_link_information message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param type  Radio link type. 0: unknown/generic type.
 * @param mode  Operation mode. Radio link dependent. UINT8_MAX: ignore/unknown.
 * @param tx_power [dBm] Tx transmit power in dBm. INT8_MAX: unknown.
 * @param rx_power [dBm] Rx transmit power in dBm. INT8_MAX: unknown.
 * @param tx_frame_rate [Hz] Frame rate in Hz (frames per second) for Tx to Rx transmission. 0: unknown.
 * @param rx_frame_rate [Hz] Frame rate in Hz (frames per second) for Rx to Tx transmission. Normally equal to tx_packet_rate. 0: unknown.
 * @param mode_str  Operation mode as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param band_str  Frequency band as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param tx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Tx to Rx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param rx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Rx to Tx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param tx_receive_sensitivity  Receive sensitivity of Tx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @param rx_receive_sensitivity  Receive sensitivity of Rx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint8_t type, uint8_t mode, int8_t tx_power, int8_t rx_power, uint16_t tx_frame_rate, uint16_t rx_frame_rate, const char *mode_str, const char *band_str, uint16_t tx_ser_data_rate, uint16_t rx_ser_data_rate, uint8_t tx_receive_sensitivity, uint8_t rx_receive_sensitivity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN];
    _mav_put_uint16_t(buf, 0, tx_frame_rate);
    _mav_put_uint16_t(buf, 2, rx_frame_rate);
    _mav_put_uint16_t(buf, 4, tx_ser_data_rate);
    _mav_put_uint16_t(buf, 6, rx_ser_data_rate);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, type);
    _mav_put_uint8_t(buf, 11, mode);
    _mav_put_int8_t(buf, 12, tx_power);
    _mav_put_int8_t(buf, 13, rx_power);
    _mav_put_uint8_t(buf, 26, tx_receive_sensitivity);
    _mav_put_uint8_t(buf, 27, rx_receive_sensitivity);
    _mav_put_char_array(buf, 14, mode_str, 6);
    _mav_put_char_array(buf, 20, band_str, 6);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#else
    mavlink_mlrs_radio_link_information_t packet;
    packet.tx_frame_rate = tx_frame_rate;
    packet.rx_frame_rate = rx_frame_rate;
    packet.tx_ser_data_rate = tx_ser_data_rate;
    packet.rx_ser_data_rate = rx_ser_data_rate;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.type = type;
    packet.mode = mode;
    packet.tx_power = tx_power;
    packet.rx_power = rx_power;
    packet.tx_receive_sensitivity = tx_receive_sensitivity;
    packet.rx_receive_sensitivity = rx_receive_sensitivity;
    mav_array_memcpy(packet.mode_str, mode_str, sizeof(char)*6);
    mav_array_memcpy(packet.band_str, band_str, sizeof(char)*6);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
}

/**
 * @brief Pack a mlrs_radio_link_information message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param type  Radio link type. 0: unknown/generic type.
 * @param mode  Operation mode. Radio link dependent. UINT8_MAX: ignore/unknown.
 * @param tx_power [dBm] Tx transmit power in dBm. INT8_MAX: unknown.
 * @param rx_power [dBm] Rx transmit power in dBm. INT8_MAX: unknown.
 * @param tx_frame_rate [Hz] Frame rate in Hz (frames per second) for Tx to Rx transmission. 0: unknown.
 * @param rx_frame_rate [Hz] Frame rate in Hz (frames per second) for Rx to Tx transmission. Normally equal to tx_packet_rate. 0: unknown.
 * @param mode_str  Operation mode as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param band_str  Frequency band as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param tx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Tx to Rx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param rx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Rx to Tx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param tx_receive_sensitivity  Receive sensitivity of Tx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @param rx_receive_sensitivity  Receive sensitivity of Rx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint8_t type, uint8_t mode, int8_t tx_power, int8_t rx_power, uint16_t tx_frame_rate, uint16_t rx_frame_rate, const char *mode_str, const char *band_str, uint16_t tx_ser_data_rate, uint16_t rx_ser_data_rate, uint8_t tx_receive_sensitivity, uint8_t rx_receive_sensitivity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN];
    _mav_put_uint16_t(buf, 0, tx_frame_rate);
    _mav_put_uint16_t(buf, 2, rx_frame_rate);
    _mav_put_uint16_t(buf, 4, tx_ser_data_rate);
    _mav_put_uint16_t(buf, 6, rx_ser_data_rate);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, type);
    _mav_put_uint8_t(buf, 11, mode);
    _mav_put_int8_t(buf, 12, tx_power);
    _mav_put_int8_t(buf, 13, rx_power);
    _mav_put_uint8_t(buf, 26, tx_receive_sensitivity);
    _mav_put_uint8_t(buf, 27, rx_receive_sensitivity);
    _mav_put_char_array(buf, 14, mode_str, 6);
    _mav_put_char_array(buf, 20, band_str, 6);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#else
    mavlink_mlrs_radio_link_information_t packet;
    packet.tx_frame_rate = tx_frame_rate;
    packet.rx_frame_rate = rx_frame_rate;
    packet.tx_ser_data_rate = tx_ser_data_rate;
    packet.rx_ser_data_rate = rx_ser_data_rate;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.type = type;
    packet.mode = mode;
    packet.tx_power = tx_power;
    packet.rx_power = rx_power;
    packet.tx_receive_sensitivity = tx_receive_sensitivity;
    packet.rx_receive_sensitivity = rx_receive_sensitivity;
    mav_array_memcpy(packet.mode_str, mode_str, sizeof(char)*6);
    mav_array_memcpy(packet.band_str, band_str, sizeof(char)*6);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#endif
}

/**
 * @brief Pack a mlrs_radio_link_information message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param type  Radio link type. 0: unknown/generic type.
 * @param mode  Operation mode. Radio link dependent. UINT8_MAX: ignore/unknown.
 * @param tx_power [dBm] Tx transmit power in dBm. INT8_MAX: unknown.
 * @param rx_power [dBm] Rx transmit power in dBm. INT8_MAX: unknown.
 * @param tx_frame_rate [Hz] Frame rate in Hz (frames per second) for Tx to Rx transmission. 0: unknown.
 * @param rx_frame_rate [Hz] Frame rate in Hz (frames per second) for Rx to Tx transmission. Normally equal to tx_packet_rate. 0: unknown.
 * @param mode_str  Operation mode as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param band_str  Frequency band as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param tx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Tx to Rx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param rx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Rx to Tx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param tx_receive_sensitivity  Receive sensitivity of Tx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @param rx_receive_sensitivity  Receive sensitivity of Rx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,uint8_t type,uint8_t mode,int8_t tx_power,int8_t rx_power,uint16_t tx_frame_rate,uint16_t rx_frame_rate,const char *mode_str,const char *band_str,uint16_t tx_ser_data_rate,uint16_t rx_ser_data_rate,uint8_t tx_receive_sensitivity,uint8_t rx_receive_sensitivity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN];
    _mav_put_uint16_t(buf, 0, tx_frame_rate);
    _mav_put_uint16_t(buf, 2, rx_frame_rate);
    _mav_put_uint16_t(buf, 4, tx_ser_data_rate);
    _mav_put_uint16_t(buf, 6, rx_ser_data_rate);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, type);
    _mav_put_uint8_t(buf, 11, mode);
    _mav_put_int8_t(buf, 12, tx_power);
    _mav_put_int8_t(buf, 13, rx_power);
    _mav_put_uint8_t(buf, 26, tx_receive_sensitivity);
    _mav_put_uint8_t(buf, 27, rx_receive_sensitivity);
    _mav_put_char_array(buf, 14, mode_str, 6);
    _mav_put_char_array(buf, 20, band_str, 6);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#else
    mavlink_mlrs_radio_link_information_t packet;
    packet.tx_frame_rate = tx_frame_rate;
    packet.rx_frame_rate = rx_frame_rate;
    packet.tx_ser_data_rate = tx_ser_data_rate;
    packet.rx_ser_data_rate = rx_ser_data_rate;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.type = type;
    packet.mode = mode;
    packet.tx_power = tx_power;
    packet.rx_power = rx_power;
    packet.tx_receive_sensitivity = tx_receive_sensitivity;
    packet.rx_receive_sensitivity = rx_receive_sensitivity;
    mav_array_memcpy(packet.mode_str, mode_str, sizeof(char)*6);
    mav_array_memcpy(packet.band_str, band_str, sizeof(char)*6);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
}

/**
 * @brief Encode a mlrs_radio_link_information struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_information C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_mlrs_radio_link_information_t* mlrs_radio_link_information)
{
    return mavlink_msg_mlrs_radio_link_information_pack(system_id, component_id, msg, mlrs_radio_link_information->target_system, mlrs_radio_link_information->target_component, mlrs_radio_link_information->type, mlrs_radio_link_information->mode, mlrs_radio_link_information->tx_power, mlrs_radio_link_information->rx_power, mlrs_radio_link_information->tx_frame_rate, mlrs_radio_link_information->rx_frame_rate, mlrs_radio_link_information->mode_str, mlrs_radio_link_information->band_str, mlrs_radio_link_information->tx_ser_data_rate, mlrs_radio_link_information->rx_ser_data_rate, mlrs_radio_link_information->tx_receive_sensitivity, mlrs_radio_link_information->rx_receive_sensitivity);
}

/**
 * @brief Encode a mlrs_radio_link_information struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_information C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_mlrs_radio_link_information_t* mlrs_radio_link_information)
{
    return mavlink_msg_mlrs_radio_link_information_pack_chan(system_id, component_id, chan, msg, mlrs_radio_link_information->target_system, mlrs_radio_link_information->target_component, mlrs_radio_link_information->type, mlrs_radio_link_information->mode, mlrs_radio_link_information->tx_power, mlrs_radio_link_information->rx_power, mlrs_radio_link_information->tx_frame_rate, mlrs_radio_link_information->rx_frame_rate, mlrs_radio_link_information->mode_str, mlrs_radio_link_information->band_str, mlrs_radio_link_information->tx_ser_data_rate, mlrs_radio_link_information->rx_ser_data_rate, mlrs_radio_link_information->tx_receive_sensitivity, mlrs_radio_link_information->rx_receive_sensitivity);
}

/**
 * @brief Encode a mlrs_radio_link_information struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param mlrs_radio_link_information C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_mlrs_radio_link_information_t* mlrs_radio_link_information)
{
    return mavlink_msg_mlrs_radio_link_information_pack_status(system_id, component_id, _status, msg,  mlrs_radio_link_information->target_system, mlrs_radio_link_information->target_component, mlrs_radio_link_information->type, mlrs_radio_link_information->mode, mlrs_radio_link_information->tx_power, mlrs_radio_link_information->rx_power, mlrs_radio_link_information->tx_frame_rate, mlrs_radio_link_information->rx_frame_rate, mlrs_radio_link_information->mode_str, mlrs_radio_link_information->band_str, mlrs_radio_link_information->tx_ser_data_rate, mlrs_radio_link_information->rx_ser_data_rate, mlrs_radio_link_information->tx_receive_sensitivity, mlrs_radio_link_information->rx_receive_sensitivity);
}

/**
 * @brief Send a mlrs_radio_link_information message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param type  Radio link type. 0: unknown/generic type.
 * @param mode  Operation mode. Radio link dependent. UINT8_MAX: ignore/unknown.
 * @param tx_power [dBm] Tx transmit power in dBm. INT8_MAX: unknown.
 * @param rx_power [dBm] Rx transmit power in dBm. INT8_MAX: unknown.
 * @param tx_frame_rate [Hz] Frame rate in Hz (frames per second) for Tx to Rx transmission. 0: unknown.
 * @param rx_frame_rate [Hz] Frame rate in Hz (frames per second) for Rx to Tx transmission. Normally equal to tx_packet_rate. 0: unknown.
 * @param mode_str  Operation mode as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param band_str  Frequency band as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 * @param tx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Tx to Rx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param rx_ser_data_rate  Maximum data rate of serial stream in bytes/s for Rx to Tx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 * @param tx_receive_sensitivity  Receive sensitivity of Tx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 * @param rx_receive_sensitivity  Receive sensitivity of Rx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_mlrs_radio_link_information_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t type, uint8_t mode, int8_t tx_power, int8_t rx_power, uint16_t tx_frame_rate, uint16_t rx_frame_rate, const char *mode_str, const char *band_str, uint16_t tx_ser_data_rate, uint16_t rx_ser_data_rate, uint8_t tx_receive_sensitivity, uint8_t rx_receive_sensitivity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN];
    _mav_put_uint16_t(buf, 0, tx_frame_rate);
    _mav_put_uint16_t(buf, 2, rx_frame_rate);
    _mav_put_uint16_t(buf, 4, tx_ser_data_rate);
    _mav_put_uint16_t(buf, 6, rx_ser_data_rate);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, type);
    _mav_put_uint8_t(buf, 11, mode);
    _mav_put_int8_t(buf, 12, tx_power);
    _mav_put_int8_t(buf, 13, rx_power);
    _mav_put_uint8_t(buf, 26, tx_receive_sensitivity);
    _mav_put_uint8_t(buf, 27, rx_receive_sensitivity);
    _mav_put_char_array(buf, 14, mode_str, 6);
    _mav_put_char_array(buf, 20, band_str, 6);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION, buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
#else
    mavlink_mlrs_radio_link_information_t packet;
    packet.tx_frame_rate = tx_frame_rate;
    packet.rx_frame_rate = rx_frame_rate;
    packet.tx_ser_data_rate = tx_ser_data_rate;
    packet.rx_ser_data_rate = rx_ser_data_rate;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.type = type;
    packet.mode = mode;
    packet.tx_power = tx_power;
    packet.rx_power = rx_power;
    packet.tx_receive_sensitivity = tx_receive_sensitivity;
    packet.rx_receive_sensitivity = rx_receive_sensitivity;
    mav_array_memcpy(packet.mode_str, mode_str, sizeof(char)*6);
    mav_array_memcpy(packet.band_str, band_str, sizeof(char)*6);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION, (const char *)&packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
#endif
}

/**
 * @brief Send a mlrs_radio_link_information message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_mlrs_radio_link_information_send_struct(mavlink_channel_t chan, const mavlink_mlrs_radio_link_information_t* mlrs_radio_link_information)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_mlrs_radio_link_information_send(chan, mlrs_radio_link_information->target_system, mlrs_radio_link_information->target_component, mlrs_radio_link_information->type, mlrs_radio_link_information->mode, mlrs_radio_link_information->tx_power, mlrs_radio_link_information->rx_power, mlrs_radio_link_information->tx_frame_rate, mlrs_radio_link_information->rx_frame_rate, mlrs_radio_link_information->mode_str, mlrs_radio_link_information->band_str, mlrs_radio_link_information->tx_ser_data_rate, mlrs_radio_link_information->rx_ser_data_rate, mlrs_radio_link_information->tx_receive_sensitivity, mlrs_radio_link_information->rx_receive_sensitivity);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION, (const char *)mlrs_radio_link_information, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
#endif
}

#if MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_mlrs_radio_link_information_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint8_t type, uint8_t mode, int8_t tx_power, int8_t rx_power, uint16_t tx_frame_rate, uint16_t rx_frame_rate, const char *mode_str, const char *band_str, uint16_t tx_ser_data_rate, uint16_t rx_ser_data_rate, uint8_t tx_receive_sensitivity, uint8_t rx_receive_sensitivity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint16_t(buf, 0, tx_frame_rate);
    _mav_put_uint16_t(buf, 2, rx_frame_rate);
    _mav_put_uint16_t(buf, 4, tx_ser_data_rate);
    _mav_put_uint16_t(buf, 6, rx_ser_data_rate);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, type);
    _mav_put_uint8_t(buf, 11, mode);
    _mav_put_int8_t(buf, 12, tx_power);
    _mav_put_int8_t(buf, 13, rx_power);
    _mav_put_uint8_t(buf, 26, tx_receive_sensitivity);
    _mav_put_uint8_t(buf, 27, rx_receive_sensitivity);
    _mav_put_char_array(buf, 14, mode_str, 6);
    _mav_put_char_array(buf, 20, band_str, 6);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION, buf, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
#else
    mavlink_mlrs_radio_link_information_t *packet = (mavlink_mlrs_radio_link_information_t *)msgbuf;
    packet->tx_frame_rate = tx_frame_rate;
    packet->rx_frame_rate = rx_frame_rate;
    packet->tx_ser_data_rate = tx_ser_data_rate;
    packet->rx_ser_data_rate = rx_ser_data_rate;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->type = type;
    packet->mode = mode;
    packet->tx_power = tx_power;
    packet->rx_power = rx_power;
    packet->tx_receive_sensitivity = tx_receive_sensitivity;
    packet->rx_receive_sensitivity = rx_receive_sensitivity;
    mav_array_memcpy(packet->mode_str, mode_str, sizeof(char)*6);
    mav_array_memcpy(packet->band_str, band_str, sizeof(char)*6);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION, (const char *)packet, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_MIN_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_CRC);
#endif
}
#endif

#endif

// MESSAGE MLRS_RADIO_LINK_INFORMATION UNPACKING


/**
 * @brief Get field target_system from mlrs_radio_link_information message
 *
 * @return  System ID (ID of target system, normally flight controller).
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_information_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Get field target_component from mlrs_radio_link_information message
 *
 * @return  Component ID (normally 0 for broadcast).
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_information_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  9);
}

/**
 * @brief Get field type from mlrs_radio_link_information message
 *
 * @return  Radio link type. 0: unknown/generic type.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_information_get_type(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  10);
}

/**
 * @brief Get field mode from mlrs_radio_link_information message
 *
 * @return  Operation mode. Radio link dependent. UINT8_MAX: ignore/unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_information_get_mode(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  11);
}

/**
 * @brief Get field tx_power from mlrs_radio_link_information message
 *
 * @return [dBm] Tx transmit power in dBm. INT8_MAX: unknown.
 */
static inline int8_t mavlink_msg_mlrs_radio_link_information_get_tx_power(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  12);
}

/**
 * @brief Get field rx_power from mlrs_radio_link_information message
 *
 * @return [dBm] Rx transmit power in dBm. INT8_MAX: unknown.
 */
static inline int8_t mavlink_msg_mlrs_radio_link_information_get_rx_power(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  13);
}

/**
 * @brief Get field tx_frame_rate from mlrs_radio_link_information message
 *
 * @return [Hz] Frame rate in Hz (frames per second) for Tx to Rx transmission. 0: unknown.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_get_tx_frame_rate(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field rx_frame_rate from mlrs_radio_link_information message
 *
 * @return [Hz] Frame rate in Hz (frames per second) for Rx to Tx transmission. Normally equal to tx_packet_rate. 0: unknown.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_get_rx_frame_rate(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  2);
}

/**
 * @brief Get field mode_str from mlrs_radio_link_information message
 *
 * @return  Operation mode as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_get_mode_str(const mavlink_message_t* msg, char *mode_str)
{
    return _MAV_RETURN_char_array(msg, mode_str, 6,  14);
}

/**
 * @brief Get field band_str from mlrs_radio_link_information message
 *
 * @return  Frequency band as human readable string. Radio link dependent. Terminated by NULL if the string length is less than 6 chars and WITHOUT NULL termination if the length is exactly 6 chars - applications have to provide 6+1 bytes storage if the mode is stored as string. Use a zero-length string if not known.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_get_band_str(const mavlink_message_t* msg, char *band_str)
{
    return _MAV_RETURN_char_array(msg, band_str, 6,  20);
}

/**
 * @brief Get field tx_ser_data_rate from mlrs_radio_link_information message
 *
 * @return  Maximum data rate of serial stream in bytes/s for Tx to Rx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_get_tx_ser_data_rate(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field rx_ser_data_rate from mlrs_radio_link_information message
 *
 * @return  Maximum data rate of serial stream in bytes/s for Rx to Tx transmission. 0: unknown. UINT16_MAX: data rate is 64 KBytes/s or larger.
 */
static inline uint16_t mavlink_msg_mlrs_radio_link_information_get_rx_ser_data_rate(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Get field tx_receive_sensitivity from mlrs_radio_link_information message
 *
 * @return  Receive sensitivity of Tx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_information_get_tx_receive_sensitivity(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  26);
}

/**
 * @brief Get field rx_receive_sensitivity from mlrs_radio_link_information message
 *
 * @return  Receive sensitivity of Rx in inverted dBm. 1..255 represents -1..-255 dBm, 0: unknown.
 */
static inline uint8_t mavlink_msg_mlrs_radio_link_information_get_rx_receive_sensitivity(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  27);
}

/**
 * @brief Decode a mlrs_radio_link_information message into a struct
 *
 * @param msg The message to decode
 * @param mlrs_radio_link_information C-struct to decode the message contents into
 */
static inline void mavlink_msg_mlrs_radio_link_information_decode(const mavlink_message_t* msg, mavlink_mlrs_radio_link_information_t* mlrs_radio_link_information)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mlrs_radio_link_information->tx_frame_rate = mavlink_msg_mlrs_radio_link_information_get_tx_frame_rate(msg);
    mlrs_radio_link_information->rx_frame_rate = mavlink_msg_mlrs_radio_link_information_get_rx_frame_rate(msg);
    mlrs_radio_link_information->tx_ser_data_rate = mavlink_msg_mlrs_radio_link_information_get_tx_ser_data_rate(msg);
    mlrs_radio_link_information->rx_ser_data_rate = mavlink_msg_mlrs_radio_link_information_get_rx_ser_data_rate(msg);
    mlrs_radio_link_information->target_system = mavlink_msg_mlrs_radio_link_information_get_target_system(msg);
    mlrs_radio_link_information->target_component = mavlink_msg_mlrs_radio_link_information_get_target_component(msg);
    mlrs_radio_link_information->type = mavlink_msg_mlrs_radio_link_information_get_type(msg);
    mlrs_radio_link_information->mode = mavlink_msg_mlrs_radio_link_information_get_mode(msg);
    mlrs_radio_link_information->tx_power = mavlink_msg_mlrs_radio_link_information_get_tx_power(msg);
    mlrs_radio_link_information->rx_power = mavlink_msg_mlrs_radio_link_information_get_rx_power(msg);
    mavlink_msg_mlrs_radio_link_information_get_mode_str(msg, mlrs_radio_link_information->mode_str);
    mavlink_msg_mlrs_radio_link_information_get_band_str(msg, mlrs_radio_link_information->band_str);
    mlrs_radio_link_information->tx_receive_sensitivity = mavlink_msg_mlrs_radio_link_information_get_tx_receive_sensitivity(msg);
    mlrs_radio_link_information->rx_receive_sensitivity = mavlink_msg_mlrs_radio_link_information_get_rx_receive_sensitivity(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN? msg->len : MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN;
        memset(mlrs_radio_link_information, 0, MAVLINK_MSG_ID_MLRS_RADIO_LINK_INFORMATION_LEN);
    memcpy(mlrs_radio_link_information, _MAV_PAYLOAD(msg), len);
#endif
}
