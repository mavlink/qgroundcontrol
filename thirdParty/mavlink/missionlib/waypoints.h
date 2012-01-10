/*******************************************************************************
 
 Copyright (C) 2011 Lorenz Meier lm ( a t ) inf.ethz.ch
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 ****************************************************************************/

/* This assumes you have the mavlink headers on your include path
 or in the same folder as this source file */

// Disable auto-data structures
#ifndef MAVLINK_NO_DATA
#define MAVLINK_NO_DATA
#endif

#include <mavlink_types.h>
extern void mavlink_send_uart_bytes(mavlink_channel_t chan, uint8_t* buffer, uint16_t len);

#ifndef MAVLINK_SEND_UART_BYTES
#define MAVLINK_SEND_UART_BYTES(chan, buffer, len) mavlink_send_uart_bytes(chan, buffer, len)
#endif
extern mavlink_system_t mavlink_system;
#include <mavlink.h>
#include <stdbool.h>

// FIXME XXX - TO BE MOVED TO XML
enum MAVLINK_WPM_STATES
{
    MAVLINK_WPM_STATE_IDLE = 0,
    MAVLINK_WPM_STATE_SENDLIST,
    MAVLINK_WPM_STATE_SENDLIST_SENDWPS,
    MAVLINK_WPM_STATE_GETLIST,
    MAVLINK_WPM_STATE_GETLIST_GETWPS,
    MAVLINK_WPM_STATE_GETLIST_GOTALL,
	MAVLINK_WPM_STATE_ENUM_END
};

enum MAVLINK_WPM_CODES
{
    MAVLINK_WPM_CODE_OK = 0,
    MAVLINK_WPM_CODE_ERR_WAYPOINT_ACTION_NOT_SUPPORTED,
    MAVLINK_WPM_CODE_ERR_WAYPOINT_FRAME_NOT_SUPPORTED,
    MAVLINK_WPM_CODE_ERR_WAYPOINT_OUT_OF_BOUNDS,
    MAVLINK_WPM_CODE_ERR_WAYPOINT_MAX_NUMBER_EXCEEDED,
    MAVLINK_WPM_CODE_ENUM_END
};


/* WAYPOINT MANAGER - MISSION LIB */

#define MAVLINK_WPM_MAX_WP_COUNT 15
#define MAVLINK_WPM_CONFIG_IN_FLIGHT_UPDATE				  ///< Enable double buffer and in-flight updates
#ifndef MAVLINK_WPM_TEXT_FEEDBACK
#define MAVLINK_WPM_TEXT_FEEDBACK 0						  ///< Report back status information as text
#endif
#define MAVLINK_WPM_PROTOCOL_TIMEOUT_DEFAULT 5000         ///< Protocol communication timeout in milliseconds
#define MAVLINK_WPM_SETPOINT_DELAY_DEFAULT 1000           ///< When to send a new setpoint
#define MAVLINK_WPM_PROTOCOL_DELAY_DEFAULT 40


struct mavlink_wpm_storage {
	mavlink_mission_item_t waypoints[MAVLINK_WPM_MAX_WP_COUNT];      ///< Currently active waypoints
#ifdef MAVLINK_WPM_CONFIG_IN_FLIGHT_UPDATE
	mavlink_mission_item_t rcv_waypoints[MAVLINK_WPM_MAX_WP_COUNT];  ///< Receive buffer for next waypoints
#endif
	uint16_t size;
	uint16_t max_size;
	uint16_t rcv_size;
	enum MAVLINK_WPM_STATES current_state;
	uint16_t current_wp_id;							///< Waypoint in current transmission
	uint16_t current_active_wp_id;					///< Waypoint the system is currently heading towards
	uint16_t current_count;
	uint8_t current_partner_sysid;
	uint8_t current_partner_compid;
	uint64_t timestamp_lastaction;
	uint64_t timestamp_last_send_setpoint;
	uint64_t timestamp_firstinside_orbit;
	uint64_t timestamp_lastoutside_orbit;
	uint32_t timeout;
	uint32_t delay_setpoint;
	float accept_range_yaw;
	float accept_range_distance;
	bool yaw_reached;
	bool pos_reached;
	bool idle;
};

typedef struct mavlink_wpm_storage mavlink_wpm_storage;

void mavlink_wpm_init(mavlink_wpm_storage* state);
void mavlink_wpm_loop();
void mavlink_wpm_message_handler(const mavlink_message_t* msg);
