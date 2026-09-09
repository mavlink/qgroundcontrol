/****************************************************************************
 *
 *   Copyright (c) 2018-2024 PX4 Development Team. All rights reserved.
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
 * @file sbf.cpp
 *
 * Septentrio protocol as defined in PPSDK SBF Reference Guide 4.1.8
 *
 * @author Matej Frančeškin <Matej.Franceskin@gmail.com>
 * @author <a href="https://github.com/SeppeG">Seppe Geuens</a>
 *
*/

#include "sbf.h"
#include "gps_time.h"
#include "rtcm.h"

#include <string.h>
#include <ctime>
#include <cmath>

#define SBF_CONFIG_TIMEOUT        1000      // ms, timeout for waiting ACK
#define SBF_PACKET_TIMEOUT        2        // ms, if now data during this delay assume that full update received
#define DISABLE_MSG_INTERVAL    1000000  // us, try to disable message with this interval
#define DNU                        100000.0 // Do-Not-Use value for PVTGeodetic
#define MSG_SIZE                    100 // size of the message to be sent to the receiver.

/**** Trace macros, disable for production builds */
#define SBF_TRACE_PARSER(...)   {/*GPS_INFO(__VA_ARGS__);*/}    /* decoding progress in parse_char() */
#define SBF_TRACE_RXMSG(...)    {/*GPS_INFO(__VA_ARGS__);*/}    /* Rx msgs in payload_rx_done() */
#define SBF_INFO(...)           {GPS_INFO(__VA_ARGS__);}

/**** Warning macros, disable to save memory */
#define SBF_WARN(...)        {GPS_WARN(__VA_ARGS__);}
#define SBF_DEBUG(...)       {/*GPS_WARN(__VA_ARGS__);*/}

GPSDriverSBF::GPSDriverSBF(GPSCallbackPtr callback, void *callback_user, struct sensor_gps_s *gps_position,
			   satellite_info_s *satellite_info, float heading_offset, float pitch_offset)
	: GPSBaseStationSupport(callback, callback_user), _gps_position(gps_position), _satellite_info(satellite_info),
	  _heading_offset(heading_offset), _pitch_offset(pitch_offset)
{
	decodeInit();
}

GPSDriverSBF::~GPSDriverSBF()
{
	delete _rtcm_parsing;
}

