// MESSAGE GPS_DATE_TIME PACKING

#define MAVLINK_MSG_ID_GPS_DATE_TIME 179
#define MAVLINK_MSG_ID_GPS_DATE_TIME_LEN 7
#define MAVLINK_MSG_179_LEN 7
#define MAVLINK_MSG_ID_GPS_DATE_TIME_KEY 0xE
#define MAVLINK_MSG_179_KEY 0xE

typedef struct __mavlink_gps_date_time_t 
{
	uint8_t year;	///< Year reported by Gps 
	uint8_t month;	///< Month reported by Gps 
	uint8_t day;	///< Day reported by Gps 
	uint8_t hour;	///< Hour reported by Gps 
	uint8_t min;	///< Min reported by Gps 
	uint8_t sec;	///< Sec reported by Gps  
	uint8_t visSat;	///< Visible sattelites reported by Gps  

} mavlink_gps_date_time_t;

/**
 * @brief Pack a gps_date_time message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param year Year reported by Gps 
 * @param month Month reported by Gps 
 * @param day Day reported by Gps 
 * @param hour Hour reported by Gps 
 * @param min Min reported by Gps 
 * @param sec Sec reported by Gps  
 * @param visSat Visible sattelites reported by Gps  
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_date_time_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t visSat)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_DATE_TIME;

	p->year = year;	// uint8_t:Year reported by Gps 
	p->month = month;	// uint8_t:Month reported by Gps 
	p->day = day;	// uint8_t:Day reported by Gps 
	p->hour = hour;	// uint8_t:Hour reported by Gps 
	p->min = min;	// uint8_t:Min reported by Gps 
	p->sec = sec;	// uint8_t:Sec reported by Gps  
	p->visSat = visSat;	// uint8_t:Visible sattelites reported by Gps  

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GPS_DATE_TIME_LEN);
}

/**
 * @brief Pack a gps_date_time message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param year Year reported by Gps 
 * @param month Month reported by Gps 
 * @param day Day reported by Gps 
 * @param hour Hour reported by Gps 
 * @param min Min reported by Gps 
 * @param sec Sec reported by Gps  
 * @param visSat Visible sattelites reported by Gps  
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_date_time_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t visSat)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_DATE_TIME;

	p->year = year;	// uint8_t:Year reported by Gps 
	p->month = month;	// uint8_t:Month reported by Gps 
	p->day = day;	// uint8_t:Day reported by Gps 
	p->hour = hour;	// uint8_t:Hour reported by Gps 
	p->min = min;	// uint8_t:Min reported by Gps 
	p->sec = sec;	// uint8_t:Sec reported by Gps  
	p->visSat = visSat;	// uint8_t:Visible sattelites reported by Gps  

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GPS_DATE_TIME_LEN);
}

/**
 * @brief Encode a gps_date_time struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gps_date_time C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gps_date_time_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gps_date_time_t* gps_date_time)
{
	return mavlink_msg_gps_date_time_pack(system_id, component_id, msg, gps_date_time->year, gps_date_time->month, gps_date_time->day, gps_date_time->hour, gps_date_time->min, gps_date_time->sec, gps_date_time->visSat);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a gps_date_time message
 * @param chan MAVLink channel to send the message
 *
 * @param year Year reported by Gps 
 * @param month Month reported by Gps 
 * @param day Day reported by Gps 
 * @param hour Hour reported by Gps 
 * @param min Min reported by Gps 
 * @param sec Sec reported by Gps  
 * @param visSat Visible sattelites reported by Gps  
 */
static inline void mavlink_msg_gps_date_time_send(mavlink_channel_t chan, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t visSat)
{
	mavlink_header_t hdr;
	mavlink_gps_date_time_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GPS_DATE_TIME_LEN )
	payload.year = year;	// uint8_t:Year reported by Gps 
	payload.month = month;	// uint8_t:Month reported by Gps 
	payload.day = day;	// uint8_t:Day reported by Gps 
	payload.hour = hour;	// uint8_t:Hour reported by Gps 
	payload.min = min;	// uint8_t:Min reported by Gps 
	payload.sec = sec;	// uint8_t:Sec reported by Gps  
	payload.visSat = visSat;	// uint8_t:Visible sattelites reported by Gps  

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GPS_DATE_TIME_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GPS_DATE_TIME;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xE, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GPS_DATE_TIME UNPACKING

/**
 * @brief Get field year from gps_date_time message
 *
 * @return Year reported by Gps 
 */
static inline uint8_t mavlink_msg_gps_date_time_get_year(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->year);
}

/**
 * @brief Get field month from gps_date_time message
 *
 * @return Month reported by Gps 
 */
static inline uint8_t mavlink_msg_gps_date_time_get_month(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->month);
}

/**
 * @brief Get field day from gps_date_time message
 *
 * @return Day reported by Gps 
 */
static inline uint8_t mavlink_msg_gps_date_time_get_day(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->day);
}

/**
 * @brief Get field hour from gps_date_time message
 *
 * @return Hour reported by Gps 
 */
static inline uint8_t mavlink_msg_gps_date_time_get_hour(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->hour);
}

/**
 * @brief Get field min from gps_date_time message
 *
 * @return Min reported by Gps 
 */
static inline uint8_t mavlink_msg_gps_date_time_get_min(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->min);
}

/**
 * @brief Get field sec from gps_date_time message
 *
 * @return Sec reported by Gps  
 */
static inline uint8_t mavlink_msg_gps_date_time_get_sec(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->sec);
}

/**
 * @brief Get field visSat from gps_date_time message
 *
 * @return Visible sattelites reported by Gps  
 */
static inline uint8_t mavlink_msg_gps_date_time_get_visSat(const mavlink_message_t* msg)
{
	mavlink_gps_date_time_t *p = (mavlink_gps_date_time_t *)&msg->payload[0];
	return (uint8_t)(p->visSat);
}

/**
 * @brief Decode a gps_date_time message into a struct
 *
 * @param msg The message to decode
 * @param gps_date_time C-struct to decode the message contents into
 */
static inline void mavlink_msg_gps_date_time_decode(const mavlink_message_t* msg, mavlink_gps_date_time_t* gps_date_time)
{
	memcpy( gps_date_time, msg->payload, sizeof(mavlink_gps_date_time_t));
}
