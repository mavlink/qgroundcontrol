/** @file
 *	@brief Main MAVLink comm protocol data.
 *	@see http://qgroundcontrol.org/mavlink/
 *	Edited on Monday, August 8 2011
 */

#ifndef  _ML_DATA_H_
#define  _ML_DATA_H_

#include "mavlink_types.h"

#ifdef MAVLINK_CHECK_LENGTH
const uint8_t MAVLINK_CONST mavlink_msg_lengths[256] = MAVLINK_MESSAGE_LENGTHS;
#endif

const uint8_t MAVLINK_CONST mavlink_msg_keys[256] = MAVLINK_MESSAGE_KEYS;

mavlink_status_t m_mavlink_status[MAVLINK_COMM_NB];
mavlink_message_t m_mavlink_message[MAVLINK_COMM_NB];
mavlink_system_t mavlink_system;
#endif