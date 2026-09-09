/****************************************************************************
 *
 *   Copyright (c) 2012-2019 PX4 Development Team. All rights reserved.
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

/**
 * @file emlid_reach.h
 *
 * @author Bastien Auneau <bastien.auneau@while-true.fr>
 */

#pragma once

#include "gps_helper.h"

// Emlid documentation
//   https://docs.emlid.com/reachm-plus/
//   https://files.emlid.com/ERB.pdf

#define ERB_HEADER_LEN           5
// Not using ERB_ID_SPACE_INFO, so use a smaller buff length
#define ERB_SENTENCE_MAX_LEN     (sizeof(erb_message_t))

#define MAX_CONST(a, b) ((a>b) ? a : b)

// Emlid ERB message definition
#pragma pack(push, 1)

typedef struct {
	uint8_t		sync1;
	uint8_t		sync2;
	uint8_t		id;
	uint16_t	length;
} erb_header_t;

typedef struct {
	uint8_t			ck_a;
	uint8_t			ck_b;
} erb_checksum_t;

typedef struct {
	uint32_t		timeGPS;
	uint8_t			verH;
	uint8_t			verM;
	uint8_t			verL;
	erb_checksum_t	checksum;
} erb_version_t;

typedef struct {
	uint32_t		timeGPS;
	double			longitude;
	double			latitude;
	double			altElipsoid;
	double			altMeanSeaLevel;
	uint32_t		accHorizontal;
	uint32_t		accVertical;
	erb_checksum_t	checksum;
} erb_geodic_position_t;

typedef struct {
	uint32_t		timeGPS;
	uint16_t		weekGPS;
	uint8_t			fixType;
	uint8_t			fixStatus;
	uint8_t			numSatUsed;
	erb_checksum_t	checksum;
} erb_navigation_status_t;

typedef struct {
	uint32_t		timeGPS;
	uint16_t		dopGeometric;
	uint16_t		dopPosition;
	uint16_t		dopVertical;
	uint16_t		dopHorizontal;
	erb_checksum_t	checksum;
} erb_dop_t;

typedef struct {
	uint32_t		timeGPS;
	int32_t			velN;
	int32_t			velE;
	int32_t			velD;
	uint32_t		speed;
	int32_t			heading;
	uint32_t		speedAccuracy;
	erb_checksum_t	checksum;
} erb_ned_velocity_t;

typedef union {
	erb_version_t			version;
	erb_geodic_position_t	geodic_position;
	erb_navigation_status_t	navigation_status;
	erb_dop_t				dop;
	erb_ned_velocity_t		ned_velocity;
} erb_payload_t;

typedef struct {
	erb_header_t	header;
	erb_payload_t	payload;
} erb_message_t;

#pragma pack(pop)


/**
 * Driver class for Emlid Reach
 * Populates caller provided sensor_gps_s
 * Some ERB messages are cached and correlated by timestamp before publishing it
 */
class GPSDriverEmlidReach : public GPSHelper
{
public:
	GPSDriverEmlidReach(GPSCallbackPtr callback, void *callback_user,
			    sensor_gps_s *gps_position,
			    satellite_info_s *satellite_info
			   );

	virtual ~GPSDriverEmlidReach() = default;

	int receive(unsigned timeout) override;
	int configure(unsigned &baudrate, const GPSConfig &config) override;

private:

	enum class ERB_State {
		init = 0,
		got_sync_1,  // E
		got_sync_2,  // R
		got_id,
		got_len_1,
		got_len_2,
		got_payload,
		got_CK_A
	};


	/** NMEA parser state machine */
	ERB_State _erb_decode_state;

	/** Buffer used by parser to build ERB sentences */
	erb_message_t _erb_buff{};
	uint16_t _erb_buff_cnt{};

	/** Buffer used by parser to build ERB checksum */
	erb_checksum_t _erb_checksum{};
	uint8_t _erb_checksum_cnt{};

	/** Pointer provided by caller, ie gps.cpp */
	sensor_gps_s *_gps_position {nullptr};
	/** Pointer provided by caller, gps.cpp */
	satellite_info_s *_satellite_info {nullptr};

	bool _testing_connection{false};
	/** counts decoded sentence when testing connection */
	unsigned _sentence_cnt{0};

	uint16_t _erb_payload_len{0};

	uint32_t _last_POS_timeGPS{0};
	uint32_t _last_VEL_timeGPS{0};
	bool _POS_received{false};
	bool _VEL_received{false};


	///// ERB messages caches /////
	uint8_t _fix_type{0};
	uint8_t _fix_status{0};
	uint8_t _satellites_used{0};
	float _hdop{0};
	float _vdop{0};


	/** Feed ERB parser with received bytes from serial
	 * @return len of decoded message, 0 if not completed, -1 if error
	 */
	int erbParseChar(uint8_t b);

	/** ERB sentence into sensor_gps_s or satellite_info_s, to be used by GPSHelper
	 *  @return 1 if gps_position updated, 2 for satellite_info_s (can be bit OR), 0 for nothing
	 */
	int handleErbSentence();

	void computeNedVelocity();

	bool testConnection();

};

