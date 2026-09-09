/****************************************************************************
 *
 *   Copyright (C) 2019. All rights reserved.
 *   Author: Rui Zheng <ruizheng@femtomes.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/** @file Femtomes protocol definitions */
#pragma once

#include "base_station.h"

class RTCMParsing;

/*** femtomes protocol binary message and payload definitions ***/
#pragma pack(push, 1)

/**
* femto_uav_gps_t struct need to be packed
*/
typedef struct {
	uint64_t 	time_utc_usec;		/** Timestamp (microseconds, UTC), this is the timestamp which comes from the gps module. It might be unavailable right after cold start, indicated by a value of 0*/
	int32_t 	lat;			/** Latitude in 1E-7 degrees*/
	int32_t 	lon;			/** Longitude in 1E-7 degrees*/
	int32_t 	alt;			/** Altitude in 1E-3 meters above MSL, (millimetres)*/
	int32_t 	alt_ellipsoid;		/** Altitude in 1E-3 meters bove Ellipsoid, (millimetres)*/
	float 		s_variance_m_s;		/** GPS speed accuracy estimate, (metres/sec)*/
	float 		c_variance_rad;		/** GPS course accuracy estimate, (radians)*/
	float 		eph;			/** GPS horizontal position accuracy (metres)*/
	float 		epv;			/** GPS vertical position accuracy (metres)*/
	float 		hdop;			/** Horizontal dilution of precision*/
	float 		vdop;			/** Vertical dilution of precision*/
	int32_t 	noise_per_ms;		/** GPS noise per millisecond*/
	int32_t 	jamming_indicator;	/** indicates jamming is occurring*/
	float 		vel_m_s;		/** GPS ground speed, (metres/sec)*/
	float 		vel_n_m_s;		/** GPS North velocity, (metres/sec)*/
	float 		vel_e_m_s;		/** GPS East velocity, (metres/sec)*/
	float 		vel_d_m_s;		/** GPS Down velocity, (metres/sec)*/
	float 		cog_rad;		/** Course over ground (NOT heading, but direction of movement), -PI..PI, (radians)*/
	int32_t 	timestamp_time_relative;/** timestamp + timestamp_time_relative = Time of the UTC timestamp since system start, (microseconds)*/
	float 		heading;		/** heading angle of XYZ body frame rel to NED. Set to NaN if not available and updated (used for dual antenna GPS), (rad, [-PI, PI])*/
	uint8_t 	fix_type;		/** 0-1: no fix, 2: 2D fix, 3: 3D fix, 4: RTCM code differential, 5: Real-Time Kinematic, float, 6: Real-Time Kinematic, fixed, 8: Extrapolated. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.*/
	bool 		vel_ned_valid;		/** True if NED velocity is valid*/
	uint8_t 	satellites_used;	/** Number of satellites used*/
	uint8_t		heading_type;		/**< 0 invalid,5 for float,6 for fix*/
} femto_uav_gps_t;

/**
* femto_msg_header_t is femto data header
*/
typedef struct {
	uint8_t 	preamble[3];	/**< Frame header preamble 0xaa 0x44 0x12 */
	uint8_t 	headerlength;	/**< Frame header length ,from the beginning 0xaa */
	uint16_t 	messageid;     /**< Frame message id ,example the FEMTO_MSG_ID_UAVGPS 8001*/
	uint8_t 	messagetype;	/**< Frame message id type */
	uint8_t 	portaddr;	/**< Frame message port address */
	uint16_t 	messagelength; /**< Frame message data length,from the beginning headerlength+1,end headerlength + messagelength*/
	uint16_t 	sequence;
	uint8_t 	idletime;	/**< Frame message idle module time */
	uint8_t 	timestatus;
	uint16_t 	week;
	uint32_t 	tow;
	uint32_t 	recvstatus;
	uint16_t 	resv;
	uint16_t 	recvswver;
} femto_msg_header_t;

