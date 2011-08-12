// MESSAGE POINT_OF_INTEREST_CONNECTION PACKING

#define MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION 162
#define MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION_LEN 55
#define MAVLINK_MSG_162_LEN 55

typedef struct __mavlink_point_of_interest_connection_t 
{
	float xp1; ///< X1 Position
	float yp1; ///< Y1 Position
	float zp1; ///< Z1 Position
	float xp2; ///< X2 Position
	float yp2; ///< Y2 Position
	float zp2; ///< Z2 Position
	uint16_t timeout; ///< 0: no timeout, >1: timeout in seconds
	uint8_t type; ///< 0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
	uint8_t color; ///< 0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
	uint8_t coordinate_system; ///< 0: global, 1:local
	char name[26]; ///< POI connection name

} mavlink_point_of_interest_connection_t;
#define MAVLINK_MSG_POINT_OF_INTEREST_CONNECTION_FIELD_NAME_LEN 26

/**
 * @brief Pack a point_of_interest_connection message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param type 0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
 * @param color 0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
 * @param coordinate_system 0: global, 1:local
 * @param timeout 0: no timeout, >1: timeout in seconds
 * @param xp1 X1 Position
 * @param yp1 Y1 Position
 * @param zp1 Z1 Position
 * @param xp2 X2 Position
 * @param yp2 Y2 Position
 * @param zp2 Z2 Position
 * @param name POI connection name
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_point_of_interest_connection_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t type, uint8_t color, uint8_t coordinate_system, uint16_t timeout, float xp1, float yp1, float zp1, float xp2, float yp2, float zp2, const char* name)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION;

	p->type = type; // uint8_t:0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
	p->color = color; // uint8_t:0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
	p->coordinate_system = coordinate_system; // uint8_t:0: global, 1:local
	p->timeout = timeout; // uint16_t:0: no timeout, >1: timeout in seconds
	p->xp1 = xp1; // float:X1 Position
	p->yp1 = yp1; // float:Y1 Position
	p->zp1 = zp1; // float:Z1 Position
	p->xp2 = xp2; // float:X2 Position
	p->yp2 = yp2; // float:Y2 Position
	p->zp2 = zp2; // float:Z2 Position
	memcpy(p->name, name, sizeof(p->name)); // char[26]:POI connection name

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION_LEN);
}

/**
 * @brief Pack a point_of_interest_connection message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param type 0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
 * @param color 0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
 * @param coordinate_system 0: global, 1:local
 * @param timeout 0: no timeout, >1: timeout in seconds
 * @param xp1 X1 Position
 * @param yp1 Y1 Position
 * @param zp1 Z1 Position
 * @param xp2 X2 Position
 * @param yp2 Y2 Position
 * @param zp2 Z2 Position
 * @param name POI connection name
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_point_of_interest_connection_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t type, uint8_t color, uint8_t coordinate_system, uint16_t timeout, float xp1, float yp1, float zp1, float xp2, float yp2, float zp2, const char* name)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION;

	p->type = type; // uint8_t:0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
	p->color = color; // uint8_t:0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
	p->coordinate_system = coordinate_system; // uint8_t:0: global, 1:local
	p->timeout = timeout; // uint16_t:0: no timeout, >1: timeout in seconds
	p->xp1 = xp1; // float:X1 Position
	p->yp1 = yp1; // float:Y1 Position
	p->zp1 = zp1; // float:Z1 Position
	p->xp2 = xp2; // float:X2 Position
	p->yp2 = yp2; // float:Y2 Position
	p->zp2 = zp2; // float:Z2 Position
	memcpy(p->name, name, sizeof(p->name)); // char[26]:POI connection name

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION_LEN);
}

/**
 * @brief Encode a point_of_interest_connection struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param point_of_interest_connection C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_point_of_interest_connection_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_point_of_interest_connection_t* point_of_interest_connection)
{
	return mavlink_msg_point_of_interest_connection_pack(system_id, component_id, msg, point_of_interest_connection->type, point_of_interest_connection->color, point_of_interest_connection->coordinate_system, point_of_interest_connection->timeout, point_of_interest_connection->xp1, point_of_interest_connection->yp1, point_of_interest_connection->zp1, point_of_interest_connection->xp2, point_of_interest_connection->yp2, point_of_interest_connection->zp2, point_of_interest_connection->name);
}

/**
 * @brief Send a point_of_interest_connection message
 * @param chan MAVLink channel to send the message
 *
 * @param type 0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
 * @param color 0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
 * @param coordinate_system 0: global, 1:local
 * @param timeout 0: no timeout, >1: timeout in seconds
 * @param xp1 X1 Position
 * @param yp1 Y1 Position
 * @param zp1 Z1 Position
 * @param xp2 X2 Position
 * @param yp2 Y2 Position
 * @param zp2 Z2 Position
 * @param name POI connection name
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_point_of_interest_connection_send(mavlink_channel_t chan, uint8_t type, uint8_t color, uint8_t coordinate_system, uint16_t timeout, float xp1, float yp1, float zp1, float xp2, float yp2, float zp2, const char* name)
{
	mavlink_header_t hdr;
	mavlink_point_of_interest_connection_t payload;
	uint16_t checksum;
	mavlink_point_of_interest_connection_t *p = &payload;

	p->type = type; // uint8_t:0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
	p->color = color; // uint8_t:0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
	p->coordinate_system = coordinate_system; // uint8_t:0: global, 1:local
	p->timeout = timeout; // uint16_t:0: no timeout, >1: timeout in seconds
	p->xp1 = xp1; // float:X1 Position
	p->yp1 = yp1; // float:Y1 Position
	p->zp1 = zp1; // float:Z1 Position
	p->xp2 = xp2; // float:X2 Position
	p->yp2 = yp2; // float:Y2 Position
	p->zp2 = zp2; // float:Z2 Position
	memcpy(p->name, name, sizeof(p->name)); // char[26]:POI connection name

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION_LEN;
	hdr.msgid = MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION;
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
// MESSAGE POINT_OF_INTEREST_CONNECTION UNPACKING

/**
 * @brief Get field type from point_of_interest_connection message
 *
 * @return 0: Notice, 1: Warning, 2: Critical, 3: Emergency, 4: Debug
 */