int GPSDriverSBF::configure(unsigned &baudrate, const GPSConfig &config)
{
	char buf[GPS_READ_BUFFER_SIZE];
	char msg[MSG_SIZE];

	_configured = false;

	setBaudrate(SBF_TX_CFG_PRT_BAUDRATE);
	baudrate = SBF_TX_CFG_PRT_BAUDRATE;
	_output_mode = config.output_mode;

	// Make sure we can send commands to the receiver
	sendMessage(SBF_CONFIG_FORCE_INPUT);

	// Disable previous output for now so we can detect the COM port
	for (int i = 1; i <= 2; i++) {
		snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "COM", i);
		sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);
	}

	for (int i = 1; i <= 4; i++) {
		snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "USB", i);
		sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);
	}

	char com_port[5] {};
	size_t offset = 1;
	bool response_detected = false;
	gps_abstime time_started = gps_absolute_time();
	sendMessage("\n\r");

	// Read buffer to get the COM port
	do {
		--offset; // overwrite the null-char
		int ret = read(reinterpret_cast<uint8_t *>(buf) + offset, sizeof(buf) - offset - 1, SBF_CONFIG_TIMEOUT);

		if (ret < 0) {
			// something went wrong when reading
			if (ret != ReadCancelled) {
				SBF_WARN("sbf read err");
			}
			return ret;
		}

		offset += ret;
		buf[offset++] = '\0';

		char *p = strstr(buf, ">");

		if (p) { //check if the length of the com port == 4 and contains a > sign
			for (int i = 0; i < 4; i++) {
				com_port[i] = buf[i];
			}

			response_detected = true;
		}

		if (offset >= sizeof(buf)) {
			offset = 1;
		}

	} while (time_started + 1000 * SBF_CONFIG_TIMEOUT > gps_absolute_time() && !response_detected);

	if (response_detected) {
		SBF_INFO("Septentrio GNSS receiver COM port: %s", com_port);
		response_detected = false; // for future use

	} else {
		SBF_WARN("No COM port detected")
		return -1;
	}

	// Delete all sbf outputs on current COM port to remove clutter data
	snprintf(msg, sizeof(msg), SBF_CONFIG_RESET, com_port);

	if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
		return -1; // connection and/or baudrate detection failed
	}

	// Set baudrate, unless we're connected over USB
	if (strncmp(com_port, "USB1", 4) != 0 && strncmp(com_port, "USB2", 4) != 0) {
		snprintf(msg, sizeof(msg), SBF_CONFIG_BAUDRATE, com_port, baudrate);

		if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
			SBF_DEBUG("Connection and/or baudrate detection failed (SBF_CONFIG_BAUDRATE)");
			return -1; // connection and/or baudrate detection failed
		}
	}

	// At this point we have correct baudrate on both ends
	SBF_DEBUG("Correct baud rate on both ends");

	// Define/inquire the type of data that the receiver should accept/send on a given connection descriptor
	snprintf(msg, sizeof(msg), SBF_DATA_IO, com_port);

	if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
		return -1;
	}

	// Set the type of dynamics the GNSS antenna is subjected to.
	if (_output_mode != OutputMode::RTCM) {

		// Release a previously configured static base position before starting navigation.
		if (!sendMessageAndWaitForAck("setPVTMode, Rover, All, auto\n", SBF_CONFIG_TIMEOUT)) {
			return -1;
		}

		// Specify the offsets that the receiver applies to the computed attitude angles.
		snprintf(msg, sizeof(msg), SBF_CONFIG_ATTITUDE_OFFSET, (double)(_heading_offset * 180 / M_PI_F), (double)_pitch_offset);

		if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
			return -1;
		}

		if (_dynamic_model < 6) {
			snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "low");

		} else if (_dynamic_model < 7) {
			snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "moderate");

		} else if (_dynamic_model < 8) {
			snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "high");

		} else {
			snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "max");
		}

		sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);

		snprintf(msg, sizeof(msg), SBF_CONFIG, com_port);

		sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);
	}

	int i = 0;

	do {
		++i;

		if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
			if (i >= 5) {
				return -1; // connection and/or baudrate detection failed
			}

		} else {
			response_detected = true;
		}
	} while (i < 5 && !response_detected);

	if (_output_mode == OutputMode::GPSAndRTCM || _output_mode == OutputMode::RTCM) {
		if (!_rtcm_parsing) {
			_rtcm_parsing = new RTCMParsing();
		}

		_rtcm_parsing->reset();
	}

	if (_output_mode != OutputMode::GPS) {
		sendMessageAndWaitForAck(SBF_CONFIG_OUTPUT_RTCM3, SBF_CONFIG_TIMEOUT);
	}

	if (_output_mode == OutputMode::RTCM) {
		switch (_base_settings.type) {
		case (BaseSettingsType::fixed_position):
			snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_COORDINATES,
				 _base_settings.settings.fixed_position.latitude,
				 _base_settings.settings.fixed_position.longitude,
				 static_cast<double>(_base_settings.settings.fixed_position.altitude));
			sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);

			snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_OFFSET, 0.0, 0.0, 0.0);
			sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);

			sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC1, SBF_CONFIG_TIMEOUT);
			sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC2, SBF_CONFIG_TIMEOUT);
			break;

		case (BaseSettingsType::survey_in):
		default:
			sendMessageAndWaitForAck(SBF_CONFIG_RTCM_SURVEY_IN, SBF_CONFIG_TIMEOUT);
			break;
		}

		sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATUS, SBF_CONFIG_TIMEOUT);
		_survey_active = true;
		_survey_activation_date = gps_absolute_time();
	}

	_configured = true;
	return 0;
}

bool GPSDriverSBF::sendMessage(const char *msg)
{
	// Send message
	SBF_DEBUG("Send MSG: %s", msg);
	int length = static_cast<int>(strlen(msg));

	return (write(msg, length) == length);
}

