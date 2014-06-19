// MESSAGE SYS_Stat PACKING

#define MAVLINK_MSG_ID_SYS_Stat 190

typedef struct __mavlink_sys_stat_t
{
 uint8_t gps; ///< 
                
            
 uint8_t act; ///< 
                
            
 uint8_t mod; ///< 
                
            
 uint8_t commRssi; ///< 
                
            
} mavlink_sys_stat_t;

#define MAVLINK_MSG_ID_SYS_Stat_LEN 4
#define MAVLINK_MSG_ID_190_LEN 4

#define MAVLINK_MSG_ID_SYS_Stat_CRC 157
#define MAVLINK_MSG_ID_190_CRC 157



#define MAVLINK_MESSAGE_INFO_SYS_Stat { \
	"SYS_Stat", \
	4, \
	{  { "gps", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_sys_stat_t, gps) }, \
         { "act", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_sys_stat_t, act) }, \
         { "mod", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_sys_stat_t, mod) }, \
         { "commRssi", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_sys_stat_t, commRssi) }, \
         } \
}


/**
 * @brief Pack a sys_stat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param gps 
                
            
 * @param act 
                
            
 * @param mod 
                
            
 * @param commRssi 
                
            
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_stat_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t gps, uint8_t act, uint8_t mod, uint8_t commRssi)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SYS_Stat_LEN];
	_mav_put_uint8_t(buf, 0, gps);
	_mav_put_uint8_t(buf, 1, act);
	_mav_put_uint8_t(buf, 2, mod);
	_mav_put_uint8_t(buf, 3, commRssi);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SYS_Stat_LEN);
#else
	mavlink_sys_stat_t packet;
	packet.gps = gps;
	packet.act = act;
	packet.mod = mod;
	packet.commRssi = commRssi;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_SYS_Stat;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYS_Stat_LEN, MAVLINK_MSG_ID_SYS_Stat_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
}

/**
 * @brief Pack a sys_stat message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param gps 
                
            
 * @param act 
                
            
 * @param mod 
                
            
 * @param commRssi 
                
            
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_stat_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t gps,uint8_t act,uint8_t mod,uint8_t commRssi)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SYS_Stat_LEN];
	_mav_put_uint8_t(buf, 0, gps);
	_mav_put_uint8_t(buf, 1, act);
	_mav_put_uint8_t(buf, 2, mod);
	_mav_put_uint8_t(buf, 3, commRssi);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SYS_Stat_LEN);
#else
	mavlink_sys_stat_t packet;
	packet.gps = gps;
	packet.act = act;
	packet.mod = mod;
	packet.commRssi = commRssi;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_SYS_Stat;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYS_Stat_LEN, MAVLINK_MSG_ID_SYS_Stat_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
}

/**
 * @brief Encode a sys_stat struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sys_stat C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_stat_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sys_stat_t* sys_stat)
{
	return mavlink_msg_sys_stat_pack(system_id, component_id, msg, sys_stat->gps, sys_stat->act, sys_stat->mod, sys_stat->commRssi);
}

/**
 * @brief Encode a sys_stat struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param sys_stat C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_stat_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_sys_stat_t* sys_stat)
{
	return mavlink_msg_sys_stat_pack_chan(system_id, component_id, chan, msg, sys_stat->gps, sys_stat->act, sys_stat->mod, sys_stat->commRssi);
}

/**
 * @brief Send a sys_stat message
 * @param chan MAVLink channel to send the message
 *
 * @param gps 
                
            
 * @param act 
                
            
 * @param mod 
                
            
 * @param commRssi 
                
            
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sys_stat_send(mavlink_channel_t chan, uint8_t gps, uint8_t act, uint8_t mod, uint8_t commRssi)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SYS_Stat_LEN];
	_mav_put_uint8_t(buf, 0, gps);
	_mav_put_uint8_t(buf, 1, act);
	_mav_put_uint8_t(buf, 2, mod);
	_mav_put_uint8_t(buf, 3, commRssi);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, buf, MAVLINK_MSG_ID_SYS_Stat_LEN, MAVLINK_MSG_ID_SYS_Stat_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, buf, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
#else
	mavlink_sys_stat_t packet;
	packet.gps = gps;
	packet.act = act;
	packet.mod = mod;
	packet.commRssi = commRssi;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, (const char *)&packet, MAVLINK_MSG_ID_SYS_Stat_LEN, MAVLINK_MSG_ID_SYS_Stat_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, (const char *)&packet, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_SYS_Stat_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_sys_stat_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t gps, uint8_t act, uint8_t mod, uint8_t commRssi)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint8_t(buf, 0, gps);
	_mav_put_uint8_t(buf, 1, act);
	_mav_put_uint8_t(buf, 2, mod);
	_mav_put_uint8_t(buf, 3, commRssi);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, buf, MAVLINK_MSG_ID_SYS_Stat_LEN, MAVLINK_MSG_ID_SYS_Stat_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, buf, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
#else
	mavlink_sys_stat_t *packet = (mavlink_sys_stat_t *)msgbuf;
	packet->gps = gps;
	packet->act = act;
	packet->mod = mod;
	packet->commRssi = commRssi;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, (const char *)packet, MAVLINK_MSG_ID_SYS_Stat_LEN, MAVLINK_MSG_ID_SYS_Stat_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_Stat, (const char *)packet, MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE SYS_Stat UNPACKING


/**
 * @brief Get field gps from sys_stat message
 *
 * @return 
                
            
 */
static inline uint8_t mavlink_msg_sys_stat_get_gps(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field act from sys_stat message
 *
 * @return 
                
            
 */
static inline uint8_t mavlink_msg_sys_stat_get_act(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field mod from sys_stat message
 *
 * @return 
                
            
 */
static inline uint8_t mavlink_msg_sys_stat_get_mod(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field commRssi from sys_stat message
 *
 * @return 
                
            
 */
static inline uint8_t mavlink_msg_sys_stat_get_commRssi(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Decode a sys_stat message into a struct
 *
 * @param msg The message to decode
 * @param sys_stat C-struct to decode the message contents into
 */
static inline void mavlink_msg_sys_stat_decode(const mavlink_message_t* msg, mavlink_sys_stat_t* sys_stat)
{
#if MAVLINK_NEED_BYTE_SWAP
	sys_stat->gps = mavlink_msg_sys_stat_get_gps(msg);
	sys_stat->act = mavlink_msg_sys_stat_get_act(msg);
	sys_stat->mod = mavlink_msg_sys_stat_get_mod(msg);
	sys_stat->commRssi = mavlink_msg_sys_stat_get_commRssi(msg);
#else
	memcpy(sys_stat, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_SYS_Stat_LEN);
#endif
}
