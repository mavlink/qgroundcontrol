#pragma once
// MESSAGE GLOBAL_POSITION PACKING

#define MAVLINK_MSG_ID_GLOBAL_POSITION 296


typedef struct __mavlink_global_position_t {
 uint64_t time_usec; /*< [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.*/
 int32_t lat; /*< [degE7] Latitude (WGS84)*/
 int32_t lon; /*< [degE7] Longitude (WGS84)*/
 float alt; /*< [m] Altitude (MSL - position-system specific value)*/
 float alt_ellipsoid; /*< [m] Altitude (WGS84 elipsoid)*/
 float eph; /*< [m] Standard deviation of horizontal position error*/
 float epv; /*< [m] Standard deviation of vertical position error*/
 uint8_t id; /*<  Sensor ID*/
 uint8_t source; /*<  Source of position/estimate (such as GNSS, estimator, etc.)*/
 uint8_t flags; /*<  Status flags*/
} mavlink_global_position_t;

#define MAVLINK_MSG_ID_GLOBAL_POSITION_LEN 35
#define MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN 35
#define MAVLINK_MSG_ID_296_LEN 35
#define MAVLINK_MSG_ID_296_MIN_LEN 35

#define MAVLINK_MSG_ID_GLOBAL_POSITION_CRC 50
#define MAVLINK_MSG_ID_296_CRC 50



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_GLOBAL_POSITION { \
    296, \
    "GLOBAL_POSITION", \
    10, \
    {  { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 32, offsetof(mavlink_global_position_t, id) }, \
         { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_global_position_t, time_usec) }, \
         { "source", NULL, MAVLINK_TYPE_UINT8_T, 0, 33, offsetof(mavlink_global_position_t, source) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT8_T, 0, 34, offsetof(mavlink_global_position_t, flags) }, \
         { "lat", NULL, MAVLINK_TYPE_INT32_T, 0, 8, offsetof(mavlink_global_position_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_INT32_T, 0, 12, offsetof(mavlink_global_position_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_global_position_t, alt) }, \
         { "alt_ellipsoid", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_global_position_t, alt_ellipsoid) }, \
         { "eph", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_global_position_t, eph) }, \
         { "epv", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_global_position_t, epv) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_GLOBAL_POSITION { \
    "GLOBAL_POSITION", \
    10, \
    {  { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 32, offsetof(mavlink_global_position_t, id) }, \
         { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_global_position_t, time_usec) }, \
         { "source", NULL, MAVLINK_TYPE_UINT8_T, 0, 33, offsetof(mavlink_global_position_t, source) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT8_T, 0, 34, offsetof(mavlink_global_position_t, flags) }, \
         { "lat", NULL, MAVLINK_TYPE_INT32_T, 0, 8, offsetof(mavlink_global_position_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_INT32_T, 0, 12, offsetof(mavlink_global_position_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_global_position_t, alt) }, \
         { "alt_ellipsoid", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_global_position_t, alt_ellipsoid) }, \
         { "eph", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_global_position_t, eph) }, \
         { "epv", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_global_position_t, epv) }, \
         } \
}
#endif