static inline uint8_t mavlink_msg_point_of_interest_connection_get_type(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (uint8_t)(p->type);
}

/**
 * @brief Get field color from point_of_interest_connection message
 *
 * @return 0: blue, 1: yellow, 2: red, 3: orange, 4: green, 5: magenta
 */
static inline uint8_t mavlink_msg_point_of_interest_connection_get_color(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (uint8_t)(p->color);
}

/**
 * @brief Get field coordinate_system from point_of_interest_connection message
 *
 * @return 0: global, 1:local
 */
static inline uint8_t mavlink_msg_point_of_interest_connection_get_coordinate_system(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (uint8_t)(p->coordinate_system);
}

/**
 * @brief Get field timeout from point_of_interest_connection message
 *
 * @return 0: no timeout, >1: timeout in seconds
 */
static inline uint16_t mavlink_msg_point_of_interest_connection_get_timeout(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (uint16_t)(p->timeout);
}

/**
 * @brief Get field xp1 from point_of_interest_connection message
 *
 * @return X1 Position
 */
static inline float mavlink_msg_point_of_interest_connection_get_xp1(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (float)(p->xp1);
}

/**
 * @brief Get field yp1 from point_of_interest_connection message
 *
 * @return Y1 Position
 */
static inline float mavlink_msg_point_of_interest_connection_get_yp1(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (float)(p->yp1);
}

/**
 * @brief Get field zp1 from point_of_interest_connection message
 *
 * @return Z1 Position
 */
static inline float mavlink_msg_point_of_interest_connection_get_zp1(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (float)(p->zp1);
}

/**
 * @brief Get field xp2 from point_of_interest_connection message
 *
 * @return X2 Position
 */
static inline float mavlink_msg_point_of_interest_connection_get_xp2(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (float)(p->xp2);
}

/**
 * @brief Get field yp2 from point_of_interest_connection message
 *
 * @return Y2 Position
 */
static inline float mavlink_msg_point_of_interest_connection_get_yp2(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (float)(p->yp2);
}

/**
 * @brief Get field zp2 from point_of_interest_connection message
 *
 * @return Z2 Position
 */
static inline float mavlink_msg_point_of_interest_connection_get_zp2(const mavlink_message_t* msg)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];
	return (float)(p->zp2);
}

/**
 * @brief Get field name from point_of_interest_connection message
 *
 * @return POI connection name
 */
static inline uint16_t mavlink_msg_point_of_interest_connection_get_name(const mavlink_message_t* msg, char* name)
{
	mavlink_point_of_interest_connection_t *p = (mavlink_point_of_interest_connection_t *)&msg->payload[0];

	memcpy(name, p->name, sizeof(p->name));
	return sizeof(p->name);
}

/**
 * @brief Decode a point_of_interest_connection message into a struct
 *
 * @param msg The message to decode
 * @param point_of_interest_connection C-struct to decode the message contents into
 */
static inline void mavlink_msg_point_of_interest_connection_decode(const mavlink_message_t* msg, mavlink_point_of_interest_connection_t* point_of_interest_connection)
{
	memcpy( point_of_interest_connection, msg->payload, sizeof(mavlink_point_of_interest_connection_t));
}
