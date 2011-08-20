// MESSAGE BRIEF_FEATURE PACKING

#define MAVLINK_MSG_ID_BRIEF_FEATURE 172
#define MAVLINK_MSG_ID_BRIEF_FEATURE_LEN 53
#define MAVLINK_MSG_172_LEN 53
#define MAVLINK_MSG_ID_BRIEF_FEATURE_KEY 0xD9
#define MAVLINK_MSG_172_KEY 0xD9

typedef struct __mavlink_brief_feature_t 
{
	float x;	///< x position in m
	float y;	///< y position in m
	float z;	///< z position in m
	float response;	///< Harris operator response at this location
	uint16_t size;	///< Size in pixels
	uint16_t orientation;	///< Orientation
	uint8_t orientation_assignment;	///< Orientation assignment 0: false, 1:true
	uint8_t descriptor[32];	///< Descriptor

} mavlink_brief_feature_t;
#define MAVLINK_MSG_BRIEF_FEATURE_FIELD_DESCRIPTOR_LEN 32

/**
 * @brief Pack a brief_feature message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param x x position in m
 * @param y y position in m
 * @param z z position in m
 * @param orientation_assignment Orientation assignment 0: false, 1:true
 * @param size Size in pixels
 * @param orientation Orientation
 * @param descriptor Descriptor
 * @param response Harris operator response at this location
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_brief_feature_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float x, float y, float z, uint8_t orientation_assignment, uint16_t size, uint16_t orientation, const uint8_t* descriptor, float response)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_BRIEF_FEATURE;

	p->x = x;	// float:x position in m
	p->y = y;	// float:y position in m
	p->z = z;	// float:z position in m
	p->orientation_assignment = orientation_assignment;	// uint8_t:Orientation assignment 0: false, 1:true
	p->size = size;	// uint16_t:Size in pixels
	p->orientation = orientation;	// uint16_t:Orientation
	memcpy(p->descriptor, descriptor, sizeof(p->descriptor));	// uint8_t[32]:Descriptor
	p->response = response;	// float:Harris operator response at this location

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_BRIEF_FEATURE_LEN);
}

/**
 * @brief Pack a brief_feature message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param x x position in m
 * @param y y position in m
 * @param z z position in m
 * @param orientation_assignment Orientation assignment 0: false, 1:true
 * @param size Size in pixels
 * @param orientation Orientation
 * @param descriptor Descriptor
 * @param response Harris operator response at this location
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_brief_feature_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float x, float y, float z, uint8_t orientation_assignment, uint16_t size, uint16_t orientation, const uint8_t* descriptor, float response)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_BRIEF_FEATURE;

	p->x = x;	// float:x position in m
	p->y = y;	// float:y position in m
	p->z = z;	// float:z position in m
	p->orientation_assignment = orientation_assignment;	// uint8_t:Orientation assignment 0: false, 1:true
	p->size = size;	// uint16_t:Size in pixels
	p->orientation = orientation;	// uint16_t:Orientation
	memcpy(p->descriptor, descriptor, sizeof(p->descriptor));	// uint8_t[32]:Descriptor
	p->response = response;	// float:Harris operator response at this location

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_BRIEF_FEATURE_LEN);
}

/**
 * @brief Encode a brief_feature struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param brief_feature C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_brief_feature_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_brief_feature_t* brief_feature)
{
	return mavlink_msg_brief_feature_pack(system_id, component_id, msg, brief_feature->x, brief_feature->y, brief_feature->z, brief_feature->orientation_assignment, brief_feature->size, brief_feature->orientation, brief_feature->descriptor, brief_feature->response);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a brief_feature message
 * @param chan MAVLink channel to send the message
 *
 * @param x x position in m
 * @param y y position in m
 * @param z z position in m
 * @param orientation_assignment Orientation assignment 0: false, 1:true
 * @param size Size in pixels
 * @param orientation Orientation
 * @param descriptor Descriptor
 * @param response Harris operator response at this location
 */
static inline void mavlink_msg_brief_feature_send(mavlink_channel_t chan, float x, float y, float z, uint8_t orientation_assignment, uint16_t size, uint16_t orientation, const uint8_t* descriptor, float response)
{
	mavlink_header_t hdr;
	mavlink_brief_feature_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_BRIEF_FEATURE_LEN )
	payload.x = x;	// float:x position in m
	payload.y = y;	// float:y position in m
	payload.z = z;	// float:z position in m
	payload.orientation_assignment = orientation_assignment;	// uint8_t:Orientation assignment 0: false, 1:true
	payload.size = size;	// uint16_t:Size in pixels
	payload.orientation = orientation;	// uint16_t:Orientation
	memcpy(payload.descriptor, descriptor, sizeof(payload.descriptor));	// uint8_t[32]:Descriptor
	payload.response = response;	// float:Harris operator response at this location

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_BRIEF_FEATURE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_BRIEF_FEATURE;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xD9, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE BRIEF_FEATURE UNPACKING

/**
 * @brief Get field x from brief_feature message
 *
 * @return x position in m
 */
static inline float mavlink_msg_brief_feature_get_x(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from brief_feature message
 *
 * @return y position in m
 */
static inline float mavlink_msg_brief_feature_get_y(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from brief_feature message
 *
 * @return z position in m
 */
static inline float mavlink_msg_brief_feature_get_z(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Get field orientation_assignment from brief_feature message
 *
 * @return Orientation assignment 0: false, 1:true
 */
static inline uint8_t mavlink_msg_brief_feature_get_orientation_assignment(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (uint8_t)(p->orientation_assignment);
}

/**
 * @brief Get field size from brief_feature message
 *
 * @return Size in pixels
 */
static inline uint16_t mavlink_msg_brief_feature_get_size(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (uint16_t)(p->size);
}

/**
 * @brief Get field orientation from brief_feature message
 *
 * @return Orientation
 */
static inline uint16_t mavlink_msg_brief_feature_get_orientation(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (uint16_t)(p->orientation);
}

/**
 * @brief Get field descriptor from brief_feature message
 *
 * @return Descriptor
 */
static inline uint16_t mavlink_msg_brief_feature_get_descriptor(const mavlink_message_t* msg, uint8_t* descriptor)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];

	memcpy(descriptor, p->descriptor, sizeof(p->descriptor));
	return sizeof(p->descriptor);
}

/**
 * @brief Get field response from brief_feature message
 *
 * @return Harris operator response at this location
 */
static inline float mavlink_msg_brief_feature_get_response(const mavlink_message_t* msg)
{
	mavlink_brief_feature_t *p = (mavlink_brief_feature_t *)&msg->payload[0];
	return (float)(p->response);
}

/**
 * @brief Decode a brief_feature message into a struct
 *
 * @param msg The message to decode
 * @param brief_feature C-struct to decode the message contents into
 */
static inline void mavlink_msg_brief_feature_decode(const mavlink_message_t* msg, mavlink_brief_feature_t* brief_feature)
{
	memcpy( brief_feature, msg->payload, sizeof(mavlink_brief_feature_t));
}