/**
 * @brief Pack a global_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param id  Sensor ID
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @param source  Source of position/estimate (such as GNSS, estimator, etc.)
 * @param flags  Status flags
 * @param lat [degE7] Latitude (WGS84)
 * @param lon [degE7] Longitude (WGS84)
 * @param alt [m] Altitude (MSL - position-system specific value)
 * @param alt_ellipsoid [m] Altitude (WGS84 elipsoid)
 * @param eph [m] Standard deviation of horizontal position error
 * @param epv [m] Standard deviation of vertical position error
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t id, uint64_t time_usec, uint8_t source, uint8_t flags, int32_t lat, int32_t lon, float alt, float alt_ellipsoid, float eph, float epv)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GLOBAL_POSITION_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_float(buf, 20, alt_ellipsoid);
    _mav_put_float(buf, 24, eph);
    _mav_put_float(buf, 28, epv);
    _mav_put_uint8_t(buf, 32, id);
    _mav_put_uint8_t(buf, 33, source);
    _mav_put_uint8_t(buf, 34, flags);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#else
    mavlink_global_position_t packet;
    packet.time_usec = time_usec;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.alt_ellipsoid = alt_ellipsoid;
    packet.eph = eph;
    packet.epv = epv;
    packet.id = id;
    packet.source = source;
    packet.flags = flags;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
}

/**
 * @brief Pack a global_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param id  Sensor ID
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @param source  Source of position/estimate (such as GNSS, estimator, etc.)
 * @param flags  Status flags
 * @param lat [degE7] Latitude (WGS84)
 * @param lon [degE7] Longitude (WGS84)
 * @param alt [m] Altitude (MSL - position-system specific value)
 * @param alt_ellipsoid [m] Altitude (WGS84 elipsoid)
 * @param eph [m] Standard deviation of horizontal position error
 * @param epv [m] Standard deviation of vertical position error
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t id, uint64_t time_usec, uint8_t source, uint8_t flags, int32_t lat, int32_t lon, float alt, float alt_ellipsoid, float eph, float epv)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GLOBAL_POSITION_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_float(buf, 20, alt_ellipsoid);
    _mav_put_float(buf, 24, eph);
    _mav_put_float(buf, 28, epv);
    _mav_put_uint8_t(buf, 32, id);
    _mav_put_uint8_t(buf, 33, source);
    _mav_put_uint8_t(buf, 34, flags);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#else
    mavlink_global_position_t packet;
    packet.time_usec = time_usec;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.alt_ellipsoid = alt_ellipsoid;
    packet.eph = eph;
    packet.epv = epv;
    packet.id = id;
    packet.source = source;
    packet.flags = flags;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#endif
}

/**
 * @brief Pack a global_position message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param id  Sensor ID
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @param source  Source of position/estimate (such as GNSS, estimator, etc.)
 * @param flags  Status flags
 * @param lat [degE7] Latitude (WGS84)
 * @param lon [degE7] Longitude (WGS84)
 * @param alt [m] Altitude (MSL - position-system specific value)
 * @param alt_ellipsoid [m] Altitude (WGS84 elipsoid)
 * @param eph [m] Standard deviation of horizontal position error
 * @param epv [m] Standard deviation of vertical position error
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t id,uint64_t time_usec,uint8_t source,uint8_t flags,int32_t lat,int32_t lon,float alt,float alt_ellipsoid,float eph,float epv)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GLOBAL_POSITION_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_float(buf, 20, alt_ellipsoid);
    _mav_put_float(buf, 24, eph);
    _mav_put_float(buf, 28, epv);
    _mav_put_uint8_t(buf, 32, id);
    _mav_put_uint8_t(buf, 33, source);
    _mav_put_uint8_t(buf, 34, flags);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#else
    mavlink_global_position_t packet;
    packet.time_usec = time_usec;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.alt_ellipsoid = alt_ellipsoid;
    packet.eph = eph;
    packet.epv = epv;
    packet.id = id;
    packet.source = source;
    packet.flags = flags;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
}

/**
 * @brief Encode a global_position struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param global_position C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_global_position_t* global_position)
{
    return mavlink_msg_global_position_pack(system_id, component_id, msg, global_position->id, global_position->time_usec, global_position->source, global_position->flags, global_position->lat, global_position->lon, global_position->alt, global_position->alt_ellipsoid, global_position->eph, global_position->epv);
}

/**
 * @brief Encode a global_position struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param global_position C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_global_position_t* global_position)
{
    return mavlink_msg_global_position_pack_chan(system_id, component_id, chan, msg, global_position->id, global_position->time_usec, global_position->source, global_position->flags, global_position->lat, global_position->lon, global_position->alt, global_position->alt_ellipsoid, global_position->eph, global_position->epv);
}

/**
 * @brief Encode a global_position struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param global_position C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_global_position_t* global_position)
{
    return mavlink_msg_global_position_pack_status(system_id, component_id, _status, msg,  global_position->id, global_position->time_usec, global_position->source, global_position->flags, global_position->lat, global_position->lon, global_position->alt, global_position->alt_ellipsoid, global_position->eph, global_position->epv);
}

/**
 * @brief Send a global_position message
 * @param chan MAVLink channel to send the message
 *
 * @param id  Sensor ID
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @param source  Source of position/estimate (such as GNSS, estimator, etc.)
 * @param flags  Status flags
 * @param lat [degE7] Latitude (WGS84)
 * @param lon [degE7] Longitude (WGS84)
 * @param alt [m] Altitude (MSL - position-system specific value)
 * @param alt_ellipsoid [m] Altitude (WGS84 elipsoid)
 * @param eph [m] Standard deviation of horizontal position error
 * @param epv [m] Standard deviation of vertical position error
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_global_position_send(mavlink_channel_t chan, uint8_t id, uint64_t time_usec, uint8_t source, uint8_t flags, int32_t lat, int32_t lon, float alt, float alt_ellipsoid, float eph, float epv)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GLOBAL_POSITION_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_float(buf, 20, alt_ellipsoid);
    _mav_put_float(buf, 24, eph);
    _mav_put_float(buf, 28, epv);
    _mav_put_uint8_t(buf, 32, id);
    _mav_put_uint8_t(buf, 33, source);
    _mav_put_uint8_t(buf, 34, flags);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GLOBAL_POSITION, buf, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
#else
    mavlink_global_position_t packet;
    packet.time_usec = time_usec;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.alt_ellipsoid = alt_ellipsoid;
    packet.eph = eph;
    packet.epv = epv;
    packet.id = id;
    packet.source = source;
    packet.flags = flags;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GLOBAL_POSITION, (const char *)&packet, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
#endif
}

/**
 * @brief Send a global_position message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_global_position_send_struct(mavlink_channel_t chan, const mavlink_global_position_t* global_position)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_global_position_send(chan, global_position->id, global_position->time_usec, global_position->source, global_position->flags, global_position->lat, global_position->lon, global_position->alt, global_position->alt_ellipsoid, global_position->eph, global_position->epv);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GLOBAL_POSITION, (const char *)global_position, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
#endif
}

#if MAVLINK_MSG_ID_GLOBAL_POSITION_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_global_position_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t id, uint64_t time_usec, uint8_t source, uint8_t flags, int32_t lat, int32_t lon, float alt, float alt_ellipsoid, float eph, float epv)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_float(buf, 20, alt_ellipsoid);
    _mav_put_float(buf, 24, eph);
    _mav_put_float(buf, 28, epv);
    _mav_put_uint8_t(buf, 32, id);
    _mav_put_uint8_t(buf, 33, source);
    _mav_put_uint8_t(buf, 34, flags);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GLOBAL_POSITION, buf, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
#else
    mavlink_global_position_t *packet = (mavlink_global_position_t *)msgbuf;
    packet->time_usec = time_usec;
    packet->lat = lat;
    packet->lon = lon;
    packet->alt = alt;
    packet->alt_ellipsoid = alt_ellipsoid;
    packet->eph = eph;
    packet->epv = epv;
    packet->id = id;
    packet->source = source;
    packet->flags = flags;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GLOBAL_POSITION, (const char *)packet, MAVLINK_MSG_ID_GLOBAL_POSITION_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_CRC);
#endif
}
#endif

#endif

// MESSAGE GLOBAL_POSITION UNPACKING


/**
 * @brief Get field id from global_position message
 *
 * @return  Sensor ID
 */