/**
*  uav status data
*/
typedef struct {
	int32_t     master_ant_status;
	int32_t     slave_ant_status;
	uint16_t    master_ant_power;
	uint16_t    slave_ant_power;
	uint32_t    jam_status;
	uint32_t    spoofing_status;
	uint16_t    reserved16_1;
	uint16_t    diff_age;		    /**< in unit of second*/
	uint32_t    base_id;
	uint32_t    reserved32_1;
	uint32_t    reserved32_2;
	uint32_t    sat_number;
	struct femto_uav_sat_status_data_t {
		uint8_t   svid;
		uint8_t   system_id;
		uint8_t   cn0;		        /**< in unit of dB-Hz*/
		uint8_t   ele;		        /**< in unit of degree*/
		uint16_t  azi;		        /**< in unit of degree*/
		uint16_t  status;
	} sat_status[64];               /**< uav status of all satellites */
} femto_uav_status_t;

/**
* Analysis Femto uavgps frame header
*/
typedef union {
	femto_msg_header_t 	femto_header;
	uint8_t 	 	data[28];
} msg_header_t;

/**
* receive Femto complete uavgps frame
*/
typedef struct {
	uint8_t 		data[600];		/**< receive Frame message content */
	uint32_t 		crc;			/**< receive Frame message crc 4 bytes */
	msg_header_t 		header;			/**< receive Frame message header */
	uint16_t 		read;			/**< receive Frame message read bytes count */
} femto_msg_t;

#pragma pack(pop)
/*** END OF femtomes protocol binary message and payload definitions ***/

enum class FemtoDecodeState {
	pream_ble1,			/**< Frame header preamble first byte 0xaa */
	pream_ble2,			/**< Frame header preamble second byte 0x44 */
	pream_ble3,			/**< Frame header preamble third byte 0x12 */
	head_length,			/**< Frame header length */
	head_data,			/**< Frame header data */
	data,				/**< Frame data */
	crc1,				/**< Frame crc1 */
	crc2,				/**< Frame crc2 */
	crc3,				/**< Frame crc3 */
	crc4,				/**< Frame crc4 */

	pream_nmea_got_sync1,           /**< NMEA Frame '$' */
	pream_nmea_got_asteriks,        /**< NMEA Frame '*' */
	pream_nmea_got_first_cs_byte,   /**< NMEA Frame cs first byte */
};

class GPSDriverFemto : public GPSBaseStationSupport
{
public:
	/**
	 * @param heading_offset heading offset in radians [-pi, pi]. It is substracted from the measurement.
	 */
	GPSDriverFemto(GPSCallbackPtr callback, void *callback_user, struct sensor_gps_s *gps_position,
		       satellite_info_s *satellite_info = nullptr,
		       float heading_offset = 0.f);
	virtual ~GPSDriverFemto();

	int receive(unsigned timeout) override;
	int configure(unsigned &baudrate, const GPSConfig &config) override;

private:

	/**
	 * when Constructor is work, initialize parameters
	 */
	void decodeInit(void);

	/**
	 * check the message if whether is 8001,memcpy data to _gps_position
	 */
	int handleMessage(int len);

	/**
	 * analysis frame data from buf[] to _femto_msg and check the frame is suceess or not
	 */
	int parseChar(uint8_t b);

	/**
	 * Write a command and wait for a (N)Ack
	 * @return 0 on success, <0 otherwise
	 */
	int writeAckedCommandFemto(const char *command, const char *reply, const unsigned timeout);

	/**
	 * receive data for at least the specified amount of time
	 */
	void receiveWait(unsigned timeout_min);

	/**
	* enable output of correction output
	*/
	void activateCorrectionOutput();

	/**
	 * enable output of rtcm
	 */
	void activateRTCMOutput();

	/**
	 * update survery in status of QGC RTK GPS
	 */
	void sendSurveyInStatusUpdate(bool active, bool valid, double latitude = (double)NAN,
				      double longitude = (double)NAN, float altitude = NAN);


	struct sensor_gps_s 	*_gps_position {nullptr};
	FemtoDecodeState		_decode_state{FemtoDecodeState::pream_ble1};
	femto_uav_gps_t			_femto_uav_gps;
	femto_msg_t 			_femto_msg;
	satellite_info_s        *_satellite_info{nullptr};
	float 					_heading_offset;

	RTCMParsing             *_rtcm_parsing{nullptr};
	OutputMode              _output_mode{OutputMode::GPS};
	bool                    _configure_done{false};
	bool                    _correction_output_activated{false};

	gps_abstime 			_survey_in_start{0};
};