bool GPSDriverSBF::sendMessageAndWaitForAck(const char *msg, const int timeout)
{
	SBF_DEBUG("Send MSG: %s", msg);

	// Send message
	int length = static_cast<int>(strlen(msg));

	if (write(msg, length) != length) {
		return false;
	}

	// Wait for acknowledge
	// For all valid set -, get - and exe -commands, the first line of the reply is an exact copy
	// of the command as entered by the user, preceded with "$R:"
	char buf[GPS_READ_BUFFER_SIZE];
	size_t offset = 1;
	gps_abstime time_started = gps_absolute_time();

	bool found_response = false;

	do {
		--offset; //overwrite the null-char
		int ret = read(reinterpret_cast<uint8_t *>(buf) + offset, sizeof(buf) - offset - 1, timeout);

		if (ret < 0) {
			// something went wrong when reading
			if (ret != ReadCancelled) {
				SBF_WARN("sbf read err");
			}
			return false;
		}

		offset += ret;
		buf[offset++] = '\0';

		if (!found_response && strstr(buf, "$R: ") != nullptr) {
			//SBF_DEBUG("READ %d: %s", (int) offset, buf);
			found_response = true;
		}

		if (offset >= sizeof(buf)) {
			offset = 1;
		}

	} while (time_started + 1000 * timeout > gps_absolute_time());

	SBF_DEBUG("response: %u", found_response)
	return found_response;
}

// return value:
// 0b1111_1111 = an error occurred
// 0b0000_0000 = no message handled (not set up yet)
// 0b0000_0001 = message handled
// 0b0000_0010 = sat info message handled
// 0b0000_0100 = base station update (RTCM message or base station position)
int GPSDriverSBF::receive(unsigned timeout)
{
	int handled = 0;
	gps_abstime time_started;
	uint8_t buf[GPS_READ_BUFFER_SIZE];

	// Do not receive messages until we're configured
	if (!_configured) {
		gps_usleep(timeout * 1000);
		return 0;
	}

	// Timeout after not receiving a complete message for a certain time
	time_started = gps_absolute_time();

	while (true) {
		// Wait for only SBF_PACKET_TIMEOUT if something already received.
		int ret = read(buf, sizeof(buf), handled ? SBF_PACKET_TIMEOUT : timeout);

		if (ret < 0) {
			// something went wrong when reading
			if (ret != ReadCancelled) {
				SBF_WARN("sbf read err");
			}
			return -1;

		} else {
			SBF_DEBUG("Read %d bytes (receive)", ret);

			// pass received bytes to the packet decoder
			for (int i = 0; i < ret; i++) {
				handled |= parseChar(buf[i]);
				SBF_DEBUG("parsed %d: 0x%x", i, buf[i]);
			}
		}

		if (handled > 0) {
			SBF_DEBUG("Handled : %i", handled);
			return handled;
		}

		// abort after timeout if no useful packets received
		if (time_started + timeout * 1000 < gps_absolute_time()) {
			SBF_DEBUG("timed out after %d ms, returning", timeout);
			return -1;
		}
	}
}

// return value:
// 0b0000_0000 = still decoding
// 0b0000_0001 = message handled
// 0b0000_0010 = sat info message handled
// 0b0000_0100 = base station update
int GPSDriverSBF::parseChar(const uint8_t b)
{
	int ret = 0;

	if (_rtcm_parsing) {
		if (_rtcm_parsing->addByte(b)) {
			SBF_DEBUG("got RTCM message with length %i", (int) _rtcm_parsing->messageLength());
			gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
			decodeInit();
			_rtcm_parsing->reset();
			return 0b0100; // ret
		}
	}

	switch (_decode_state) {

	// Expecting Sync1
	case SBF_DECODE_SYNC1:
		if (b == SBF_SYNC1) { // Sync1 found --> expecting Sync2
			SBF_TRACE_PARSER("A");
			payloadRxAdd(b); // add a payload byte
			_decode_state = SBF_DECODE_SYNC2;
		}

		break;

	// Expecting Sync2
	case SBF_DECODE_SYNC2:
		if (b == SBF_SYNC2) { // Sync2 found --> expecting CRC
			SBF_TRACE_PARSER("B");
			payloadRxAdd(b); // add a payload byte
			_decode_state = SBF_DECODE_PAYLOAD;

		} else { // Sync1 not followed by Sync2: reset parser
			decodeInit();
		}

		break;

	// Expecting payload
	case SBF_DECODE_PAYLOAD: SBF_TRACE_PARSER(".");

		ret = payloadRxAdd(b); // add a payload byte

		if (ret < 0) {
			// payload not handled, discard message
			ret = 0;
			decodeInit();

		} else if (ret > 0) {
			ret = payloadRxDone(); // finish payload processing

			if (_rtcm_parsing) {
				_rtcm_parsing->reset();
			}

			decodeInit();

		} else {
			// expecting more payload, stay in state SBF_DECODE_PAYLOAD
			ret = 0;
		}

		break;

	default:
		break;
	}

	return ret;
}