static inline uint8_t mavlink_msg_global_position_get_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  32);
}

/**
 * @brief Get field time_usec from global_position message
 *
 * @return [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 */
static inline uint64_t mavlink_msg_global_position_get_time_usec(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field source from global_position message
 *
 * @return  Source of position/estimate (such as GNSS, estimator, etc.)
 */
static inline uint8_t mavlink_msg_global_position_get_source(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  33);
}

/**
 * @brief Get field flags from global_position message
 *
 * @return  Status flags
 */
static inline uint8_t mavlink_msg_global_position_get_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  34);
}

/**
 * @brief Get field lat from global_position message
 *
 * @return [degE7] Latitude (WGS84)
 */
static inline int32_t mavlink_msg_global_position_get_lat(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int32_t(msg,  8);
}

/**
 * @brief Get field lon from global_position message
 *
 * @return [degE7] Longitude (WGS84)
 */
static inline int32_t mavlink_msg_global_position_get_lon(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int32_t(msg,  12);
}

/**
 * @brief Get field alt from global_position message
 *
 * @return [m] Altitude (MSL - position-system specific value)
 */
static inline float mavlink_msg_global_position_get_alt(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field alt_ellipsoid from global_position message
 *
 * @return [m] Altitude (WGS84 elipsoid)
 */
static inline float mavlink_msg_global_position_get_alt_ellipsoid(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  20);
}

/**
 * @brief Get field eph from global_position message
 *
 * @return [m] Standard deviation of horizontal position error
 */
static inline float mavlink_msg_global_position_get_eph(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Get field epv from global_position message
 *
 * @return [m] Standard deviation of vertical position error
 */
static inline float mavlink_msg_global_position_get_epv(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Decode a global_position message into a struct
 *
 * @param msg The message to decode
 * @param global_position C-struct to decode the message contents into
 */
static inline void mavlink_msg_global_position_decode(const mavlink_message_t* msg, mavlink_global_position_t* global_position)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    global_position->time_usec = mavlink_msg_global_position_get_time_usec(msg);
    global_position->lat = mavlink_msg_global_position_get_lat(msg);
    global_position->lon = mavlink_msg_global_position_get_lon(msg);
    global_position->alt = mavlink_msg_global_position_get_alt(msg);
    global_position->alt_ellipsoid = mavlink_msg_global_position_get_alt_ellipsoid(msg);
    global_position->eph = mavlink_msg_global_position_get_eph(msg);
    global_position->epv = mavlink_msg_global_position_get_epv(msg);
    global_position->id = mavlink_msg_global_position_get_id(msg);
    global_position->source = mavlink_msg_global_position_get_source(msg);
    global_position->flags = mavlink_msg_global_position_get_flags(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_GLOBAL_POSITION_LEN? msg->len : MAVLINK_MSG_ID_GLOBAL_POSITION_LEN;
        memset(global_position, 0, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
    memcpy(global_position, _MAV_PAYLOAD(msg), len);
#endif
}
