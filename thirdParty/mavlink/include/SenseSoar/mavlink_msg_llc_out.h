// MESSAGE LLC_OUT PACKING

#define MAVLINK_MSG_ID_LLC_OUT 186

typedef struct __mavlink_llc_out_t 
{
	int16_t servoOut[4]; ///< Servo signal
	int16_t MotorOut[2]; ///< motor signal

} mavlink_llc_out_t;

#define MAVLINK_MSG_LLC_OUT_FIELD_SERVOOUT_LEN 4
#define MAVLINK_MSG_LLC_OUT_FIELD_MOTOROUT_LEN 2


/**
 * @brief Pack a llc_out message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param servoOut Servo signal
 * @param MotorOut motor signal
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_llc_out_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const int16_t* servoOut, const int16_t* MotorOut)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_LLC_OUT;

	i += put_array_by_index((const int8_t*)servoOut, sizeof(int16_t)*4, i, msg->payload); // Servo signal
	i += put_array_by_index((const int8_t*)MotorOut, sizeof(int16_t)*2, i, msg->payload); // motor signal

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a llc_out message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param servoOut Servo signal
 * @param MotorOut motor signal
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_llc_out_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const int16_t* servoOut, const int16_t* MotorOut)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_LLC_OUT;

	i += put_array_by_index((const int8_t*)servoOut, sizeof(int16_t)*4, i, msg->payload); // Servo signal
	i += put_array_by_index((const int8_t*)MotorOut, sizeof(int16_t)*2, i, msg->payload); // motor signal

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a llc_out struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param llc_out C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_llc_out_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_llc_out_t* llc_out)
{
	return mavlink_msg_llc_out_pack(system_id, component_id, msg, llc_out->servoOut, llc_out->MotorOut);
}

/**
 * @brief Send a llc_out message
 * @param chan MAVLink channel to send the message
 *
 * @param servoOut Servo signal
 * @param MotorOut motor signal
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_llc_out_send(mavlink_channel_t chan, const int16_t* servoOut, const int16_t* MotorOut)
{
	mavlink_message_t msg;
	mavlink_msg_llc_out_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, servoOut, MotorOut);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE LLC_OUT UNPACKING

/**
 * @brief Get field servoOut from llc_out message
 *
 * @return Servo signal
 */
static inline uint16_t mavlink_msg_llc_out_get_servoOut(const mavlink_message_t* msg, int16_t* r_data)
{

	memcpy(r_data, msg->payload, sizeof(int16_t)*4);
	return sizeof(int16_t)*4;
}

/**
 * @brief Get field MotorOut from llc_out message
 *
 * @return motor signal
 */
static inline uint16_t mavlink_msg_llc_out_get_MotorOut(const mavlink_message_t* msg, int16_t* r_data)
{

	memcpy(r_data, msg->payload+sizeof(int16_t)*4, sizeof(int16_t)*2);
	return sizeof(int16_t)*2;
}

/**
 * @brief Decode a llc_out message into a struct
 *
 * @param msg The message to decode
 * @param llc_out C-struct to decode the message contents into
 */
static inline void mavlink_msg_llc_out_decode(const mavlink_message_t* msg, mavlink_llc_out_t* llc_out)
{
	mavlink_msg_llc_out_get_servoOut(msg, llc_out->servoOut);
	mavlink_msg_llc_out_get_MotorOut(msg, llc_out->MotorOut);
}
