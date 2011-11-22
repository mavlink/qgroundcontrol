// MESSAGE PM_ELEC PACKING

#define MAVLINK_MSG_ID_PM_ELEC 188

typedef struct __mavlink_pm_elec_t 
{
	float PwCons; ///< current power consumption
	float BatStat; ///< battery status
	float PwGen[3]; ///< Power generation from each module

} mavlink_pm_elec_t;

#define MAVLINK_MSG_PM_ELEC_FIELD_PWGEN_LEN 3


/**
 * @brief Pack a pm_elec message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param PwCons current power consumption
 * @param BatStat battery status
 * @param PwGen Power generation from each module
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_pm_elec_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float PwCons, float BatStat, const float* PwGen)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_PM_ELEC;

	i += put_float_by_index(PwCons, i, msg->payload); // current power consumption
	i += put_float_by_index(BatStat, i, msg->payload); // battery status
	i += put_array_by_index((const int8_t*)PwGen, sizeof(float)*3, i, msg->payload); // Power generation from each module

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a pm_elec message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param PwCons current power consumption
 * @param BatStat battery status
 * @param PwGen Power generation from each module
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_pm_elec_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float PwCons, float BatStat, const float* PwGen)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_PM_ELEC;

	i += put_float_by_index(PwCons, i, msg->payload); // current power consumption
	i += put_float_by_index(BatStat, i, msg->payload); // battery status
	i += put_array_by_index((const int8_t*)PwGen, sizeof(float)*3, i, msg->payload); // Power generation from each module

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a pm_elec struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param pm_elec C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_pm_elec_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_pm_elec_t* pm_elec)
{
	return mavlink_msg_pm_elec_pack(system_id, component_id, msg, pm_elec->PwCons, pm_elec->BatStat, pm_elec->PwGen);
}

/**
 * @brief Send a pm_elec message
 * @param chan MAVLink channel to send the message
 *
 * @param PwCons current power consumption
 * @param BatStat battery status
 * @param PwGen Power generation from each module
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_pm_elec_send(mavlink_channel_t chan, float PwCons, float BatStat, const float* PwGen)
{
	mavlink_message_t msg;
	mavlink_msg_pm_elec_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, PwCons, BatStat, PwGen);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE PM_ELEC UNPACKING

/**
 * @brief Get field PwCons from pm_elec message
 *
 * @return current power consumption
 */
static inline float mavlink_msg_pm_elec_get_PwCons(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (float)r.f;
}

/**
 * @brief Get field BatStat from pm_elec message
 *
 * @return battery status
 */
static inline float mavlink_msg_pm_elec_get_BatStat(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field PwGen from pm_elec message
 *
 * @return Power generation from each module
 */
static inline uint16_t mavlink_msg_pm_elec_get_PwGen(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload+sizeof(float)+sizeof(float), sizeof(float)*3);
	return sizeof(float)*3;
}

/**
 * @brief Decode a pm_elec message into a struct
 *
 * @param msg The message to decode
 * @param pm_elec C-struct to decode the message contents into
 */
static inline void mavlink_msg_pm_elec_decode(const mavlink_message_t* msg, mavlink_pm_elec_t* pm_elec)
{
	pm_elec->PwCons = mavlink_msg_pm_elec_get_PwCons(msg);
	pm_elec->BatStat = mavlink_msg_pm_elec_get_BatStat(msg);
	mavlink_msg_pm_elec_get_PwGen(msg, pm_elec->PwGen);
}
