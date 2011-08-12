// MESSAGE GLOBAL_POSITION PACKING

#define MAVLINK_MSG_ID_GLOBAL_POSITION 33
#define MAVLINK_MSG_ID_GLOBAL_POSITION_LEN 32
#define MAVLINK_MSG_33_LEN 32

typedef struct __mavlink_global_position_t 
{
	uint64_t usec; ///< Timestamp (microseconds since unix epoch)
	float lat; ///< Latitude, in degrees
	float lon; ///< Longitude, in degrees
	float alt; ///< Absolute altitude, in meters
	float vx; ///< X Speed (in Latitude direction, positive: going north)
	float vy; ///< Y Speed (in Longitude direction, positive: going east)
	float vz; ///< Z Speed (in Altitude direction, positive: going up)

} mavlink_global_position_t;

/**
 * @brief Pack a global_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds since unix epoch)
 * @param lat Latitude, in degrees
 * @param lon Longitude, in degrees
 * @param alt Absolute altitude, in meters
 * @param vx X Speed (in Latitude direction, positive: going north)
 * @param vy Y Speed (in Longitude direction, positive: going east)
 * @param vz Z Speed (in Altitude direction, positive: going up)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float lat, float lon, float alt, float vx, float vy, float vz)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION;

	p->usec = usec; // uint64_t:Timestamp (microseconds since unix epoch)
	p->lat = lat; // float:Latitude, in degrees
	p->lon = lon; // float:Longitude, in degrees
	p->alt = alt; // float:Absolute altitude, in meters
	p->vx = vx; // float:X Speed (in Latitude direction, positive: going north)
	p->vy = vy; // float:Y Speed (in Longitude direction, positive: going east)
	p->vz = vz; // float:Z Speed (in Altitude direction, positive: going up)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
}

/**
 * @brief Pack a global_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds since unix epoch)
 * @param lat Latitude, in degrees
 * @param lon Longitude, in degrees
 * @param alt Absolute altitude, in meters
 * @param vx X Speed (in Latitude direction, positive: going north)
 * @param vy Y Speed (in Longitude direction, positive: going east)
 * @param vz Z Speed (in Altitude direction, positive: going up)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float lat, float lon, float alt, float vx, float vy, float vz)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION;

	p->usec = usec; // uint64_t:Timestamp (microseconds since unix epoch)
	p->lat = lat; // float:Latitude, in degrees
	p->lon = lon; // float:Longitude, in degrees
	p->alt = alt; // float:Absolute altitude, in meters
	p->vx = vx; // float:X Speed (in Latitude direction, positive: going north)
	p->vy = vy; // float:Y Speed (in Longitude direction, positive: going east)
	p->vz = vz; // float:Z Speed (in Altitude direction, positive: going up)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GLOBAL_POSITION_LEN);
}

/**
 * @brief Encode a global_position struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param global_position C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_global_position_t* global_position)
{
	return mavlink_msg_global_position_pack(system_id, component_id, msg, global_position->usec, global_position->lat, global_position->lon, global_position->alt, global_position->vx, global_position->vy, global_position->vz);
}

/**
 * @brief Send a global_position message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since unix epoch)
 * @param lat Latitude, in degrees
 * @param lon Longitude, in degrees
 * @param alt Absolute altitude, in meters
 * @param vx X Speed (in Latitude direction, positive: going north)
 * @param vy Y Speed (in Longitude direction, positive: going east)
 * @param vz Z Speed (in Altitude direction, positive: going up)
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_global_position_send(mavlink_channel_t chan, uint64_t usec, float lat, float lon, float alt, float vx, float vy, float vz)
{
	mavlink_header_t hdr;
	mavlink_global_position_t payload;
	uint16_t checksum;
	mavlink_global_position_t *p = &payload;

	p->usec = usec; // uint64_t:Timestamp (microseconds since unix epoch)
	p->lat = lat; // float:Latitude, in degrees
	p->lon = lon; // float:Longitude, in degrees
	p->alt = alt; // float:Absolute altitude, in meters
	p->vx = vx; // float:X Speed (in Latitude direction, positive: going north)
	p->vy = vy; // float:Y Speed (in Longitude direction, positive: going east)
	p->vz = vz; // float:Z Speed (in Altitude direction, positive: going up)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GLOBAL_POSITION_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GLOBAL_POSITION;
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
// MESSAGE GLOBAL_POSITION UNPACKING

/**
 * @brief Get field usec from global_position message
 *
 * @return Timestamp (microseconds since unix epoch)
 */
static inline uint64_t mavlink_msg_global_position_get_usec(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field lat from global_position message
 *
 * @return Latitude, in degrees
 */
static inline float mavlink_msg_global_position_get_lat(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (float)(p->lat);
}

/**
 * @brief Get field lon from global_position message
 *
 * @return Longitude, in degrees
 */
static inline float mavlink_msg_global_position_get_lon(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (float)(p->lon);
}

/**
 * @brief Get field alt from global_position message
 *
 * @return Absolute altitude, in meters
 */
static inline float mavlink_msg_global_position_get_alt(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (float)(p->alt);
}

/**
 * @brief Get field vx from global_position message
 *
 * @return X Speed (in Latitude direction, positive: going north)
 */
static inline float mavlink_msg_global_position_get_vx(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (float)(p->vx);
}

/**
 * @brief Get field vy from global_position message
 *
 * @return Y Speed (in Longitude direction, positive: going east)
 */
static inline float mavlink_msg_global_position_get_vy(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (float)(p->vy);
}

/**
 * @brief Get field vz from global_position message
 *
 * @return Z Speed (in Altitude direction, positive: going up)
 */
static inline float mavlink_msg_global_position_get_vz(const mavlink_message_t* msg)
{
	mavlink_global_position_t *p = (mavlink_global_position_t *)&msg->payload[0];
	return (float)(p->vz);
}

/**
 * @brief Decode a global_position message into a struct
 *
 * @param msg The message to decode
 * @param global_position C-struct to decode the message contents into
 */
static inline void mavlink_msg_global_position_decode(const mavlink_message_t* msg, mavlink_global_position_t* global_position)
{
	memcpy( global_position, msg->payload, sizeof(mavlink_global_position_t));
}