/**
 * Add payload rx byte
 */
// -1 = error, 0 = ok, 1 = payload completed
int GPSDriverSBF::payloadRxAdd(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = reinterpret_cast<uint8_t *>(&_buf);

	p_buf[_rx_payload_index++] = b;

	if ((_rx_payload_index > 7 && _rx_payload_index >= _buf.length) || _rx_payload_index >= sizeof(_buf)) {
		ret = 1; // payload received completely
	}

	return ret;
}

/**
 * Calculate buffer CRC16
 */
uint16_t crc16(const uint8_t *data_p, uint32_t length)
{
	uint8_t x;
	uint16_t crc = 0;

	while (length--) {
		x = crc >> 8 ^ *data_p++;
		x ^= x >> 4;
		crc = static_cast<uint16_t>((crc << 8) ^ (x << 12) ^ (x << 5) ^ x);
	}

	return crc;
}

/**
 * Finish payload rx
 */
// 0 = no message handled, 1 = message handled, 2 = sat info message handled
int GPSDriverSBF::payloadRxDone()
{
	int ret = 0;
#ifndef NO_MKTIME
	struct tm timeinfo;
	time_t epoch;
#endif

	if (_buf.length <= 4 ||
	    _buf.length > _rx_payload_index ||
	    _buf.crc16 != crc16(reinterpret_cast<uint8_t *>(&_buf) + 4, _buf.length - 4)) {
		SBF_TRACE_RXMSG("Rx Unknow");
		return 0;
	}

	// handle message
	switch (_buf.msg_id) {
	case SBF_ID_PVTGeodetic: SBF_TRACE_RXMSG("Rx PVTGeodetic");
		_msg_status |= 1;

		if (_buf.payload_pvt_geodetic.mode_type < 1) {
			_gps_position->fix_type = 1;

		} else {

			switch (_buf.payload_pvt_geodetic.mode_type) {
			case 6:
				_gps_position->fix_type = 4;
				break;

			case 5:
			case 8:
				_gps_position->fix_type = 5;
				break;

			case 4:
			case 7:
				_gps_position->fix_type = 6;
				break;

			default:
				_gps_position->fix_type = 3;
				break;
			}
		}

		// Check fix and error code
		_gps_position->vel_ned_valid = _gps_position->fix_type > 1 && _buf.payload_pvt_geodetic.error == 0;

		// Check boundaries and invalidate GPS velocities
		// We're not just checking for the do-not-use value (-2*10^10) but for any value beyond the specified max values
		if (fabsf(_buf.payload_pvt_geodetic.vn) > 600.0f || fabsf(_buf.payload_pvt_geodetic.ve) > 600.0f ||
		    fabsf(_buf.payload_pvt_geodetic.vu) > 600.0f) {
			_gps_position->vel_ned_valid = false;
		}

		// Check boundaries and invalidate position
		// We're not just checking for the do-not-use value (-2*10^10) but for any value beyond the specified max values
		if (fabs(_buf.payload_pvt_geodetic.latitude) > (double)(M_PI_F / 2.0f) ||
		    fabs(_buf.payload_pvt_geodetic.longitude) > (double) M_PI_F ||
		    fabs(_buf.payload_pvt_geodetic.height) > DNU ||
		    fabsf(_buf.payload_pvt_geodetic.undulation) > (float) DNU) {
			_gps_position->fix_type = 0;
		}

		if (_buf.payload_pvt_geodetic.nr_sv < 255) {  // 255 = do not use value
			_gps_position->satellites_used = _buf.payload_pvt_geodetic.nr_sv;

			if (_satellite_info) {
				// Only fill in the satellite count for now (we could use the ChannelStatus message for the
				// other data, but it's really large: >800B)
				_satellite_info->timestamp = gps_absolute_time();
				_satellite_info->count = _gps_position->satellites_used;
				ret |= 2;
			}

		} else {
			_gps_position->satellites_used = 0;
		}

		_gps_position->latitude_deg = _buf.payload_pvt_geodetic.latitude * M_RAD_TO_DEG;
		_gps_position->longitude_deg = _buf.payload_pvt_geodetic.longitude * M_RAD_TO_DEG;
		_gps_position->altitude_ellipsoid_m = _buf.payload_pvt_geodetic.height;
		_gps_position->altitude_msl_m = _buf.payload_pvt_geodetic.height - static_cast<double>
						(_buf.payload_pvt_geodetic.undulation);

		/* H and V accuracy are reported in 2DRMS, but based off the uBlox reporting we expect RMS.
		 * Devide by 100 from cm to m and in addition divide by 2 to get RMS. */
		_gps_position->eph = static_cast<float>(_buf.payload_pvt_geodetic.h_accuracy) / 200.0f;
		_gps_position->epv = static_cast<float>(_buf.payload_pvt_geodetic.v_accuracy) / 200.0f;

		_gps_position->vel_n_m_s = static_cast<float>(_buf.payload_pvt_geodetic.vn);
		_gps_position->vel_e_m_s = static_cast<float>(_buf.payload_pvt_geodetic.ve);
		_gps_position->vel_d_m_s = -1.0f * static_cast<float>(_buf.payload_pvt_geodetic.vu);
		_gps_position->vel_m_s = sqrtf(_gps_position->vel_n_m_s * _gps_position->vel_n_m_s +
					       _gps_position->vel_e_m_s * _gps_position->vel_e_m_s);

		_gps_position->cog_rad = static_cast<float>(_buf.payload_pvt_geodetic.cog) * M_DEG_TO_RAD_F;
		_gps_position->c_variance_rad = 1.0f * M_DEG_TO_RAD_F;

		// _buf.payload_pvt_geodetic.cog is set to -2*10^10 for velocities below 0.1m/s
		if (_buf.payload_pvt_geodetic.cog > 360.0f) {
			_buf.payload_pvt_geodetic.cog = NAN;
		}

		_gps_position->time_utc_usec = 0;
#ifndef NO_MKTIME
		/* convert to unix timestamp */
		memset(&timeinfo, 0, sizeof(timeinfo));

		timeinfo.tm_year = 1980 - 1900;
		timeinfo.tm_mon = 0;
		timeinfo.tm_mday = 6 + _buf.WNc * 7;
		timeinfo.tm_hour = 0;
		timeinfo.tm_min = 0;
		timeinfo.tm_sec = _buf.TOW / 1000;

		epoch = gpsTimeToEpoch(timeinfo);

		if (epoch > GPS_EPOCH_SECS) {
			// FMUv2+ boards have a hardware RTC, but GPS helps us to configure it
			// and control its drift. Since we rely on the HRT for our monotonic
			// clock, updating it from time to time is safe.

			timespec ts;
			memset(&ts, 0, sizeof(ts));
			ts.tv_sec = epoch;
			ts.tv_nsec = (_buf.TOW % 1000) * 1000 * 1000;
			setClock(ts);

			_gps_position->time_utc_usec = static_cast<uint64_t>(epoch) * 1000000ULL;
			_gps_position->time_utc_usec += (_buf.TOW % 1000) * 1000;
		}

#endif
		_gps_position->timestamp = gps_absolute_time();
		_last_timestamp_time = _gps_position->timestamp;
		_rate_count_vel++;
		_rate_count_lat_lon++;
		ret |= (_msg_status == 7) ? 1 : 0;


		// In RTCM mode, PVTGeodetic is used to get base station survey-in
		if (_output_mode == OutputMode::RTCM) {
			SurveyInStatus status{};
			status.latitude = _gps_position->latitude_deg;
			status.longitude = _gps_position->longitude_deg;
			status.altitude = _gps_position->altitude_ellipsoid_m;
			status.duration = _survey_active ? (float)(gps_absolute_time() - _survey_activation_date) / 1000000.0f : 0;
			status.mean_accuracy = (_buf.payload_pvt_geodetic.h_accuracy + _buf.payload_pvt_geodetic.v_accuracy) /
					       20; // Value in mm
			status.flags = (_buf.payload_pvt_geodetic.mode_type > 0 ? 1 : 0) | (_survey_active & 1) << 1;
			surveyInStatus(status);
			ret |= 4; // RTCM infos have been updated
		}

		//SBF_DEBUG("PVTGeodetic handled");
		break;

	case SBF_ID_VelCovGeodetic: SBF_TRACE_RXMSG("Rx VelCovGeodetic");
		_msg_status |= 2;
		_gps_position->s_variance_m_s = _buf.payload_vel_col_geodetic.cov_ve_ve;

		if (_gps_position->s_variance_m_s < _buf.payload_vel_col_geodetic.cov_vn_vn) {
			_gps_position->s_variance_m_s = _buf.payload_vel_col_geodetic.cov_vn_vn;
		}

		if (_gps_position->s_variance_m_s < _buf.payload_vel_col_geodetic.cov_vu_vu) {
			_gps_position->s_variance_m_s = _buf.payload_vel_col_geodetic.cov_vu_vu;
		}

		//SBF_DEBUG("VelCovGeodetic handled");
		break;

	case SBF_ID_DOP: SBF_TRACE_RXMSG("Rx DOP");
		_msg_status |= 4;
		_gps_position->hdop = _buf.payload_dop.hDOP * 0.01f;
		_gps_position->vdop = _buf.payload_dop.vDOP * 0.01f;
		//SBF_DEBUG("DOP handled");
		break;

	case SBF_ID_AttEuler: SBF_TRACE_RXMSG("Rx AttEuler");

		if (!_buf.payload_att_euler.error_not_requested) {

			int error_aux1 = _buf.payload_att_euler.error_aux1;
			int error_aux2 = _buf.payload_att_euler.error_aux2;

			// SBF_DEBUG("Mode: %u", _buf.payload_att_euler.mode)
			if (error_aux1 == 0 && error_aux2 == 0) {
				float heading = _buf.payload_att_euler.heading;
				heading *= M_PI_F / 180.0f; // deg to rad, now in range [0, 2pi]


				if (heading > M_PI_F) {
					heading -= 2.f * M_PI_F; // final range is [-pi, pi]
				}

				_gps_position->heading = heading;
				// SBF_DEBUG("Heading: %.3f rad", (double) _gps_position->heading)
				//SBF_DEBUG("AttEuler handled");

			} else if (error_aux1 != 0) {
				//SBF_DEBUG("Error code for Main-Aux1 baseline: Not enough measurements");
			} else if (error_aux2 != 0) {
				//SBF_DEBUG("Error code for Main-Aux2 baseline: Not enough measurements");
			}
		} else {
			//SBF_DEBUG("AttEuler: attitude not requested by user");
		}


		break;

	case SBF_ID_AttCovEuler: SBF_TRACE_RXMSG("Rx AttCovEuler");

		if (!_buf.payload_att_cov_euler.error_not_requested) {
			int error_aux1 = _buf.payload_att_cov_euler.error_aux1;
			int error_aux2 = _buf.payload_att_cov_euler.error_aux2;

			if (error_aux1 == 0 && error_aux2 == 0) {
				float heading_acc = _buf.payload_att_cov_euler.cov_headhead;
				heading_acc *= M_PI_F / 180.0f; // deg to rad, now in range [0, 2pi]
				_gps_position->heading_accuracy = heading_acc;
				// SBF_DEBUG("Heading-Accuracy: %.3f rad", (double) _gps_position->heading_accuracy)
				//SBF_DEBUG("AttCovEuler handled");

			} else if (error_aux1 != 0) {
				//SBF_DEBUG("Error code for Main-Aux1 baseline: %u: Not enough measurements", error_aux1);
			} else if (error_aux2 != 0) {
				//SBF_DEBUG("Error code for Main-Aux2 baseline: %u: Not enough measurements", error_aux2);
			}
		} else {
			//SBF_DEBUG("AttCovEuler: attitude not requested by user");
		}

		break;

	default:
		SBF_TRACE_RXMSG("Rx other.");
		break;
	}

	if (ret > 0) {
		_gps_position->timestamp_time_relative = static_cast<int32_t>(_last_timestamp_time - _gps_position->timestamp);
	}

	if (ret == 1) {
		_msg_status &= ~1;
	}

	return ret;
}

void GPSDriverSBF::decodeInit()
{
	_decode_state = SBF_DECODE_SYNC1;
	_rx_payload_index = 0;
}

int GPSDriverSBF::reset(GPSRestartType restart_type)
{
	bool res = false;

	switch (restart_type) {
	case GPSRestartType::Hot:
		res = sendMessageAndWaitForAck(SBF_CONFIG_RESET_HOT, SBF_CONFIG_TIMEOUT);
		break;

	case GPSRestartType::Warm:
		res = sendMessageAndWaitForAck(SBF_CONFIG_RESET_WARM, SBF_CONFIG_TIMEOUT);
		break;

	case GPSRestartType::Cold:
		res = sendMessageAndWaitForAck(SBF_CONFIG_RESET_COLD, SBF_CONFIG_TIMEOUT);
		break;

	default:
		break;
	}

	return (res) ? 0 : -2;
}
