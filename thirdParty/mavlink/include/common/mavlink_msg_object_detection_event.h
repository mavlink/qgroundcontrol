// MESSAGE OBJECT_DETECTION_EVENT PACKING

#define MAVLINK_MSG_ID_OBJECT_DETECTION_EVENT 140

typedef struct __mavlink_object_detection_event_t 
{
	uint32_t time; ///< Timestamp in milliseconds since system boot
	uint16_t object_id; ///< Object ID
	uint8_t type; ///< Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
	char name[20]; ///< Name of the object as defined by the detector
	uint8_t quality; ///< Detection quality / confidence. 0: bad, 255: maximum confidence
	float bearing; ///< Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
	float distance; ///< Ground distance in meters

} mavlink_object_detection_event_t;

#define MAVLINK_MSG_OBJECT_DETECTION_EVENT_FIELD_NAME_LEN 20


/**
 * @brief Pack a object_detection_event message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time Timestamp in milliseconds since system boot
 * @param object_id Object ID
 * @param type Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
 * @param name Name of the object as defined by the detector
 * @param quality Detection quality / confidence. 0: bad, 255: maximum confidence
 * @param bearing Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
 * @param distance Ground distance in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_object_detection_event_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint32_t time, uint16_t object_id, uint8_t type, const char* name, uint8_t quality, float bearing, float distance)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBJECT_DETECTION_EVENT;

	i += put_uint32_t_by_index(time, i, msg->payload); // Timestamp in milliseconds since system boot
	i += put_uint16_t_by_index(object_id, i, msg->payload); // Object ID
	i += put_uint8_t_by_index(type, i, msg->payload); // Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
	i += put_array_by_index((const int8_t*)name, sizeof(char)*20, i, msg->payload); // Name of the object as defined by the detector
	i += put_uint8_t_by_index(quality, i, msg->payload); // Detection quality / confidence. 0: bad, 255: maximum confidence
	i += put_float_by_index(bearing, i, msg->payload); // Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
	i += put_float_by_index(distance, i, msg->payload); // Ground distance in meters

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a object_detection_event message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time Timestamp in milliseconds since system boot
 * @param object_id Object ID
 * @param type Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
 * @param name Name of the object as defined by the detector
 * @param quality Detection quality / confidence. 0: bad, 255: maximum confidence
 * @param bearing Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
 * @param distance Ground distance in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_object_detection_event_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint32_t time, uint16_t object_id, uint8_t type, const char* name, uint8_t quality, float bearing, float distance)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBJECT_DETECTION_EVENT;

	i += put_uint32_t_by_index(time, i, msg->payload); // Timestamp in milliseconds since system boot
	i += put_uint16_t_by_index(object_id, i, msg->payload); // Object ID
	i += put_uint8_t_by_index(type, i, msg->payload); // Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
	i += put_array_by_index((const int8_t*)name, sizeof(char)*20, i, msg->payload); // Name of the object as defined by the detector
	i += put_uint8_t_by_index(quality, i, msg->payload); // Detection quality / confidence. 0: bad, 255: maximum confidence
	i += put_float_by_index(bearing, i, msg->payload); // Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
	i += put_float_by_index(distance, i, msg->payload); // Ground distance in meters

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a object_detection_event struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param object_detection_event C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_object_detection_event_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_object_detection_event_t* object_detection_event)
{
	return mavlink_msg_object_detection_event_pack(system_id, component_id, msg, object_detection_event->time, object_detection_event->object_id, object_detection_event->type, object_detection_event->name, object_detection_event->quality, object_detection_event->bearing, object_detection_event->distance);
}

/**
 * @brief Send a object_detection_event message
 * @param chan MAVLink channel to send the message
 *
 * @param time Timestamp in milliseconds since system boot
 * @param object_id Object ID
 * @param type Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
 * @param name Name of the object as defined by the detector
 * @param quality Detection quality / confidence. 0: bad, 255: maximum confidence
 * @param bearing Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
 * @param distance Ground distance in meters
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_object_detection_event_send(mavlink_channel_t chan, uint32_t time, uint16_t object_id, uint8_t type, const char* name, uint8_t quality, float bearing, float distance)
{
	mavlink_message_t msg;
	mavlink_msg_object_detection_event_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, time, object_id, type, name, quality, bearing, distance);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBJECT_DETECTION_EVENT UNPACKING

/**
 * @brief Get field time from object_detection_event message
 *
 * @return Timestamp in milliseconds since system boot
 */
static inline uint32_t mavlink_msg_object_detection_event_get_time(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (uint32_t)r.i;
}

/**
 * @brief Get field object_id from object_detection_event message
 *
 * @return Object ID
 */
static inline uint16_t mavlink_msg_object_detection_event_get_object_id(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint32_t))[0];
	r.b[0] = (msg->payload+sizeof(uint32_t))[1];
	return (uint16_t)r.s;
}

/**
 * @brief Get field type from object_detection_event message
 *
 * @return Object type: 0: image, 1: letter, 2: ground vehicle, 3: air vehicle, 4: surface vehicle, 5: sub-surface vehicle, 6: human, 7: animal
 */
static inline uint8_t mavlink_msg_object_detection_event_get_type(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint32_t)+sizeof(uint16_t))[0];
}

/**
 * @brief Get field name from object_detection_event message
 *
 * @return Name of the object as defined by the detector
 */
static inline uint16_t mavlink_msg_object_detection_event_get_name(const mavlink_message_t* msg, char* r_data)
{

	memcpy(r_data, msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t), sizeof(char)*20);
	return sizeof(char)*20;
}

/**
 * @brief Get field quality from object_detection_event message
 *
 * @return Detection quality / confidence. 0: bad, 255: maximum confidence
 */
static inline uint8_t mavlink_msg_object_detection_event_get_quality(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20)[0];
}

/**
 * @brief Get field bearing from object_detection_event message
 *
 * @return Angle of the object with respect to the body frame in NED coordinates in radians. 0: front
 */
static inline float mavlink_msg_object_detection_event_get_bearing(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t))[0];
	r.b[2] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t))[1];
	r.b[1] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t))[2];
	r.b[0] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t))[3];
	return (float)r.f;
}

/**
 * @brief Get field distance from object_detection_event message
 *
 * @return Ground distance in meters
 */
static inline float mavlink_msg_object_detection_event_get_distance(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(char)*20+sizeof(uint8_t)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Decode a object_detection_event message into a struct
 *
 * @param msg The message to decode
 * @param object_detection_event C-struct to decode the message contents into
 */
static inline void mavlink_msg_object_detection_event_decode(const mavlink_message_t* msg, mavlink_object_detection_event_t* object_detection_event)
{
	object_detection_event->time = mavlink_msg_object_detection_event_get_time(msg);
	object_detection_event->object_id = mavlink_msg_object_detection_event_get_object_id(msg);
	object_detection_event->type = mavlink_msg_object_detection_event_get_type(msg);
	mavlink_msg_object_detection_event_get_name(msg, object_detection_event->name);
	object_detection_event->quality = mavlink_msg_object_detection_event_get_quality(msg);
	object_detection_event->bearing = mavlink_msg_object_detection_event_get_bearing(msg);
	object_detection_event->distance = mavlink_msg_object_detection_event_get_distance(msg);
}
