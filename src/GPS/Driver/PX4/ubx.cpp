/****************************************************************************
 *
 *   Copyright (c) 2012-2023 PX4 Development Team. All rights reserved.
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
 * @file ubx.cpp
 *
 * U-Blox protocol implementation. Following u-blox 6/7/8/9 Receiver Description
 * including Prototol Specification.
 *
 * @author Thomas Gubler <thomasgubler@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 * @author Anton Babushkin <anton.babushkin@me.com>
 * @author Beat Kueng <beat-kueng@gmx.net>
 *
 * @author Hannes Delago
 *   (rework, add ubx7+ compatibility)
 *
 * @see https://www.u-blox.com/sites/default/files/products/documents/u-blox6-GPS-GLONASS-QZSS-V14_ReceiverDescrProtSpec_%28GPS.G6-SW-12013%29_Public.pdf
 * @see https://www.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_%28UBX-13003221%29_Public.pdf
 * @see https://www.u-blox.com/sites/default/files/ZED-F9P_InterfaceDescription_%28UBX-18010854%29.pdf
 */

#include <cmath>
#include <string.h>

#include "rtcm.h"
#include "ubx.h"

#define MIN(X,Y)              ((X) < (Y) ? (X) : (Y))
#define SWAP16(X)             ((((X) >>  8) & 0x00ff) | (((X) << 8) & 0xff00))

/**** Trace macros, disable for production builds */
#define UBX_TRACE_PARSER(...) {/*GPS_INFO(__VA_ARGS__);*/}    // decoding progress in parse_char()
#define UBX_TRACE_RXMSG(...)  {/*GPS_INFO(__VA_ARGS__);*/}    // Rx msgs in payload_rx_done()
#define UBX_TRACE_SVINFO(...) {/*GPS_INFO(__VA_ARGS__);*/}    // NAV-SVINFO processing (debug use only, will cause rx buffer overflows)

/**** Warning macros, disable to save memory */
#define UBX_WARN(...)         {GPS_WARN(__VA_ARGS__);}
#define UBX_DEBUG(...)        {/*GPS_WARN(__VA_ARGS__);*/}

// RTCM3 message sets for a base: the station/bias messages plus GPS, GLONASS, Galileo and BeiDou
// observations as MSM4 or MSM7 (1074/1084/1094/1124 vs 1077/1087/1097/1127)
static constexpr uint32_t RTCM_BASE_MSGOUT_I2C[] = {
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1005_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1077_I2C,
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1087_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1230_I2C,
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1097_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1127_I2C
};
static constexpr uint32_t RTCM_MSM4_UART1[] = {
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1074_UART1, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1084_UART1,
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1094_UART1, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1124_UART1
};
static constexpr uint32_t RTCM_MSM7_UART1[] = {
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1077_UART1, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1087_UART1,
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1097_UART1, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1127_UART1
};
static constexpr uint32_t RTCM_MSM4_UART2[] = {
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1074_UART2, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1084_UART2,
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1094_UART2, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1124_UART2
};
static constexpr uint32_t RTCM_MSM7_UART2[] = {
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1077_UART2, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1087_UART2,
	UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1097_UART2, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1127_UART2
};

GPSDriverUBX::GPSDriverUBX(Interface gpsInterface, GPSCallbackPtr callback, void *callback_user,
			   sensor_gps_s *gps_position, satellite_info_s *satellite_info, Settings settings) :
	GPSBaseStationSupport(callback, callback_user),
	_interface(gpsInterface),
	_gps_position(gps_position),
	_satellite_info(satellite_info),
	_dyn_model(settings.dynamic_model),
	_dgnss_timeout(settings.dgnss_timeout),
	_min_cno(settings.min_cno),
	_min_elev(settings.min_elev),
	_output_rate(settings.output_rate),
	_mode(settings.mode),
	_heading_offset(settings.heading_offset),
	_uart1_baudrate(settings.uart1_baudrate),
	_uart2_baudrate(settings.uart2_baudrate),
	_ppk_output(settings.ppk_output),
	_jam_det_sensitivity_hi(settings.jam_det_sensitivity_hi)
{
	decodeInit();
}

GPSDriverUBX::~GPSDriverUBX()
{
	delete _rtcm_parsing;
}

int GPSDriverUBX::enableNmeaOutput(unsigned baudrate)
{
	if (!_configured || _output_mode != OutputMode::GPS || _mode != UBXMode::Normal
	    || _interface != Interface::UART || baudrate == 0) {
		return -1;
	}

	// The decoder accepts configuration acknowledgements only while configuring.
	_configured = false;
	const int result = [&]() -> int {
		if (_proto_ver_27_or_higher) {
			// CFG-MSGOUT NMEA RMC, GGA, GSA and GSV keys for the I2C port; cfgValsetPort
			// selects UART1 and USB without changing unrelated receiver ports.
			static constexpr uint32_t nmea_messages[] = {
				UBX_CFG_KEY_MSGOUT_NMEA_RMC_I2C, UBX_CFG_KEY_MSGOUT_NMEA_GGA_I2C,
				UBX_CFG_KEY_MSGOUT_NMEA_GSA_I2C, UBX_CFG_KEY_MSGOUT_NMEA_GSV_I2C
			};
			initCfgValset();
			cfgValsetPort(nmea_messages, 1);
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_NMEA, 1);
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA, 1);
			return sendCfgValsetAcked();
		}

		// Legacy receivers use CFG-MSG and CFG-PRT rather than configuration keys.
		static constexpr uint16_t legacy_messages[] = {0x04f0, 0x00f0, 0x02f0, 0x03f0};
		for (const uint16_t message : legacy_messages) {
			if (!configureMessageRateAndAck(message, 1, true)) {
				return -1;
			}
		}
		ubx_payload_tx_cfg_prt_t ports[2]{};
		for (unsigned i = 0; i < 2; ++i) {
			ports[i].portID = i == 0 ? UBX_TX_CFG_PRT_PORTID : UBX_TX_CFG_PRT_PORTID_USB;
			ports[i].mode = UBX_TX_CFG_PRT_MODE;
			ports[i].baudRate = baudrate;
			ports[i].inProtoMask = UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM;
			ports[i].outProtoMask = UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_NMEA;
		}
		if (!sendMessage(UBX_MSG_CFG_PRT, reinterpret_cast<uint8_t *>(ports), sizeof(ports))) {
			return -1;
		}
		return waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, true);
	}();
	_configured = result == 0;
	return result;
}

int
GPSDriverUBX::configure(unsigned &baudrate, const GPSConfig &config)
{
	return configure(baudrate, config, OutputProtocol::Native);
}

GPSDriverUBX::BaseStationCapability GPSDriverUBX::baseStationCapability() const
{
	switch (_board) {
	case Board::u_blox8:
		return _is_m8p ? BaseStationCapability::Supported :
		       (_model_name[0] ? BaseStationCapability::Unsupported : BaseStationCapability::Unknown);
	case Board::u_blox9_F9P_L1L2:
	case Board::u_blox9_F9P_L1L5:
	case Board::u_blox_X20:
		return BaseStationCapability::Supported;
	case Board::u_blox9:
	case Board::u_blox10:
	case Board::u_blox10_L1L5:
		return BaseStationCapability::Unsupported;
	default:
		return BaseStationCapability::Unknown;
	}
}

int
GPSDriverUBX::configure(unsigned &baudrate, const GPSConfig &config, OutputProtocol output_protocol)
{
	_configured = false;
	if (output_protocol != OutputProtocol::Native
	    && (output_protocol != OutputProtocol::NMEA || config.output_mode != OutputMode::GPS
	        || _mode != UBXMode::Normal || _interface != Interface::UART)) {
		return -1;
	}
	_output_mode = config.output_mode;

	ubx_payload_tx_cfg_prt_t cfg_prt[2];

	uint16_t out_proto_mask = _output_mode == OutputMode::GPS ?
				  UBX_TX_CFG_PRT_PROTO_UBX :
				  (UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM);

	uint16_t in_proto_mask = (_output_mode == OutputMode::GPS || _output_mode == OutputMode::GPSAndRTCM) ?
				 (UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM) :
				 UBX_TX_CFG_PRT_PROTO_UBX;

	const bool auto_baudrate = baudrate == 0;

	if (_interface == Interface::UART) {

		/* try different baudrates */
		const unsigned baudrates[] = {38400, 57600, 9600, 115200, 230400, 460800, 921600};

		unsigned baud_i;
		unsigned desired_baudrate = auto_baudrate ? UBX_BAUDRATE_M8_AND_NEWER : baudrate;

		// uart1_baudrate (GPS_UBX_BAUD1) is the sole UART1 target after auto-detect.
		// 0 keeps the driver default (115200). The probe scan is unaffected, so this
		// cannot lock the driver out of a receiver at its power-on default the way a
		// fixed baudrate does.
		if (_uart1_baudrate > 0) {
			desired_baudrate = _uart1_baudrate;
		}

		for (baud_i = 0; baud_i < sizeof(baudrates) / sizeof(baudrates[0]); baud_i++) {
			unsigned test_baudrate = baudrates[baud_i];

			if (!auto_baudrate && baudrate != test_baudrate) {
				continue; // skip to next baudrate
			}

			UBX_DEBUG("baudrate set to %i", test_baudrate);

			setBaudrate(test_baudrate);

			/* flush input and wait for at least 20 ms silence */
			decodeInit();
			receive(20);
			decodeInit();

			if (config.cfg_wipe) {
				/* Send a CFG-CFG message to wipe the FLASH and reload a clean config */
				memset(&_buf.payload_tx_cfg_cfg, 0, sizeof(_buf.payload_tx_cfg_cfg));
				_buf.payload_tx_cfg_cfg.clearMask = 0xFFFFFFFF;
				_buf.payload_tx_cfg_cfg.loadMask = 0xFFFFFFFF;

				if (!sendMessage(UBX_MSG_CFG_CFG, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_cfg))) {
					continue;
				}

				if (waitForAck(UBX_MSG_CFG_CFG, 2000, true) < 0) {
					continue;
				}
			}

			// try CFG-VALSET: if we get an ACK we know we can use protocol version 27+
			static constexpr CfgValsetItem uart1_ubx[] = {
				{UBX_CFG_KEY_CFG_UART1_STOPBITS, 1},
				{UBX_CFG_KEY_CFG_UART1_DATABITS, 0},
				{UBX_CFG_KEY_CFG_UART1_PARITY, 0},
				{UBX_CFG_KEY_CFG_UART1INPROT_UBX, 1},
				{UBX_CFG_KEY_CFG_UART1INPROT_NMEA, 0},
				{UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1},
				{UBX_CFG_KEY_CFG_UART1OUTPROT_NMEA, 0},
			};
			initCfgValset();
			cfgValset(uart1_ubx);
			// TODO: are we ever connected to UART2?

			// Note: USB protocol settings are handled later in the configureDevice function.

			bool cfg_valset_success = false;

			if (sendCfgValset()) {

				// Note: The M10 comes up sending NMEA sentences at 9600. It can't
				// respond with an ACK until the current sentence has completed transmission.
				// This can take over a second so need a large timeout on this particular wait.
				// Once it has acked this it will turn off the NMEA sentences and all is good
				// for future transactions.
				if (waitForAck(UBX_MSG_CFG_VALSET, 2000, true) == 0) {
					cfg_valset_success = true;
				}
			}

			if (cfg_valset_success) {
				_proto_ver_27_or_higher = true;
				// Now we only have to change the baudrate
				initCfgValset();
				cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART1_BAUDRATE, desired_baudrate);

				if (!sendCfgValset()) {
					continue;
				}

				/* no ACK is expected here, but read the buffer anyway in case we actually get an ACK */
				waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false);

			} else {
				_proto_ver_27_or_higher = false;

				UBX_DEBUG("trying old protocol");

				/* Send a CFG-PRT message to set the UBX protocol for in and out
				 * and leave the baudrate as it is, we just want an ACK-ACK for this */
				memset(cfg_prt, 0, 2 * sizeof(ubx_payload_tx_cfg_prt_t));
				cfg_prt[0].portID		= UBX_TX_CFG_PRT_PORTID;
				cfg_prt[0].mode		= UBX_TX_CFG_PRT_MODE;
				cfg_prt[0].baudRate	= test_baudrate;
				cfg_prt[0].inProtoMask	= in_proto_mask;
				cfg_prt[0].outProtoMask	= out_proto_mask;
				cfg_prt[1].portID		= UBX_TX_CFG_PRT_PORTID_USB;
				cfg_prt[1].mode		= UBX_TX_CFG_PRT_MODE;
				cfg_prt[1].baudRate	= test_baudrate;
				cfg_prt[1].inProtoMask	= in_proto_mask;
				cfg_prt[1].outProtoMask	= out_proto_mask;

				if (!sendMessage(UBX_MSG_CFG_PRT, (uint8_t *)cfg_prt, 2 * sizeof(ubx_payload_tx_cfg_prt_t))) {
					continue;
				}

				if (waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false) < 0) {
					/* try next baudrate */
					continue;
				}

				if (auto_baudrate && _uart1_baudrate == 0) {
					desired_baudrate = UBX_TX_CFG_PRT_BAUDRATE;
				}

				/* Send a CFG-PRT message again, this time change the baudrate */
				cfg_prt[0].baudRate	= desired_baudrate;
				cfg_prt[1].baudRate	= desired_baudrate;

				if (!sendMessage(UBX_MSG_CFG_PRT, (uint8_t *)cfg_prt, 2 * sizeof(ubx_payload_tx_cfg_prt_t))) {
					continue;
				}

				/* no ACK is expected here, but read the buffer anyway in case we actually get an ACK */
				waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false);
			}

			if (desired_baudrate != test_baudrate) {
				setBaudrate(desired_baudrate);

				decodeInit();
				receive(20);
				decodeInit();
			}

			/* at this point we have correct baudrate on both ends */
			baudrate = desired_baudrate;
			break;
		}

		if (baud_i >= sizeof(baudrates) / sizeof(baudrates[0])) {
			return -1;	// connection and/or baudrate detection failed
		}

	} else if (_interface == Interface::SPI) {

		if (config.cfg_wipe) {
			/* Send a CFG-CFG message to wipe the FLASH and reload a clean config */
			memset(&_buf.payload_tx_cfg_cfg, 0, sizeof(_buf.payload_tx_cfg_cfg));
			_buf.payload_tx_cfg_cfg.clearMask = 0xFFFFFFFF;
			_buf.payload_tx_cfg_cfg.loadMask = 0xFFFFFFFF;

			if (!sendMessage(UBX_MSG_CFG_CFG, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_cfg))) {
				return -1;
			}

			if (waitForAck(UBX_MSG_CFG_CFG, 2000, true) < 0) {
				return -1;
			}
		}

		// try CFG-VALSET: if we get an ACK we know we can use protocol version 27+
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_SPI_ENABLED, 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_SPI_MAXFF, 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIINPROT_UBX, 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIINPROT_RTCM3X, _output_mode == OutputMode::RTCM ? 0 : 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIINPROT_NMEA, 0);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIOUTPROT_UBX, 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIOUTPROT_RTCM3X, _output_mode == OutputMode::GPS ? 0 : 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIOUTPROT_NMEA, 0);

		bool cfg_valset_success = false;

		if (sendCfgValset()) {

			if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true) == 0) {
				cfg_valset_success = true;
			}
		}

		if (cfg_valset_success) {
			_proto_ver_27_or_higher = true;

		} else {
			_proto_ver_27_or_higher = false;
			memset(cfg_prt, 0, sizeof(ubx_payload_tx_cfg_prt_t));
			cfg_prt[0].portID		= UBX_TX_CFG_PRT_PORTID_SPI;
			cfg_prt[0].mode			= UBX_TX_CFG_PRT_MODE_SPI;
			cfg_prt[0].inProtoMask	= in_proto_mask;
			cfg_prt[0].outProtoMask	= out_proto_mask;

			if (!sendMessage(UBX_MSG_CFG_PRT, (uint8_t *)cfg_prt, sizeof(ubx_payload_tx_cfg_prt_t))) {
				return -1;
			}

			waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false);
		}

	} else {
		return -1;
	}

	UBX_DEBUG("Protocol version 27+: %i", static_cast<int>(_proto_ver_27_or_higher));

	/* Request module version information by sending an empty MON-VER message */
	if (!sendMessage(UBX_MSG_MON_VER, nullptr, 0)) {
		return -1;
	}

	/* Wait for the reply so that we know to which device we're connected (_board will be set).
	 * Note: we won't actually get an ACK-ACK, but UBX_MSG_MON_VER will also set the ack state.
	 */
	if (waitForAck(UBX_MSG_MON_VER, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}
	if (_output_mode == OutputMode::RTCM && baseStationCapability() == BaseStationCapability::Unsupported) {
		return -1;
	}

	/* Now that we know the board, update the baudrate on M8 boards (on F9+ we already used the
	 * higher baudrate with CFG-VALSET) */
	if (_interface == Interface::UART && auto_baudrate && _board == Board::u_blox8) {

		cfg_prt[0].baudRate	= UBX_BAUDRATE_M8_AND_NEWER;
		cfg_prt[1].baudRate	= UBX_BAUDRATE_M8_AND_NEWER;

		if (sendMessage(UBX_MSG_CFG_PRT, (uint8_t *)cfg_prt, 2 * sizeof(ubx_payload_tx_cfg_prt_t))) {
			/* no ACK is expected here, but read the buffer anyway in case we actually get an ACK */
			waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false);

			setBaudrate(UBX_BAUDRATE_M8_AND_NEWER);
			baudrate = UBX_BAUDRATE_M8_AND_NEWER;
		}
	}

	if (_output_mode == OutputMode::GPSAndRTCM || _output_mode == OutputMode::RTCM || _mode == UBXMode::MovingBaseUART1 || _ppk_output) {
		if (!_rtcm_parsing) {
			_rtcm_parsing = new RTCMParsing();
		}

		_rtcm_parsing->reset();
	}

	if (_output_mode == OutputMode::RTCM) {
		// RTCM mode force stationary dynamic model
		_dyn_model = 2;
	}

	int ret;

	/* Configure the device, use config commands depending on protocol version */
	if (_proto_ver_27_or_higher) {
		ret = configureDevice(config, _uart2_baudrate);

	} else {
		ret = configureDevicePreV27(config.gnss_systems);
	}

	if (ret != 0) {
		return ret;
	}

	// All GPS navigation modes, including heading rovers and moving bases, need time mode disabled.
	if (_output_mode != OutputMode::RTCM
	    && (_is_m8p || _board == Board::u_blox9_F9P_L1L2 || _board == Board::u_blox9_F9P_L1L5
	        || _board == Board::u_blox_X20)) {
		if (disableTimeMode() < 0) {
			return -1;
		}
	}

	if (_output_mode == OutputMode::RTCM) {
		if (restartSurveyIn() < 0) {
			return -1;
		}

	} else if (_output_mode == OutputMode::GPSAndRTCM) {
		if (activateRTCMOutput(false) < 0) {
			return -1;
		}
	}

	_configured = true;
	return output_protocol == OutputProtocol::NMEA ? enableNmeaOutput(baudrate) : 0;
}


int GPSDriverUBX::configureDevicePreV27(const GNSSSystemsMask &gnssSystems)
{
	/* Send a CFG-RATE message to define update rate */
	memset(&_buf.payload_tx_cfg_rate, 0, sizeof(_buf.payload_tx_cfg_rate));
	_buf.payload_tx_cfg_rate.measRate	= UBX_TX_CFG_RATE_MEASINTERVAL;
	_buf.payload_tx_cfg_rate.navRate	= UBX_TX_CFG_RATE_NAVRATE;
	_buf.payload_tx_cfg_rate.timeRef	= UBX_TX_CFG_RATE_TIMEREF;

	if (!sendMessage(UBX_MSG_CFG_RATE, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_rate))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_RATE, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}

	/* send a NAV5 message to set the options for the internal filter */
	memset(&_buf.payload_tx_cfg_nav5, 0, sizeof(_buf.payload_tx_cfg_nav5));
	_buf.payload_tx_cfg_nav5.mask		= UBX_TX_CFG_NAV5_MASK;
	_buf.payload_tx_cfg_nav5.dynModel	= _dyn_model;
	_buf.payload_tx_cfg_nav5.fixMode	= UBX_TX_CFG_NAV5_FIXMODE;

	if (!sendMessage(UBX_MSG_CFG_NAV5, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_nav5))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_NAV5, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}

	/* configure active GNSS systems (number of channels and used signals taken from U-Center default) */
	if (static_cast<int32_t>(gnssSystems) != 0) {
		memset(&_buf.payload_tx_cfg_gnss, 0, sizeof(_buf.payload_tx_cfg_gnss));
		_buf.payload_tx_cfg_gnss.msgVer = 0x00;
		_buf.payload_tx_cfg_gnss.numTrkChHw = 0x00;  // read only
		_buf.payload_tx_cfg_gnss.numTrkChUse = 0xFF;  // use max number of HW channels
		_buf.payload_tx_cfg_gnss.numConfigBlocks = 7;  // always configure all systems

		// GPS and QZSS should always be enabled and disabled together, according to uBlox
		_buf.payload_tx_cfg_gnss.block[0].gnssId = UBX_TX_CFG_GNSS_GNSSID_GPS;
		_buf.payload_tx_cfg_gnss.block[1].gnssId = UBX_TX_CFG_GNSS_GNSSID_QZSS;

		if (gnssSystems & GNSSSystemsMask::ENABLE_GPS) {
			UBX_DEBUG("GNSS Systems: Use GPS + QZSS");
			_buf.payload_tx_cfg_gnss.block[0].resTrkCh = 8;
			_buf.payload_tx_cfg_gnss.block[0].maxTrkCh = 16;
			_buf.payload_tx_cfg_gnss.block[0].flags = UBX_TX_CFG_GNSS_FLAGS_GPS_L1CA | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
			_buf.payload_tx_cfg_gnss.block[1].resTrkCh = 0;
			_buf.payload_tx_cfg_gnss.block[1].maxTrkCh = 3;
			_buf.payload_tx_cfg_gnss.block[1].flags = UBX_TX_CFG_GNSS_FLAGS_QZSS_L1CA | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
		}

		_buf.payload_tx_cfg_gnss.block[2].gnssId = UBX_TX_CFG_GNSS_GNSSID_SBAS;

		if (gnssSystems & GNSSSystemsMask::ENABLE_SBAS) {
			UBX_DEBUG("GNSS Systems: Use SBAS");
			_buf.payload_tx_cfg_gnss.block[2].resTrkCh = 1;
			_buf.payload_tx_cfg_gnss.block[2].maxTrkCh = 3;
			_buf.payload_tx_cfg_gnss.block[2].flags = UBX_TX_CFG_GNSS_FLAGS_SBAS_L1CA | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
		}

		_buf.payload_tx_cfg_gnss.block[3].gnssId = UBX_TX_CFG_GNSS_GNSSID_GALILEO;

		if (gnssSystems & GNSSSystemsMask::ENABLE_GALILEO) {
			UBX_DEBUG("GNSS Systems: Use Galileo");
			_buf.payload_tx_cfg_gnss.block[3].resTrkCh = 4;
			_buf.payload_tx_cfg_gnss.block[3].maxTrkCh = 8;
			_buf.payload_tx_cfg_gnss.block[3].flags = UBX_TX_CFG_GNSS_FLAGS_GALILEO_E1 | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
		}

		_buf.payload_tx_cfg_gnss.block[4].gnssId = UBX_TX_CFG_GNSS_GNSSID_BEIDOU;

		if (gnssSystems & GNSSSystemsMask::ENABLE_BEIDOU) {
			UBX_DEBUG("GNSS Systems: Use BeiDou");
			_buf.payload_tx_cfg_gnss.block[4].resTrkCh = 8;
			_buf.payload_tx_cfg_gnss.block[4].maxTrkCh = 16;
			_buf.payload_tx_cfg_gnss.block[4].flags = UBX_TX_CFG_GNSS_FLAGS_BEIDOU_B1I | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
		}

		_buf.payload_tx_cfg_gnss.block[5].gnssId = UBX_TX_CFG_GNSS_GNSSID_GLONASS;

		if (gnssSystems & GNSSSystemsMask::ENABLE_GLONASS) {
			UBX_DEBUG("GNSS Systems: Use GLONASS");
			_buf.payload_tx_cfg_gnss.block[5].resTrkCh = 8;
			_buf.payload_tx_cfg_gnss.block[5].maxTrkCh = 14;
			_buf.payload_tx_cfg_gnss.block[5].flags = UBX_TX_CFG_GNSS_FLAGS_GLONASS_L1 | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
		}

		// IMES always disabled
		_buf.payload_tx_cfg_gnss.block[6].gnssId = UBX_TX_CFG_GNSS_GNSSID_IMES;
		_buf.payload_tx_cfg_gnss.block[6].flags = 0;

		// send message
		if (!sendMessage(UBX_MSG_CFG_GNSS, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_gnss))) {
			UBX_DEBUG("UBX CFG-GNSS message send failed");
			return -1;
		}

		if (waitForAck(UBX_MSG_CFG_GNSS, UBX_CONFIG_TIMEOUT, true) < 0) {
			// The receiver rejects the configuration as a whole if it names a constellation it
			// cannot receive, e.g. BeiDou on a SAM-M8Q, or more of them than it can track at
			// once. Keep the receiver's own selection rather than losing the fix over it.
			UBX_WARN("GNSS constellation config rejected, keeping receiver config");
		}

		// On u-blox 8 the Galileo change only takes effect once the configuration has been
		// saved and the receiver hardware reset, which we cannot do without dropping the
		// rest of this session's configuration
		if (gnssSystems & GNSSSystemsMask::ENABLE_GALILEO) {
			UBX_WARN("Galileo needs a receiver power cycle to take effect");
		}

		waitForGnssReset();
	}

	/* configure message rates */
	/* the last argument is divisor for measurement rate (set by CFG RATE), i.e. 1 means 5Hz */

	/* try to set rate for NAV-PVT */
	/* (implemented for ubx7+ modules only, use NAV-SOL, NAV-POSLLH, NAV-VELNED and NAV-TIMEUTC for ubx6) */
	if (!configureMessageRate(UBX_MSG_NAV_PVT, 1)) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_MSG, UBX_CONFIG_TIMEOUT, true) < 0) {
		_use_nav_pvt = false;

	} else {
		_use_nav_pvt = true;
	}

	UBX_DEBUG("%susing NAV-PVT", _use_nav_pvt ? "" : "not ");

	if (!_use_nav_pvt) {
		if (!configureMessageRateAndAck(UBX_MSG_NAV_TIMEUTC, 5, true)) {
			return -1;
		}

		if (!configureMessageRateAndAck(UBX_MSG_NAV_POSLLH, 1, true)) {
			return -1;
		}

		if (!configureMessageRateAndAck(UBX_MSG_NAV_SOL, 1, true)) {
			return -1;
		}

		if (!configureMessageRateAndAck(UBX_MSG_NAV_VELNED, 1, true)) {
			return -1;
		}
	}

	if (!configureMessageRateAndAck(UBX_MSG_NAV_STATUS, 1, true)) {
		return -1;
	}

	if (!configureMessageRateAndAck(UBX_MSG_NAV_DOP, 1, true)) {
		return -1;
	}

	if (!configureMessageRateAndAck(UBX_MSG_NAV_SVINFO, (_satellite_info != nullptr) ? 5 : 0, true)) {
		return -1;
	}

	if (!configureMessageRateAndAck(UBX_MSG_MON_HW, 1, true)) {
		return -1;
	}

	return 0;
}

int GPSDriverUBX::configureDevice(const GPSConfig &config, const int32_t uart2_baudrate)
{
	// There is no RTCM or USB interface on M10
	if (_board != Board::u_blox10 && _board != Board::u_blox10_L1L5) {

		initCfgValset();

		const uint8_t enable_corrections_in = (_output_mode == OutputMode::RTCM) ? 0 : 1;

		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1INPROT_RTCM3X, enable_corrections_in);

		// USB
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_UBX, 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_RTCM3X, enable_corrections_in);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_NMEA, 0);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_UBX, 1);

		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA, 0);

		// Only RTCM-output-capable receivers expose these keys. M9 SPG rejects
		// the entire VALSET if they are included, even with a value of zero.
		if (_board == Board::u_blox9_F9P_L1L2 || _board == Board::u_blox9_F9P_L1L5
		    || _board == Board::u_blox_X20) {
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, _output_mode != OutputMode::GPS);
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X, _output_mode != OutputMode::GPS);
		}

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		// Optional SPARTN input (PointPerfect). Sent separately so modules without
		// SPARTN support can NACK without failing the rest of configuration.
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1INPROT_SPARTN, enable_corrections_in);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_SPARTN, enable_corrections_in);

		if (_interface == Interface::SPI) {
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_SPIINPROT_SPARTN, enable_corrections_in);
		}

		sendCfgValsetAcked(false);
	}

	/* set configuration parameters */
	initCfgValset();
	cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_FIXMODE, 3 /* Auto 2d/3d */);
	cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_UTCSTANDARD, 3 /* USNO (U.S. Naval Observatory derived from GPS) */);
	cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_DYNMODEL, _dyn_model);

	if (_min_cno != 0) {
		cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_INFIL_MINCNO, _min_cno);
	}

	if (_min_elev != 0) {
		cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_INFIL_MINELEV, _min_elev);
	}

	if (_dgnss_timeout != 0) {
		cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_CONSTR_DGNSSTO, _dgnss_timeout);
	}

	// measurement rate
	// M9N max rate is 8Hz for all satellites, above 8Hz the number of used satellites is restricted to 16.
	// F9P L1L2 in firmware <1.50 the max update rate with 4 constellations is 9Hz without RTK and 7Hz with RTK
	// F9P L1L2 in firmware >=1.50 the max update rate with 4 constellations is 7Hz without RTK and 5Hz with RTK
	// F9P L1L5 the max update rate with 4 constellations is 8Hz without RTK and 7Hz with RTK
	// DAN-F10N the max update rate is 10Hz with GPS+GAL+BDS(Default)
	// X20 max update rate is 25Hz, but 25Hz at 115200 baud causes high dropouts, especially with RTK. So default 10Hz is selected.
	// Receivers such as M9N and DAN-F10N can go higher than 10Hz, but the number of used satellites will be restricted to 16. (Not mentioned in datasheet)
	int rate_meas = 100; // 10Hz

	if (_output_rate > 0) {

		if (_output_rate > 25) {
			UBX_WARN("Rate %u Hz exceeds max, limiting to 25Hz", _output_rate);
			_output_rate = 25;
		}

		// convert hz to ms
		rate_meas = 1000 / _output_rate;

	} else {
		switch (_board) {
		case Board::u_blox9:
			rate_meas = 125; // 8Hz
			break;

		case Board::u_blox9_F9P_L1L2:
			rate_meas = 200; // 5Hz
			break;

		case Board::u_blox9_F9P_L1L5:
			rate_meas = 200; // 5Hz
			break;

		default:
			break;
		}
	}

	cfgValset<uint16_t>(UBX_CFG_KEY_RATE_MEAS, rate_meas);
	cfgValset<uint16_t>(UBX_CFG_KEY_RATE_NAV, 1);
	cfgValset<uint8_t>(UBX_CFG_KEY_RATE_TIMEREF, 0);

	if (sendCfgValsetAcked() < 0) {
		return -1;
	}

	// Disable odometer. Separate, non-fatal VALSET: CFG-ODO-* was removed on the
	// F20 platform (ZED-X20P HPG 2.10+), so a NAK here must not abort config.
	initCfgValset();
	cfgValset<uint8_t>(UBX_CFG_KEY_ODO_USE_ODO, 0);

	// M9 (SPG) only has USE_ODO and PROFILE in CFG-ODO
	if (_board != Board::u_blox9) {
		static constexpr uint32_t odo_keys[] = {UBX_CFG_KEY_ODO_USE_COG, UBX_CFG_KEY_ODO_OUTLPVEL, UBX_CFG_KEY_ODO_OUTLPCOG};
		cfgValset(odo_keys, 0);
	}

	sendCfgValsetAcked(false);

	// RTK (optional, as only RTK devices like F9P support it)
	initCfgValset();
	cfgValset<uint8_t>(UBX_CFG_KEY_NAVHPG_DGNSSMODE, 3 /* RTK Fixed */);

	if (!sendCfgValset()) {
		return -1;
	}

	waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false);

	// Jamming detection. Firmware with CFG-SEC-JAMDET (F9 HPG 1.50+, F9 L1L5, F20/X20) has
	// detection always on and no CFG-ITFM; everything older has CFG-ITFM and no JAMDET key.
	// A NAK on the sensitivity key is therefore the signal that the monitor still needs enabling.
	initCfgValset();
	cfgValset<uint8_t>(UBX_CFG_KEY_SEC_JAMDET_SENSITIVITY_HI, _jam_det_sensitivity_hi ? 1 : 0);

	if (sendCfgValsetAcked(false) < 0) {
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_ITFM_ENABLE, 1);

		if (sendCfgValsetAcked(false) < 0) {
			UBX_WARN("Jamming monitor not supported by this receiver");
		}
	}

	// configure active GNSS systems (leave signal bands as is)
	// Note: For M10 configuration if changing from default. As per the
	//       MAX-M10S integration guide UBX-20053088 - R03, see section
	//       2.1.1.3 GNSS signal configuration for details on some restrictions.
	//       Implementing these restrictions are a TODO item for M10.
	if (static_cast<int32_t>(config.gnss_systems) != 0) {
		initCfgValset();

		// GPS and QZSS should always be enabled and disabled together, according to uBlox
		if (config.gnss_systems & GNSSSystemsMask::ENABLE_GPS) {
			UBX_DEBUG("GNSS Systems: Use GPS + QZSS");
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_ENA, 1);
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_ENA, 1);
			UBX_DEBUG("GNSS Systems: Enable QZSS L1CA");
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L1CA_ENA, 1);

			// M9 (SPG) has no CFG-SIGNAL key for QZSS L1S
			if (_board != Board::u_blox9) {
				UBX_DEBUG("GNSS Systems: Enable QZSS L1S");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L1S_ENA, 1);
			}

			if (_board == Board::u_blox_X20) {
				UBX_DEBUG("GNSS Systems: Use GPS L2C + L5, QZSS L2C + L5");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L2C_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L5_ENA, 1);

			} else if (_board == Board::u_blox9_F9P_L1L2) {
				UBX_DEBUG("GNSS Systems: Use GPS L2C");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 1);
				UBX_DEBUG("GNSS Systems: Enable QZSS L2C");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L2C_ENA, 1);

			} else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
				UBX_DEBUG("GNSS Systems: Use GPS L5");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 1);
				UBX_DEBUG("GNSS Systems: Use QZSS L5");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L5_ENA, 1);
			}

		} else {
			UBX_DEBUG("GNSS Systems: Disable GPS + QZSS");

			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_ENA, 0);
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_ENA, 0);

			if (_board == Board::u_blox_X20) {
				UBX_DEBUG("GNSS Systems: Disable GPS L2C + L5");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 0);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 0);

			} else if (_board == Board::u_blox9_F9P_L1L2) {
				UBX_DEBUG("GNSS Systems: Disable GPS L2C");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 0);

			} else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
				UBX_DEBUG("GNSS Systems: Disable GPS L5");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 0);
			}
		}

		if (config.gnss_systems & GNSSSystemsMask::ENABLE_GALILEO) {
			UBX_DEBUG("GNSS Systems: Use Galileo");
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_ENA, 1);

			if (_board == Board::u_blox_X20) {
				UBX_DEBUG("GNSS Systems: Use Galileo E5A + E6");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E6_ENA, 1);

			} else if (_board == Board::u_blox9_F9P_L1L2) {
				UBX_DEBUG("GNSS Systems: Use Galileo E5B");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5B_ENA, 1);

			} else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
				UBX_DEBUG("GNSS Systems: Use Galileo E5A");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 1);
			}

		} else {
			UBX_DEBUG("GNSS Systems: Disable Galileo");

			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_ENA, 0);

			if (_board == Board::u_blox_X20) {
				UBX_DEBUG("GNSS Systems: Disable Galileo E5A + E6");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 0);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E6_ENA, 0);

			} else if (_board == Board::u_blox9_F9P_L1L2) {
				UBX_DEBUG("GNSS Systems: Disable Galileo E5B");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5B_ENA, 0);

			} else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
				UBX_DEBUG("GNSS Systems: Disable Galileo E5A");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 0);
			}
		}

		if (config.gnss_systems & GNSSSystemsMask::ENABLE_BEIDOU) {
			UBX_DEBUG("GNSS Systems: Use BeiDou");
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_ENA, 1);

			if (_board == Board::u_blox_X20) {
				UBX_DEBUG("GNSS Systems: Use BeiDou B1C + B2A + B3");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B1C_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B3_ENA, 1);

			} else if (_board == Board::u_blox9_F9P_L1L2) {
				UBX_DEBUG("GNSS Systems: Use BeiDou B2");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2_ENA, 1);

			} else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
				UBX_DEBUG("GNSS Systems: Use BeiDou B2A");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 1);
			}

		} else {
			UBX_DEBUG("GNSS Systems: Disable BeiDou");

			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_ENA, 0);

			if (_board == Board::u_blox_X20) {
				UBX_DEBUG("GNSS Systems: Disable BeiDou B1C + B2A + B3");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B1C_ENA, 0);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 0);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B3_ENA, 0);

			} else if (_board == Board::u_blox9_F9P_L1L2) {
				UBX_DEBUG("GNSS Systems: Disable BeiDou B2");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2_ENA, 0);

			} else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
				UBX_DEBUG("GNSS Systems: Disable BeiDou B2A");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 0);
			}
		}

		// GLONASS is not supported on DAN-F10N and X20
		if (_board != Board::u_blox10_L1L5 && _board != Board::u_blox_X20) {
			if (config.gnss_systems & GNSSSystemsMask::ENABLE_GLONASS) {
				UBX_DEBUG("GNSS Systems: Use GLONASS");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L1_ENA, 1);

				if (_board == Board::u_blox9_F9P_L1L2) {
					UBX_DEBUG("GNSS Systems: Use GLONASS L2C");
					cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L2_ENA, 1);
				}

			} else {
				UBX_DEBUG("GNSS Systems: Disable GLONASS");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_ENA, 0);
				// cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L1_ENA, 0);

				if (_board == Board::u_blox9_F9P_L1L2) {
					UBX_DEBUG("GNSS Systems: Disable GLONASS L2C");
					cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L2_ENA, 0);
				}
			}
		}

		if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5 || _board == Board::u_blox_X20) {
			if (config.gnss_systems & GNSSSystemsMask::ENABLE_NAVIC) {
				UBX_DEBUG("GNSS Systems: Use NavIC");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_ENA, 1);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_L5_ENA, 1);

			} else {
				UBX_DEBUG("GNSS Systems: Disable NavIC");
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_ENA, 0);
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_L5_ENA, 0);
			}
		}

		if (!sendCfgValset()) {
			UBX_DEBUG("UBX GNSS config send failed");
			return -1;
		}

		if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true) < 0) {
			// The receiver NAKs the whole message and applies nothing if it does not know a
			// single key, so a signal key missing on this generation would leave the receiver
			// unconfigured. Retry with the constellation enables, those exist everywhere.
			UBX_WARN("GNSS signal config rejected, retrying without signal bands");

			initCfgValset();

			const uint8_t use_gps = (config.gnss_systems & GNSSSystemsMask::ENABLE_GPS) ? 1 : 0;
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_ENA, use_gps);
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_ENA, use_gps);
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_ENA, (config.gnss_systems & GNSSSystemsMask::ENABLE_GALILEO) ? 1 : 0);
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_ENA, (config.gnss_systems & GNSSSystemsMask::ENABLE_BEIDOU) ? 1 : 0);

			if (_board != Board::u_blox10_L1L5 && _board != Board::u_blox_X20) {
				cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_ENA, (config.gnss_systems & GNSSSystemsMask::ENABLE_GLONASS) ? 1 : 0);
			}

			if (!sendCfgValset()) {
				return -1;
			}

			if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true) < 0) {
				// Keep going with whatever the receiver already has, a refused constellation
				// selection must not cost us the fix
				UBX_WARN("GNSS constellation config rejected, keeping receiver config");
			}
		}

		waitForGnssReset();

		// send SBAS config separately, because it seems to be buggy (with u-center, too)
		initCfgValset();

		if (config.gnss_systems & GNSSSystemsMask::ENABLE_SBAS) {
			UBX_DEBUG("GNSS Systems: Use SBAS");
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_SBAS_ENA, 1);
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_SBAS_L1CA_ENA, 1);

		} else {
			cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_SBAS_ENA, 0);
		}

		if (!sendCfgValset()) {
			return -1;
		}

		waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true);

		waitForGnssReset();
	}

	// GPS L5 is broadcast unhealthy while it is pre-operational, so tell the receiver to take
	// the L1 health flag instead. The key is not in any interface description, only in app note
	// UBX-21038688, so it gets a message of its own rather than putting the constellation
	// config at the mercy of a firmware that has never heard of it.
	if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5 || _board == Board::u_blox_X20) {
		const bool use_gps = (static_cast<int32_t>(config.gnss_systems) == 0)
				     || (config.gnss_systems & GNSSSystemsMask::ENABLE_GPS);

		UBX_DEBUG("%s L5 health override", use_gps ? "Enabling" : "Disabling");

		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_L5_HEALTH_OVERRIDE, use_gps ? 1 : 0);

		if (sendCfgValsetAcked(false) < 0) {
			UBX_WARN("GPS L5 health override not supported by this receiver");
		}
	}

	// Configure message rates
	// Send a new CFG-VALSET message to make sure it does not get too large
	initCfgValset();
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_PVT_I2C, 1);

	// There is no RTCM on M10 and M9* (except F9P)
	if (_board != Board::u_blox10 && _board != Board::u_blox9 && _board != Board::u_blox10_L1L5) {
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_RELPOSNED_I2C,
			      _mode == UBXMode::RoverWithMovingBaseUART2 || _mode == UBXMode::RoverWithMovingBaseUART1 ? 1 : 0);
	}

	_use_nav_pvt = true;
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_DOP_I2C, 1);
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SAT_I2C, (_satellite_info != nullptr) ? 10 : 0);
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_STATUS_I2C, 1);
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_MON_RF_I2C, 1);
	_got_sec_sig = false;

	if (sendCfgValsetAcked() < 0) {
		return -1;
	}

	// Correction input status. RXM-COR reports every protocol (RTCM3, SPARTN, HAS) and is
	// the only form on the X20, which has no RXM-RTCM; receivers without it get RXM-RTCM.
	initCfgValset();
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_COR_I2C, 1);

	if (sendCfgValsetAcked(false) < 0
	    && ((_board == Board::u_blox9) || (_board == Board::u_blox9_F9P_L1L2) || (_board == Board::u_blox9_F9P_L1L5))) {
		initCfgValset();
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_RTCM_I2C, 1);
		sendCfgValsetAcked(false);
	}

	// UBX-SEC-SIG carries jammingState. MON-RF jammingState is always 0 on
	// firmware that supports this message (F9 HPG 1.51, X20). The rate key is
	// absent on older protocol-27 receivers; a NAK must not abort config.
	initCfgValset();
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_SEC_SIG_I2C, 1);

	if (sendCfgValsetAcked(false) < 0) {
		UBX_DEBUG("UBX-SEC-SIG not supported by this receiver");
	}

	// Explicitly disable the messages this driver never consumes. We do not enable them,
	// but they can be left on in the receiver's non-volatile config by other software
	// (u-center, or another firmware such as ArduPilot, which writes CFG-VALSET to the
	// BBR/FLASH layers). Clearing them here rather than waiting for payloadRxInit() to
	// notice them avoids wasting UART bandwidth on every boot.
	// Separate, non-fatal VALSET: these messages do not exist on every generation, and
	// bandwidth we failed to save is not worth losing the receiver over.
	initCfgValset();
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_TIMEGPS_I2C, 0);
	cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_SFRBX_I2C, 0);

	// M10 and F10 have no raw measurement output
	if (_board != Board::u_blox10 && _board != Board::u_blox10_L1L5) {
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_RAWX_I2C, 0);
	}

	if (sendCfgValsetAcked(false) < 0) {
		UBX_WARN("Could not disable unused messages");
	}

	// Dual antenna heading, not used in a moving base setup where NAV-RELPOSNED provides it. The rate is
	// always written so a mode change is idempotent; a NAK from a position-only X20P must not abort config.
	// The receiver side heading offset is zeroed, leaving GPS_YAW_OFFSET as the only offset applied.
	if (_board == Board::u_blox_X20) {
		const bool moving_base_setup = (_mode == UBXMode::RoverWithMovingBaseUART2)
					       || (_mode == UBXMode::RoverWithMovingBaseUART1)
					       || isMovingBase();

		initCfgValset();
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_DAHEADING_I2C, moving_base_setup ? 0 : 1);
		cfgValset<int32_t>(UBX_CFG_KEY_NAVSPG_DAHEADING_OFFSET, 0);

		if (sendCfgValset()) {
			if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false) < 0) {
				UBX_DEBUG("NAV-DAHEADING not supported by this receiver");
			}
		}

		// Galileo HAS (HPG 2.10+) is only processed while host corrections are off, so the pair is
		// written together and every other mode restores host input: a HOST=0 left behind by
		// u-center would silently discard RTCM. Absent before 2.10, so a NAK must not abort config.
		const bool use_has = (_mode == UBXMode::GalileoHAS);
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_NAVCOR_ENABLE_HOST, use_has ? 0 : 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_NAVCOR_ENABLE_GAL_HAS, use_has ? 1 : 0);

		if (sendCfgValsetAcked(false) < 0) {
			if (use_has) {
				UBX_WARN("Galileo HAS not supported by this receiver (needs HPG 2.10)");
			}

		} else if (use_has) {
			// TODO: nothing reports whether HAS corrections are actually applied beyond carrSoln
			// going float; RXM-COR with protocol 5 is the signal, and corrections_protocol carries
			// it. HAS and OSNMA are mutually exclusive on HPG 2.10, so an OSNMA option has to
			// refuse this mode.
			GPS_INFO("Galileo HAS enabled, host corrections disabled");
		}

	} else if (_mode == UBXMode::GalileoHAS) {
		UBX_WARN("Galileo HAS needs a ZED-X20P, running without corrections");
	}

	if (_interface == Interface::UART || _interface == Interface::SPI) {

		// Enable/Disable GPS protocols at I2C interface
		initCfgValset();

		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2CINPROT_UBX,
				   config.interface_protocols & InterfaceProtocolsMask::I2C_IN_PROT_UBX);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2CINPROT_NMEA,
				   config.interface_protocols & InterfaceProtocolsMask::I2C_IN_PROT_NMEA);

		// There is no RTCM on M10
		if (_board != Board::u_blox10 && _board != Board::u_blox10_L1L5) {
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2CINPROT_RTCM3X,
					   config.interface_protocols & InterfaceProtocolsMask::I2C_IN_PROT_RTCM3X);
		}

		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2COUTPROT_UBX,
				   config.interface_protocols & InterfaceProtocolsMask::I2C_OUT_PROT_UBX);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2COUTPROT_NMEA,
				   config.interface_protocols & InterfaceProtocolsMask::I2C_OUT_PROT_NMEA);

		if ((_board == Board::u_blox9_F9P_L1L2) || (_board == Board::u_blox9_F9P_L1L5)) {
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2COUTPROT_RTCM3X,
					   config.interface_protocols & InterfaceProtocolsMask::I2C_OUT_PROT_RTCM3X);
		}

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		// Optional SPARTN on I2C (best-effort; NACK is fine on non-SPARTN firmware)
		if (_board != Board::u_blox10 && _board != Board::u_blox10_L1L5) {
			initCfgValset();
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_I2CINPROT_SPARTN,
					   config.interface_protocols & InterfaceProtocolsMask::I2C_IN_PROT_RTCM3X);

			sendCfgValsetAcked(false);
		}
	}

	if ((_mode == UBXMode::Normal || _mode == UBXMode::GalileoHAS) && _ppk_output) {
		UBX_DEBUG("Configuring Normal with MSM7 output");
		initCfgValset();

		// Enable output protocols on UART1
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1);
		cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, 1);

		// Configure MSM7 message outputs on UART1
		cfgValset<uint8_t>(UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1230_UART1, 1); // GLONASS bias
		cfgValset(RTCM_MSM7_UART1, 1);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

	} else if (_mode == UBXMode::RoverWithStaticBaseUART2 || _mode == UBXMode::RoverWithMovingBaseUART2) {
		UBX_DEBUG("Configuring UART2 for rover");
		// UBX only on UART1, RTCM input on UART2
		static constexpr CfgValsetItem rover_uart2[] = {
			{UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1},
			{UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, 0},
			{UBX_CFG_KEY_CFG_UART2_STOPBITS, 1},
			{UBX_CFG_KEY_CFG_UART2_DATABITS, 0},
			{UBX_CFG_KEY_CFG_UART2_PARITY, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_UBX, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_RTCM3X, 1},
			{UBX_CFG_KEY_CFG_UART2INPROT_NMEA, 0},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_UBX, 0},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_RTCM3X, 0},
		};
		initCfgValset();
		cfgValset(rover_uart2);
		cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART2_BAUDRATE, uart2_baudrate);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		GPS_INFO("UART2: RTCM3 in @ %d baud (%s)", (int)uart2_baudrate,
			 _mode == UBXMode::RoverWithMovingBaseUART2 ? "moving base" : "static base");

	} else if (_mode == UBXMode::MovingBaseUART2) {
		UBX_DEBUG("Configuring UART2 for moving base");
		// RTCM output on UART2
		static constexpr CfgValsetItem moving_base_uart2[] = {
			{UBX_CFG_KEY_CFG_UART2_STOPBITS, 1},
			{UBX_CFG_KEY_CFG_UART2_DATABITS, 0},
			{UBX_CFG_KEY_CFG_UART2_PARITY, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_UBX, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_RTCM3X, 1},
			{UBX_CFG_KEY_CFG_UART2INPROT_NMEA, 0},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_UBX, 0},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_RTCM3X, 1},
			{UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1230_UART2, 1}, // GLONASS bias
		};
		initCfgValset();
		cfgValset(moving_base_uart2);
		cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART2_BAUDRATE, uart2_baudrate);
		// MSM7 for PPK, MSM4 otherwise, never both
		cfgValset(RTCM_MSM7_UART2, _ppk_output ? 1 : 0);
		cfgValset(RTCM_MSM4_UART2, _ppk_output ? 0 : 1);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		GPS_INFO("UART2: RTCM3 out @ %d baud (rover)", (int)uart2_baudrate);

		// 4072.0 marks the base as moving, without it the rover solves against it as a
		// static base and never reports a heading. The F9P-15B doesn't support 4072, and
		// neither does the X20 before HPG 2.10, so it goes out on its own: losing the
		// heading is bad, losing the receiver is worse.
		if (_board == Board::u_blox9_F9P_L1L2 || _board == Board::u_blox_X20) {
			UBX_DEBUG("Configuring ublox 4072");
			initCfgValset();
			cfgValset<uint8_t>(UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE4072_0_UART2, 1);

			if (sendCfgValsetAcked(false) < 0) {
				UBX_WARN("RTCM 4072.0 not supported, no moving base heading");
			}
		}

	} else if (_mode == UBXMode::RoverWithMovingBaseUART1) {
		UBX_DEBUG("Configuring UART1 for rover");
		// RTCM input on UART1
		static constexpr CfgValsetItem rover_uart1[] = {
			{UBX_CFG_KEY_CFG_UART1INPROT_UBX, 1},
			{UBX_CFG_KEY_CFG_UART1INPROT_RTCM3X, 1},
			{UBX_CFG_KEY_CFG_UART1INPROT_NMEA, 0},
			{UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1},
			{UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, 0},
		};
		initCfgValset();
		cfgValset(rover_uart1);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

	} else if (_mode == UBXMode::MovingBaseUART1) {
		UBX_DEBUG("Configuring UART1 for moving base");
		// RTCM output on UART1
		static constexpr CfgValsetItem moving_base_uart1[] = {
			{UBX_CFG_KEY_CFG_UART1INPROT_UBX, 1},
			{UBX_CFG_KEY_CFG_UART1INPROT_RTCM3X, 1},
			{UBX_CFG_KEY_CFG_UART1INPROT_NMEA, 0},
			{UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1},
			{UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, 1},
			{UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1230_UART1, 1}, // GLONASS bias
		};
		initCfgValset();
		cfgValset(moving_base_uart1);
		// MSM7 for PPK, MSM4 otherwise, never both
		cfgValset(RTCM_MSM7_UART1, _ppk_output ? 1 : 0);
		cfgValset(RTCM_MSM4_UART1, _ppk_output ? 0 : 1);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		// 4072.0 marks the base as moving, without it the rover solves against it as a
		// static base and never reports a heading. The F9P-15B doesn't support 4072, and
		// neither does the X20 before HPG 2.10, so it goes out on its own: losing the
		// heading is bad, losing the receiver is worse.
		if (_board == Board::u_blox9_F9P_L1L2 || _board == Board::u_blox_X20) {
			UBX_DEBUG("Configuring ublox 4072");
			initCfgValset();
			cfgValset<uint8_t>(UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE4072_0_UART1, 1);

			if (sendCfgValsetAcked(false) < 0) {
				UBX_WARN("RTCM 4072.0 not supported, no moving base heading");
			}
		}

	} else if (_mode == UBXMode::GroundControlStation) {
		UBX_DEBUG("Configuring UART2 for Ground Control Station");
		// NMEA output only on UART2
		static constexpr CfgValsetItem gcs_uart2[] = {
			{UBX_CFG_KEY_CFG_UART2_STOPBITS, 1},
			{UBX_CFG_KEY_CFG_UART2_DATABITS, 0},
			{UBX_CFG_KEY_CFG_UART2_PARITY, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_UBX, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_RTCM3X, 0},
			{UBX_CFG_KEY_CFG_UART2INPROT_NMEA, 0},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_NMEA, 1},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_UBX, 0},
			{UBX_CFG_KEY_CFG_UART2OUTPROT_RTCM3X, 0},
		};
		initCfgValset();
		cfgValset(gcs_uart2);
		cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART2_BAUDRATE, uart2_baudrate);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		GPS_INFO("UART2: NMEA out @ %d baud (ground control station)", (int)uart2_baudrate);

	} else if (_mode == UBXMode::UCenterUART2) {
		// Diagnostic port. UART1 keeps carrying the flight controller's own session, UART2 gets
		// UBX in both directions so u-center can watch and poll the receiver. Nothing below is
		// fatal: a debug port that will not come up must not cost us the GPS.
		if (_board == Board::u_blox10 || _board == Board::u_blox10_L1L5) {
			UBX_WARN("Receiver has no UART2, u-center mode not configured");

		} else {
			// On its own, because the platforms that wire UART2 permanently on have no such key
			// and a NAK would otherwise take the port settings down with it.
			initCfgValset();
			cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART2_ENABLED, 1);

			sendCfgValsetAcked(false);

			// Input stays open so u-center can poll MON-VER and drive the configuration view
			static constexpr CfgValsetItem ucenter_uart2[] = {
				{UBX_CFG_KEY_CFG_UART2_STOPBITS, 1},
				{UBX_CFG_KEY_CFG_UART2_DATABITS, 0},
				{UBX_CFG_KEY_CFG_UART2_PARITY, 0},
				{UBX_CFG_KEY_CFG_UART2INPROT_UBX, 1},
				{UBX_CFG_KEY_CFG_UART2INPROT_NMEA, 0},
				{UBX_CFG_KEY_CFG_UART2INPROT_RTCM3X, 0},
				{UBX_CFG_KEY_CFG_UART2OUTPROT_UBX, 1},
				{UBX_CFG_KEY_CFG_UART2OUTPROT_NMEA, 0},
				{UBX_CFG_KEY_CFG_UART2OUTPROT_RTCM3X, 0},
			};
			initCfgValset();
			cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART2_BAUDRATE, uart2_baudrate);
			cfgValset(ucenter_uart2);

			if (sendCfgValset()) {
				if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false) < 0) {
					UBX_WARN("UART2 port config for u-center rejected");
				}
			}

			// Message rates are per port, so the set enabled for the driver's own port leaves UART2
			// silent. u-center shows nothing at all without these.
			static constexpr uint32_t ucenter_msgout[] = {
				UBX_CFG_KEY_MSGOUT_UBX_NAV_PVT_I2C, UBX_CFG_KEY_MSGOUT_UBX_NAV_DOP_I2C,
				UBX_CFG_KEY_MSGOUT_UBX_NAV_STATUS_I2C, UBX_CFG_KEY_MSGOUT_UBX_MON_RF_I2C
			};
			initCfgValset();
			cfgValsetUart2(ucenter_msgout, 1);
			// NAV-SAT is 12 bytes per satellite against ~250 for all of the above together, so it
			// gets the same decimation the driver uses for itself rather than setting the link speed
			cfgValsetUart2(UBX_CFG_KEY_MSGOUT_UBX_NAV_SAT_I2C, 10);

			if (_board != Board::u_blox9) {
				cfgValsetUart2(UBX_CFG_KEY_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);
			}

			if (_board == Board::u_blox9 || _board == Board::u_blox9_F9P_L1L2 || _board == Board::u_blox9_F9P_L1L5) {
				cfgValsetUart2(UBX_CFG_KEY_MSGOUT_UBX_RXM_RTCM_I2C, 1);
			}

			if (sendCfgValset()) {
				if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false) < 0) {
					UBX_WARN("UART2 message rates for u-center rejected");
				}
			}

			initCfgValset();
			cfgValsetUart2(UBX_CFG_KEY_MSGOUT_UBX_SEC_SIG_I2C, 1);
			sendCfgValsetAcked(false);

			GPS_INFO("UART2: UBX in/out @ %d baud (u-center)", (int)uart2_baudrate);
		}
	}

	return 0;
}

const char *GPSDriverUBX::uart1Protocols(UBXMode mode, bool ppk_output)
{
	switch (mode) {
	case UBXMode::Normal:
	case UBXMode::GalileoHAS: return ppk_output ? "UBX in/out + RTCM3 out" : "UBX in/out";

	case UBXMode::RoverWithMovingBaseUART2:
	case UBXMode::RoverWithStaticBaseUART2: return "UBX out";

	case UBXMode::RoverWithMovingBaseUART1: return "UBX in/out + RTCM3 in";

	case UBXMode::MovingBaseUART1: return "UBX in/out + RTCM3 in/out";

	case UBXMode::MovingBaseUART2:
	case UBXMode::GroundControlStation:
	case UBXMode::UCenterUART2: return "UBX in/out";
	}

	return "UBX in/out";
}

void GPSDriverUBX::initCfgValset()
{
	static_assert(sizeof(_tx_cfg_valset_buf) >= sizeof(ubx_payload_tx_cfg_valset_t),
		      "_tx_cfg_valset_buf must hold at least the CFG-VALSET header");
	auto *header = reinterpret_cast<ubx_payload_tx_cfg_valset_t *>(_tx_cfg_valset_buf);
	memset(_tx_cfg_valset_buf, 0, sizeof(_tx_cfg_valset_buf));
	header->layers = UBX_CFG_LAYER_RAM;
	_tx_cfg_valset_size = sizeof(*header) - sizeof(header->cfgData);
}

bool GPSDriverUBX::sendCfgValset()
{
	return sendMessage(UBX_MSG_CFG_VALSET, _tx_cfg_valset_buf, _tx_cfg_valset_size);
}

int GPSDriverUBX::sendCfgValsetAcked(bool report_ack_error)
{
	if (!sendCfgValset()) {
		return -1;
	}

	return waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, report_ack_error);
}

bool GPSDriverUBX::cfgValsetRaw(uint32_t key_id, uint32_t value)
{
	// Size field: 1 = L, 2 = U1/I1/E1/X1, 3 = 2 bytes, 4 = 4 bytes (5 = 8 bytes, unsupported here)
	const unsigned size_field = (key_id >> 28) & 0x7;
	const unsigned value_size = (size_field <= 2) ? 1 : (size_field == 3) ? 2 : 4;

	if (_tx_cfg_valset_size + sizeof(key_id) + value_size > sizeof(_tx_cfg_valset_buf)) {
		// If this ever fires, either bump UBX_CFG_VALSET_BUF_SIZE or split the
		// batch into multiple CFG-VALSET messages at the call site.
		UBX_WARN("buf for CFG_VALSET too small");
		return false;
	}

	memcpy(_tx_cfg_valset_buf + _tx_cfg_valset_size, &key_id, sizeof(key_id));
	_tx_cfg_valset_size += sizeof(key_id);
	// little-endian: the low value_size bytes of value are the narrower type's bytes
	memcpy(_tx_cfg_valset_buf + _tx_cfg_valset_size, &value, value_size);
	_tx_cfg_valset_size += value_size;
	return true;
}

bool GPSDriverUBX::cfgValsetPort(uint32_t key_id, uint8_t value)
{
	if (_interface == Interface::SPI) {
		if (!cfgValset<uint8_t>(key_id + 4, value)) {
			return false;
		}

	} else {
		// enable on UART1 & USB (TODO: should we enable UART2 too? -> better would be to detect the port)
		if (!cfgValset<uint8_t>(key_id + 1, value)) {
			return false;
		}

		// M10 has no USB
		if (_board != Board::u_blox10 && _board != Board::u_blox10_L1L5) {
			if (!cfgValset<uint8_t>(key_id + 3, value)) {
				return false;
			}
		}
	}

	return true;
}

bool GPSDriverUBX::cfgValsetUart2(uint32_t key_id, uint8_t value)
{
	return cfgValset<uint8_t>(key_id + 2, value);
}

bool GPSDriverUBX::cfgValsetItems(const CfgValsetItem *items, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (!cfgValsetRaw(items[i].key, items[i].value)) {
			return false;
		}
	}

	return true;
}

bool GPSDriverUBX::cfgValsetKeys(const uint32_t *keys, size_t count, uint8_t value)
{
	for (size_t i = 0; i < count; i++) {
		if (!cfgValsetRaw(keys[i], value)) {
			return false;
		}
	}

	return true;
}

bool GPSDriverUBX::cfgValsetPortKeys(const uint32_t *keys, size_t count, uint8_t value)
{
	for (size_t i = 0; i < count; i++) {
		if (!cfgValsetPort(keys[i], value)) {
			return false;
		}
	}

	return true;
}

bool GPSDriverUBX::cfgValsetUart2Keys(const uint32_t *keys, size_t count, uint8_t value)
{
	for (size_t i = 0; i < count; i++) {
		if (!cfgValsetUart2(keys[i], value)) {
			return false;
		}
	}

	return true;
}

int GPSDriverUBX::disableTimeMode()
{
	// GPS output alone does not release a receiver previously configured as a fixed base.
	if (_proto_ver_27_or_higher) {
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 0);
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 0);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

	} else {
		if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 0, true)) {
			return -1;
		}

		memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));

		if (!sendMessage(UBX_MSG_CFG_TMODE3, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_tmode3))
		    || waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
			return -1;
		}
	}

	_survey_in_stopped = false;
	const gps_abstime stop_deadline = gps_absolute_time() + 3000000;

	while (!_survey_in_stopped && gps_absolute_time() < stop_deadline) {
		if (!sendMessage(UBX_MSG_NAV_SVIN, nullptr, 0)) {
			return -1;
		}

		const gps_abstime poll_deadline = gps_absolute_time() + 100000;

		while (!_survey_in_stopped && gps_absolute_time() < poll_deadline) {
			bool read_error;
			receiveInternal(100, read_error);

			if (read_error) {
				return -1;
			}
		}
	}

	if (!_survey_in_stopped) {
		UBX_WARN("Time mode did not stop");
		return -1;
	}

	return 0;
}

int GPSDriverUBX::restartSurveyInPreV27()
{
	UBX_DEBUG("restartSurveyInPreV27");

	//disable RTCM (MSM7) output
	configureMessageRate(UBX_MSG_RTCM3_1005, 0);
	configureMessageRate(UBX_MSG_RTCM3_1077, 0);
	configureMessageRate(UBX_MSG_RTCM3_1087, 0);
	configureMessageRate(UBX_MSG_RTCM3_1230, 0);
	configureMessageRate(UBX_MSG_RTCM3_1097, 0);
	configureMessageRate(UBX_MSG_RTCM3_1127, 0);

	//stop it first
	//FIXME: stopping the survey-in process does not seem to work
	memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
	_buf.payload_tx_cfg_tmode3.flags        = 0; /* disable time mode */

	if (!sendMessage(UBX_MSG_CFG_TMODE3, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_tmode3))) {
		UBX_WARN("TMODE3 failed. Device w/o base station support?");
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}

	if (_base_settings.type == BaseSettingsType::survey_in) {
		UBX_DEBUG("Starting Survey-in");

		memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
		_buf.payload_tx_cfg_tmode3.flags        = 1; /* start survey-in */
		_buf.payload_tx_cfg_tmode3.svinMinDur   = _base_settings.settings.survey_in.min_dur;
		_buf.payload_tx_cfg_tmode3.svinAccLimit = _base_settings.settings.survey_in.acc_limit;

		if (!sendMessage(UBX_MSG_CFG_TMODE3, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_tmode3))) {
			return -1;
		}

		if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
			return -1;
		}

		/* enable status output of survey-in */
		if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 5, true)) {
			return -1;
		}

	} else {
		UBX_DEBUG("Setting fixed base position");

		const FixedPositionSettings &settings = _base_settings.settings.fixed_position;

		memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
		_buf.payload_tx_cfg_tmode3.flags = 2 /* fixed mode */ | (1 << 8) /* lat/lon mode */;
		int64_t lat64 = (int64_t)(settings.latitude * 1e9);
		_buf.payload_tx_cfg_tmode3.ecefXOrLat = (int32_t)(lat64 / 100);
		_buf.payload_tx_cfg_tmode3.ecefXOrLatHP = lat64 % 100; // range [-99, 99]
		int64_t lon64 = (int64_t)(settings.longitude * 1e9);
		_buf.payload_tx_cfg_tmode3.ecefYOrLon = (int32_t)(lon64 / 100);
		_buf.payload_tx_cfg_tmode3.ecefYOrLonHP = lon64 % 100;
		int64_t alt64 = (int64_t)((double)settings.altitude * 1e4);
		_buf.payload_tx_cfg_tmode3.ecefZOrAlt = (int32_t)(alt64 / 100); // cm
		_buf.payload_tx_cfg_tmode3.ecefZOrAltHP = alt64 % 100; // 0.1mm

		_buf.payload_tx_cfg_tmode3.fixedPosAcc = (uint32_t)(settings.position_accuracy * 10.f);

		if (!sendMessage(UBX_MSG_CFG_TMODE3, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_tmode3))) {
			return -1;
		}

		if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
			return -1;
		}

		// directly enable RTCM3 output
		return activateRTCMOutput(true);
	}

	return 0;
}

int GPSDriverUBX::restartSurveyIn()
{
	if (_output_mode != OutputMode::RTCM) {
		return -1;
	}

	if (!_proto_ver_27_or_higher) {
		return restartSurveyInPreV27();
	}

	UBX_DEBUG("restartSurveyIn");

	//disable RTCM output
	initCfgValset();
	cfgValsetPort(RTCM_BASE_MSGOUT_I2C, 0);
	sendCfgValsetAcked(false);

	if (_base_settings.type == BaseSettingsType::survey_in) {
		UBX_DEBUG("Starting Survey-in");

		// Reapplying survey-in mode does not restart an existing survey.
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 0 /* Disabled */);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		// Time-mode changes take effect on a navigation epoch, not on the ACK.
		_survey_in_stopped = false;
		const gps_abstime stop_deadline = gps_absolute_time() + 3000000;

		while (!_survey_in_stopped && gps_absolute_time() < stop_deadline) {
			if (!sendMessage(UBX_MSG_NAV_SVIN, nullptr, 0)) {
				return -1;
			}

			const gps_abstime poll_deadline = gps_absolute_time() + 100000;

			while (!_survey_in_stopped && gps_absolute_time() < poll_deadline) {
				bool read_error;
				receiveInternal(100, read_error);

				if (read_error) {
					return -1;
				}
			}
		}

		if (!_survey_in_stopped) {
			UBX_WARN("Survey-in did not stop");
			return -1;
		}

		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 1 /* Survey-in */);
		cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_SVIN_MIN_DUR, _base_settings.settings.survey_in.min_dur);
		cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_SVIN_ACC_LIMIT, _base_settings.settings.survey_in.acc_limit);
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 5);

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

	} else {
		UBX_DEBUG("Setting fixed base position");

		const FixedPositionSettings &settings = _base_settings.settings.fixed_position;
		initCfgValset();
		cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 2 /* Fixed Mode */);
		cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_POS_TYPE, 1 /* Lat/Lon/Height */);
		int64_t lat64 = (int64_t)(settings.latitude * 1e9);
		cfgValset<int32_t>(UBX_CFG_KEY_TMODE_LAT, (int32_t)(lat64 / 100));
		cfgValset<int8_t>(UBX_CFG_KEY_TMODE_LAT_HP, lat64 % 100 /* range [-99, 99] */);
		int64_t lon64 = (int64_t)(settings.longitude * 1e9);
		cfgValset<int32_t>(UBX_CFG_KEY_TMODE_LON, (int32_t)(lon64 / 100));
		cfgValset<int8_t>(UBX_CFG_KEY_TMODE_LON_HP, lon64 % 100 /* range [-99, 99] */);
		int64_t alt64 = (int64_t)((double)settings.altitude * 1e4);
		cfgValset<int32_t>(UBX_CFG_KEY_TMODE_HEIGHT, (int32_t)(alt64 / 100) /* cm */);
		cfgValset<int8_t>(UBX_CFG_KEY_TMODE_HEIGHT_HP, alt64 % 100 /* 0.1mm */);
		cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_FIXED_POS_ACC, (uint32_t)(settings.position_accuracy * 10.f));

		if (sendCfgValsetAcked() < 0) {
			return -1;
		}

		// directly enable RTCM3 output
		return activateRTCMOutput(true);

	}

	return 0;
}

int	// -1 = NAK, error or timeout, 0 = ACK
GPSDriverUBX::waitForAck(const uint16_t msg, const unsigned timeout, const bool report)
{
	int ret = -1;

	_ack_state = UBX_ACK_WAITING;
	_ack_waiting_msg = msg;	// memorize sent msg class&ID for ACK check

	gps_abstime time_started = gps_absolute_time();

	while ((_ack_state == UBX_ACK_WAITING) && (gps_absolute_time() < time_started + timeout * 1000)) {
		bool read_error;
		receiveInternal(timeout, read_error);
		if (read_error) {
			break;
		}
	}

	if (_ack_state == UBX_ACK_GOT_ACK) {
		ret = 0;	// ACK received ok

	} else if (report) {
		if (_ack_state == UBX_ACK_GOT_NAK) {
			UBX_DEBUG("ubx msg 0x%04x NAK", SWAP16((unsigned)msg));

		} else {
			UBX_DEBUG("ubx msg 0x%04x ACK timeout", SWAP16((unsigned)msg));
		}
	}

	_ack_state = UBX_ACK_IDLE;
	return ret;
}

void
GPSDriverUBX::waitForGnssReset()
{
	// Changing the enabled constellations resets the GNSS subsystem, and every u-blox
	// interface description asks for 0.5 s after the acknowledgement before the next
	// command. Keep reading while we wait, the receiver is still streaming.
	const gps_abstime time_started = gps_absolute_time();

	while (gps_absolute_time() < time_started + UBX_GNSS_RESET_TIME) {
		receive(UBX_CONFIG_TIMEOUT);
	}
}

int	// -1 = error, 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::receive(unsigned timeout)
{
	bool read_error;
	return receiveInternal(timeout, read_error);
}

int GPSDriverUBX::receiveInternal(unsigned timeout, bool &read_error)
{
	read_error = false;
	uint8_t buf[GPS_READ_BUFFER_SIZE];

	/* timeout additional to poll */
	gps_abstime time_started = gps_absolute_time();

	int handled = 0;

	while (true) {
		bool ready_to_return = _configured ? (_got_posllh && _got_velned) : handled;

		/* return success if ready */
		if (ready_to_return) {
			_got_posllh = false;
			_got_velned = false;
			return handled;
		}

		/* Wait for only UBX_PACKET_TIMEOUT if something already received. */
		int ret = read(buf, sizeof(buf), (_got_posllh || _got_velned) ? UBX_PACKET_TIMEOUT : timeout);

		if (ret < 0) {
			/* something went wrong when polling or reading */
			read_error = true;
			if (ret != ReadCancelled) {
				UBX_WARN("ubx poll_or_read err");
			}
			return -1;

		} else if (ret > 0) {
			//UBX_DEBUG("read %d bytes", ret);

			/* pass received bytes to the packet decoder */
			for (int i = 0; i < ret; i++) {
				handled |= parseChar(buf[i]);
				//UBX_DEBUG("parsed %d: 0x%x", i, buf[i]);
			}

			if (_interface == Interface::SPI) {
				if (buf[ret - 1] == 0xff) {
					if (ready_to_return) {
						_got_posllh = false;
						_got_velned = false;
						return handled;
					}
				}
			}
		}

		/* abort after timeout if no useful packets received */
		if (time_started + timeout * 1000 < gps_absolute_time()) {
			UBX_DEBUG("timed out, returning");
			return -1;
		}
	}
}

int	// 0 = decoding, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::parseChar(const uint8_t b)
{
	int ret = 0;

	if (_rtcm_parsing) {
		if (_rtcm_parsing->addByte(b)) {
			gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
			decodeInit();
			_rtcm_parsing->reset();
			return ret;
		}
	}

	switch (_decode_state) {

	/* Expecting Sync1 */
	case UBX_DECODE_SYNC1:
		if (b == UBX_SYNC1) {	// Sync1 found --> expecting Sync2
			UBX_TRACE_PARSER("A");
			_decode_state = UBX_DECODE_SYNC2;
		}

		break;

	/* Expecting Sync2 */
	case UBX_DECODE_SYNC2:
		if (b == UBX_SYNC2) {	// Sync2 found --> expecting Class
			UBX_TRACE_PARSER("B");
			_decode_state = UBX_DECODE_CLASS;

		} else {		// Sync1 not followed by Sync2: reset parser
			decodeInit();
		}

		break;

	/* Expecting Class */
	case UBX_DECODE_CLASS:
		UBX_TRACE_PARSER("C");
		addByteToChecksum(b);  // checksum is calculated for everything except Sync and Checksum bytes
		_rx_msg = b;
		_decode_state = UBX_DECODE_ID;
		break;

	/* Expecting ID */
	case UBX_DECODE_ID:
		UBX_TRACE_PARSER("D");
		addByteToChecksum(b);
		_rx_msg |= b << 8;
		_decode_state = UBX_DECODE_LENGTH1;
		break;

	/* Expecting first length byte */
	case UBX_DECODE_LENGTH1:
		UBX_TRACE_PARSER("E");
		addByteToChecksum(b);
		_rx_payload_length = b;
		_decode_state = UBX_DECODE_LENGTH2;
		break;

	/* Expecting second length byte */
	case UBX_DECODE_LENGTH2:
		UBX_TRACE_PARSER("F");
		addByteToChecksum(b);
		_rx_payload_length |= b << 8;	// calculate payload size

		if (payloadRxInit() != 0) {	// start payload reception
			// payload will not be handled, discard message
			decodeInit();

		} else {
			_decode_state = (_rx_payload_length > 0) ? UBX_DECODE_PAYLOAD : UBX_DECODE_CHKSUM1;
		}

		break;

	/* Expecting payload */
	case UBX_DECODE_PAYLOAD:
		UBX_TRACE_PARSER(".");
		addByteToChecksum(b);

		switch (_rx_msg) {
		case UBX_MSG_NAV_SAT:
			ret = payloadRxAddNavSat(b);	// add a NAV-SAT payload byte
			break;

		case UBX_MSG_NAV_SVINFO:
			ret = payloadRxAddNavSvinfo(b);	// add a NAV-SVINFO payload byte
			break;

		case UBX_MSG_MON_VER:
			ret = payloadRxAddMonVer(b);	// add a MON-VER payload byte
			break;

		default:
			ret = payloadRxAdd(b);		// add a payload byte
			break;
		}

		if (ret < 0) {
			// payload not handled, discard message
			decodeInit();

		} else if (ret > 0) {
			// payload complete, expecting checksum
			_decode_state = UBX_DECODE_CHKSUM1;

		} else {
			// expecting more payload, stay in state UBX_DECODE_PAYLOAD
		}

		ret = 0;
		break;

	/* Expecting first checksum byte */
	case UBX_DECODE_CHKSUM1:
		if (_rx_ck_a != b) {
			UBX_DEBUG("ubx checksum err");
			decodeInit();

		} else {
			_decode_state = UBX_DECODE_CHKSUM2;
		}

		break;

	/* Expecting second checksum byte */
	case UBX_DECODE_CHKSUM2:
		if (_rx_ck_b != b) {
			UBX_DEBUG("ubx checksum err");

		} else {
			ret = payloadRxDone();	// finish payload processing

			if (_rtcm_parsing) {
				_rtcm_parsing->reset();
			}
		}

		decodeInit();
		break;

	default:
		break;
	}

	return ret;
}

/**
 * Start payload rx
 */
int	// -1 = abort, 0 = continue
GPSDriverUBX::payloadRxInit()
{
	int ret = 0;

	_rx_state = UBX_RXMSG_HANDLE;	// handle by default

	switch (_rx_msg) {
	case UBX_MSG_MON_COMMS:
		if (_rx_payload_length < 8 || _rx_payload_length > sizeof(ubx_payload_rx_mon_comms_t)
		    || (_rx_payload_length - 8) % sizeof(ubx_payload_rx_mon_comms_port_t) != 0) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;
		}

		break;

	case UBX_MSG_NAV_PVT:
		if ((_rx_payload_length != UBX_PAYLOAD_RX_NAV_PVT_SIZE_UBX7)		/* u-blox 7 msg format */
		    && (_rx_payload_length != UBX_PAYLOAD_RX_NAV_PVT_SIZE_UBX8)) {	/* u-blox 8+ msg format */
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (!_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if not using NAV-PVT
		}

		break;

	case UBX_MSG_INF_DEBUG:
	case UBX_MSG_INF_ERROR:
	case UBX_MSG_INF_NOTICE:
	case UBX_MSG_INF_WARNING:
		if (_rx_payload_length >= sizeof(ubx_buf_t)) {
			_rx_payload_length = sizeof(ubx_buf_t) - 1; //avoid buffer overflow
		}

		break;

	case UBX_MSG_NAV_POSLLH:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_posllh_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_NAV_SOL:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_sol_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_NAV_STATUS:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_status_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_DOP:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_dop_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_RELPOSNED:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_relposned_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_DAHEADING:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_daheading_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_HPPOSLLH:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_hpposllh_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_TIMEUTC:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_timeutc_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_NAV_SAT:
	case UBX_MSG_NAV_SVINFO:
		if (_satellite_info == nullptr) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if sat info not requested

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else {
			memset(_satellite_info, 0, sizeof(*_satellite_info));        // initialize sat info
		}

		break;

	case UBX_MSG_NAV_SVIN:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_svin_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;
		}

		break;

	case UBX_MSG_NAV_VELNED:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_velned_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_MON_VER:
		break;		// unconditionally handle this message

	case UBX_MSG_MON_HW:
		if ((_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_ubx6_t))	/* u-blox 6 msg format */
		    && (_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_ubx7_t))	/* u-blox 7+ msg format */
		    && (_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_deprecated_t))) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured
		}

		break;

	case UBX_MSG_MON_RF:
		if (_rx_payload_length < sizeof(ubx_payload_rx_mon_rf_t) ||
		    (_rx_payload_length - 4) % sizeof(ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t) != 0) {

			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured
		}

		break;

	case UBX_MSG_SEC_SIG:
		if (_rx_payload_length < 4) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;
		}

		break;

	case UBX_MSG_RXM_RTCM:
		if (_rx_payload_length != sizeof(ubx_payload_rx_rxm_rtcm_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured
		}

		break;

	case UBX_MSG_RXM_COR:
		if (_rx_payload_length != sizeof(ubx_payload_rx_rxm_cor_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;
		}

		break;

	case UBX_MSG_ACK_ACK:
		if (_rx_payload_length != sizeof(ubx_payload_rx_ack_ack_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if _configured
		}

		break;

	case UBX_MSG_ACK_NAK:
		if (_rx_payload_length != sizeof(ubx_payload_rx_ack_nak_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if _configured
		}

		break;

	default:
		_rx_state = UBX_RXMSG_DISABLE;	// disable all other messages
		break;
	}

	switch (_rx_state) {
	case UBX_RXMSG_HANDLE:	// handle message
	case UBX_RXMSG_IGNORE:	// ignore message but don't report error
		ret = 0;
		break;

	case UBX_RXMSG_DISABLE:	// disable unexpected messages
		UBX_DEBUG("ubx msg 0x%04x len %u unexpected", SWAP16((unsigned)_rx_msg), (unsigned)_rx_payload_length);

		// TODO: UBX-MON-HW2
		// [uavcan:52:gps] ubx msg 0x0a0b len 28 unexpected

		if (_proto_ver_27_or_higher) {
			uint32_t key_id = 0;

			switch (_rx_msg) { // we cannot infer the config Key ID from _rx_msg for protocol version 27+
			case UBX_MSG_RXM_RAWX:
				key_id = UBX_CFG_KEY_MSGOUT_UBX_RXM_RAWX_I2C;
				break;

			case UBX_MSG_RXM_SFRBX:
				key_id = UBX_CFG_KEY_MSGOUT_UBX_RXM_SFRBX_I2C;
				break;

			case UBX_MSG_NAV_TIMEGPS:
				key_id = UBX_CFG_KEY_MSGOUT_UBX_NAV_TIMEGPS_I2C;
				break;
			}

			if (key_id != 0) {
				gps_abstime t = gps_absolute_time();

				if (t > _disable_cmd_last + DISABLE_MSG_INTERVAL && _configured) {
					/* don't attempt for every message to disable, some might not be disabled */
					_disable_cmd_last = t;
					UBX_DEBUG("ubx disabling msg 0x%04x (0x%04x)", SWAP16((unsigned)_rx_msg), (uint16_t)key_id);

					initCfgValset();
					cfgValsetPort(key_id, 0);
					sendCfgValset();
				}
			}

		} else {
			gps_abstime t = gps_absolute_time();

			if (t > _disable_cmd_last + DISABLE_MSG_INTERVAL) {
				/* don't attempt for every message to disable, some might not be disabled */
				_disable_cmd_last = t;
				UBX_DEBUG("ubx disabling msg 0x%04x", SWAP16((unsigned)_rx_msg));

				configureMessageRate(_rx_msg, 0);
			}
		}

		ret = -1;	// return error, abort handling this message
		break;

	case UBX_RXMSG_ERROR_LENGTH:	// error: invalid length
		UBX_DEBUG("ubx msg 0x%04x invalid len %u", SWAP16((unsigned)_rx_msg), (unsigned)_rx_payload_length);
		ret = -1;	// return error, abort handling this message
		break;

	default:	// invalid message state
		UBX_WARN("ubx internal err1");
		ret = -1;	// return error, abort handling this message
		break;
	}

	return ret;
}

/**
 * Add payload rx byte
 */
int	// -1 = error, 0 = ok, 1 = payload completed
GPSDriverUBX::payloadRxAdd(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;

	if (_rx_payload_index < sizeof(_buf)) {
		p_buf[_rx_payload_index] = b;
	}

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

int	// -1 = error, 0 = ok, 1 = payload completed
GPSDriverUBX::payloadRxAddNavSat(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;

	if (_rx_payload_index < sizeof(ubx_payload_rx_nav_sat_part1_t)) {
		// Fill Part 1 buffer
		p_buf[_rx_payload_index] = b;

	} else {
		if (_rx_payload_index == sizeof(ubx_payload_rx_nav_sat_part1_t)) {
			// Part 1 complete: decode Part 1 buffer
			_satellite_info->count = MIN(_buf.payload_rx_nav_sat_part1.numSvs, satellite_info_s::SAT_INFO_MAX_SATELLITES);
			UBX_TRACE_SVINFO("SAT len %u  numCh %u", (unsigned)_rx_payload_length,
					 (unsigned)_buf.payload_rx_nav_sat_part1.numSvs);
		}

		if (_rx_payload_index < sizeof(ubx_payload_rx_nav_sat_part1_t) + _satellite_info->count * sizeof(
			    ubx_payload_rx_nav_sat_part2_t)) {
			// Still room in _satellite_info: fill Part 2 buffer
			unsigned buf_index = (_rx_payload_index - sizeof(ubx_payload_rx_nav_sat_part1_t)) % sizeof(
						     ubx_payload_rx_nav_sat_part2_t);
			p_buf[buf_index] = b;

			if (buf_index == sizeof(ubx_payload_rx_nav_sat_part2_t) - 1) {
				// Part 2 complete: decode Part 2 buffer
				unsigned sat_index = (_rx_payload_index - sizeof(ubx_payload_rx_nav_sat_part1_t)) /
						     sizeof(ubx_payload_rx_nav_sat_part2_t);

				// convert gnssId:svId to a 8 bit number (use svId numbering from NAV-SVINFO)
				uint8_t ubx_sat_gnssId = static_cast<uint8_t>(_buf.payload_rx_nav_sat_part2.gnssId);
				uint8_t ubx_sat_svId = static_cast<uint8_t>(_buf.payload_rx_nav_sat_part2.svId);

				uint8_t svinfo_svid = 255;

				switch (ubx_sat_gnssId) {
				case 0:  // GPS: G1-G23 -> 1-32
					if (ubx_sat_svId >= 1 && ubx_sat_svId <= 32) {
						svinfo_svid = ubx_sat_svId;
					}

					break;

				case 1:  // SBAS: S120-S158 -> 120-158
					if (ubx_sat_svId >= 120 && ubx_sat_svId <= 158) {
						svinfo_svid = ubx_sat_svId;
					}

					break;

				case 2:  // Galileo: E1-E36 -> 211-246
					if (ubx_sat_svId >= 1 && ubx_sat_svId <= 36) {
						svinfo_svid = ubx_sat_svId + 210;
					}

					break;

				case 3:  // BeiDou: B1-B37 -> 159-163,33-64
					if (ubx_sat_svId >= 1 && ubx_sat_svId <= 4) {
						svinfo_svid = ubx_sat_svId + 158;

					} else if (ubx_sat_svId >= 5 && ubx_sat_svId <= 37) {
						svinfo_svid = ubx_sat_svId + 28;
					}

					break;

				case 4:  // IMES: I1-I10 -> 173-182
					if (ubx_sat_svId >= 1 && ubx_sat_svId <= 10) {
						svinfo_svid = ubx_sat_svId + 172;
					}

					break;

				case 5:  // QZSS: Q1-A10 -> 193-202
					if (ubx_sat_svId >= 1 && ubx_sat_svId <= 10) {
						svinfo_svid = ubx_sat_svId + 192;
					}

					break;

				case 6:  // GLONASS: R1-R32 -> 65-96, R? -> 255
					if (ubx_sat_svId >= 1 && ubx_sat_svId <= 32) {
						svinfo_svid = ubx_sat_svId + 64;
					}

					break;
				}

				_satellite_info->svid[sat_index]	  = svinfo_svid;
				// NAV-SAT flags: bits 2..0 qualityInd, bit 3 svUsed
				_satellite_info->used[sat_index]	  = static_cast<uint8_t>((_buf.payload_rx_nav_sat_part2.flags >> 3) & 0x01);
				// TODO: elev and azim are signed on the wire but unsigned in satellite_info, so
				// negatives wrap. Needs SatelliteInfo.msg to carry signed angles and an unknown
				// marker; its elevation comment is inverted too.
				_satellite_info->elevation[sat_index] = static_cast<uint8_t>(_buf.payload_rx_nav_sat_part2.elev);
				_satellite_info->azimuth[sat_index]	  = static_cast<uint8_t>(static_cast<float>(_buf.payload_rx_nav_sat_part2.azim) *
						255.0f / 360.0f);
				_satellite_info->snr[sat_index]		  = static_cast<uint8_t>(_buf.payload_rx_nav_sat_part2.cno);
				_satellite_info->prn[sat_index]		  = svinfo_svid;
				UBX_TRACE_SVINFO("SAT #%02u  svid %3u  used %u  elevation %3u  azimuth %3u  snr %3u  prn %3u",
						 static_cast<unsigned>(sat_index + 1),
						 static_cast<unsigned>(_satellite_info->svid[sat_index]),
						 static_cast<unsigned>(_satellite_info->used[sat_index]),
						 static_cast<unsigned>(_satellite_info->elevation[sat_index]),
						 static_cast<unsigned>(_satellite_info->azimuth[sat_index]),
						 static_cast<unsigned>(_satellite_info->snr[sat_index]),
						 static_cast<unsigned>(_satellite_info->prn[sat_index])
						);
			}
		}
	}

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

/**
 * Add NAV-SVINFO payload rx byte
 */
int	// -1 = error, 0 = ok, 1 = payload completed
GPSDriverUBX::payloadRxAddNavSvinfo(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;

	if (_rx_payload_index < sizeof(ubx_payload_rx_nav_svinfo_part1_t)) {
		// Fill Part 1 buffer
		p_buf[_rx_payload_index] = b;

	} else {
		if (_rx_payload_index == sizeof(ubx_payload_rx_nav_svinfo_part1_t)) {
			// Part 1 complete: decode Part 1 buffer
			_satellite_info->count = MIN(_buf.payload_rx_nav_svinfo_part1.numCh, satellite_info_s::SAT_INFO_MAX_SATELLITES);
			UBX_TRACE_SVINFO("SVINFO len %u  numCh %u", (unsigned)_rx_payload_length,
					 (unsigned)_buf.payload_rx_nav_svinfo_part1.numCh);
		}

		if (_rx_payload_index < sizeof(ubx_payload_rx_nav_svinfo_part1_t) + _satellite_info->count * sizeof(
			    ubx_payload_rx_nav_svinfo_part2_t)) {
			// Still room in _satellite_info: fill Part 2 buffer
			unsigned buf_index = (_rx_payload_index - sizeof(ubx_payload_rx_nav_svinfo_part1_t)) % sizeof(
						     ubx_payload_rx_nav_svinfo_part2_t);
			p_buf[buf_index] = b;

			if (buf_index == sizeof(ubx_payload_rx_nav_svinfo_part2_t) - 1) {
				// Part 2 complete: decode Part 2 buffer
				unsigned sat_index = (_rx_payload_index - sizeof(ubx_payload_rx_nav_svinfo_part1_t)) /
						     sizeof(ubx_payload_rx_nav_svinfo_part2_t);
				_satellite_info->svid[sat_index]      = static_cast<uint8_t>(_buf.payload_rx_nav_svinfo_part2.svid);
				// NAV-SVINFO flags: bit 0 svUsed
				_satellite_info->used[sat_index]      = static_cast<uint8_t>(_buf.payload_rx_nav_svinfo_part2.flags & 0x01);
				// TODO: same elev/azim wrap as NAV-SAT above
				_satellite_info->elevation[sat_index] = static_cast<uint8_t>(_buf.payload_rx_nav_svinfo_part2.elev);
				_satellite_info->azimuth[sat_index]   = static_cast<uint8_t>(static_cast<float>(_buf.payload_rx_nav_svinfo_part2.azim) *
									255.0f / 360.0f);
				_satellite_info->snr[sat_index]       = static_cast<uint8_t>(_buf.payload_rx_nav_svinfo_part2.cno);
				_satellite_info->prn[sat_index]       = static_cast<uint8_t>(_buf.payload_rx_nav_svinfo_part2.svid);

				UBX_TRACE_SVINFO("SVINFO #%02u  svid %3u  used %u  elevation %3u  azimuth %3u  snr %3u  prn %3u",
						 static_cast<unsigned>(sat_index + 1),
						 static_cast<unsigned>(_satellite_info->svid[sat_index]),
						 static_cast<unsigned>(_satellite_info->used[sat_index]),
						 static_cast<unsigned>(_satellite_info->elevation[sat_index]),
						 static_cast<unsigned>(_satellite_info->azimuth[sat_index]),
						 static_cast<unsigned>(_satellite_info->snr[sat_index]),
						 static_cast<unsigned>(_satellite_info->prn[sat_index])
						);
			}
		}
	}

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

/**
 * Add MON-VER payload rx byte
 */
int	// -1 = error, 0 = ok, 1 = payload completed
GPSDriverUBX::payloadRxAddMonVer(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;
	if (_rx_payload_index == 0) {
		_model_name[0] = '\0';
		_firmware_version[0] = '\0';
	}

	if (_rx_payload_index < sizeof(ubx_payload_rx_mon_ver_part1_t)) {
		// Fill Part 1 buffer
		p_buf[_rx_payload_index] = b;

	} else {
		if (_rx_payload_index == sizeof(ubx_payload_rx_mon_ver_part1_t)) {
			// Part 1 complete: decode Part 1 buffer and calculate hash for SW&HW version strings
			// The protocol specifies these as nul-terminated strings, but the terminator comes
			// from the device, so enforce it before anything walks the field.
			_buf.payload_rx_mon_ver_part1.swVersion[sizeof(_buf.payload_rx_mon_ver_part1.swVersion) - 1] = 0;
			_buf.payload_rx_mon_ver_part1.hwVersion[sizeof(_buf.payload_rx_mon_ver_part1.hwVersion) - 1] = 0;
			memcpy(_firmware_version, _buf.payload_rx_mon_ver_part1.swVersion, sizeof(_firmware_version));

			_ubx_version = fnv1_32_str(_buf.payload_rx_mon_ver_part1.swVersion, FNV1_32_INIT);
			_ubx_version = fnv1_32_str(_buf.payload_rx_mon_ver_part1.hwVersion, _ubx_version);
			UBX_DEBUG("VER hash 0x%08x", (uint16_t)_ubx_version);
			UBX_DEBUG("VER hw  \"%10s\"", _buf.payload_rx_mon_ver_part1.hwVersion);
			UBX_DEBUG("VER sw  \"%30s\"", _buf.payload_rx_mon_ver_part1.swVersion);

			// Device detection (See https://forum.u-blox.com/index.php/9432/need-help-decoding-ubx-mon-ver-hardware-string)
			static constexpr struct {
				char hw_version[9];
				Board board;
			} known_boards[] = {
				{"00040005", Board::u_blox5},
				{"00040007", Board::u_blox6},
				{"00070000", Board::u_blox7},
				{"00080000", Board::u_blox8},
				{"00190000", Board::u_blox9},
				{"000A0000", Board::u_blox10},
				{"000B0000", Board::u_blox_X20},
			};
			bool known = false;

			for (const auto &known_board : known_boards) {
				if (strncmp((const char *)_buf.payload_rx_mon_ver_part1.hwVersion, known_board.hw_version,
					    sizeof(_buf.payload_rx_mon_ver_part1.hwVersion)) == 0) {
					_board = known_board.board;
					known = true;
					break;
				}
			}

			if (!known) {
				UBX_WARN("unknown board hw: %s", _buf.payload_rx_mon_ver_part1.hwVersion);
			}

			UBX_DEBUG("detected board: %i", static_cast<int>(_board));
		}

		// fill Part 2 buffer
		unsigned buf_index = (_rx_payload_index - sizeof(ubx_payload_rx_mon_ver_part1_t)) % sizeof(
					     ubx_payload_rx_mon_ver_part2_t);
		p_buf[buf_index] = b;

		if (buf_index == sizeof(ubx_payload_rx_mon_ver_part2_t) - 1) {
			// Part 2 complete: decode Part 2 buffer
			// Same as above: the protocol specifies a nul-terminated string, the device provides
			// the terminator, so enforce it before strstr() walks the field.
			_buf.payload_rx_mon_ver_part2.extension[sizeof(_buf.payload_rx_mon_ver_part2.extension) - 1] = 0;

			UBX_DEBUG("VER ext \" %30s\"", _buf.payload_rx_mon_ver_part2.extension);

			// "FWVER=" Firmware of product category and version
			const char *fwver_str = strstr((const char *)_buf.payload_rx_mon_ver_part2.extension, "FWVER=");

			if (fwver_str != nullptr) {
				strncpy(_firmware_version, fwver_str + strlen("FWVER="), sizeof(_firmware_version) - 1);
				_firmware_version[sizeof(_firmware_version) - 1] = '\0';
				GPS_INFO("u-blox firmware version: %s", fwver_str + strlen("FWVER="));

				// Check if its a ZED-F9P-15B
				if ((_board == Board::u_blox9) && strstr(fwver_str, "HPGL1L5")) {
					_board = Board::u_blox9_F9P_L1L5;
					UBX_DEBUG("F9P-15B detected");
				}
			}

			// "PROTVER=" Supported protocol version.
			const char *protver_str = strstr((const char *)_buf.payload_rx_mon_ver_part2.extension, "PROTVER=");

			if (protver_str != nullptr) {
				GPS_INFO("u-blox protocol version: %s", protver_str + strlen("PROTVER="));
			}

			// "MOD=" Module identification. Set in production.
			const char *mod_str = strstr((const char *)_buf.payload_rx_mon_ver_part2.extension, "MOD=");

			if (mod_str != nullptr) {
				strncpy(_model_name, mod_str + strlen("MOD="), sizeof(_model_name) - 1);
				_model_name[sizeof(_model_name) - 1] = '\0';
				_is_m8p = strstr(mod_str, "M8P") != nullptr;
				// in case of u-blox9 family, check if it's an F9P
				if (_board == Board::u_blox9) {
					if (strstr(mod_str, "F9P")) {
						_board = Board::u_blox9_F9P_L1L2;
						UBX_DEBUG("F9P detected");
					}

				} else if (_board == Board::u_blox10) {
					if (strstr(mod_str, "DAN-F10N")) {
						_board = Board::u_blox10_L1L5;
						UBX_DEBUG("DAN-F10N detected");
					}
				}

				GPS_INFO("u-blox module: %s", mod_str + strlen("MOD="));
			}
		}
	}

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

/**
 * Finish payload rx
 */
int	// 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::payloadRxDone()
{
	int ret = 0;

	// return if no message handled
	if (_rx_state != UBX_RXMSG_HANDLE) {
		return ret;
	}

	// handle message
	switch (_rx_msg) {

	case UBX_MSG_NAV_PVT:
		UBX_TRACE_RXMSG("Rx NAV-PVT");

		//Check if position fix flag is good
		if ((_buf.payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK) == 1) {
			_gps_position->fix_type		 = _buf.payload_rx_nav_pvt.fixType;

			if (_buf.payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_DIFFSOLN) {
				_gps_position->fix_type = 4; //DGPS
			}

			uint8_t carr_soln = _buf.payload_rx_nav_pvt.flags >> 6;

			if (carr_soln == 1) {
				_gps_position->fix_type = 5; //Float RTK

			} else if (carr_soln == 2) {
				_gps_position->fix_type = 6; //Fixed RTK
			}

			_gps_position->vel_ned_valid = true;

		} else {
			_gps_position->fix_type		 = 0;
			_gps_position->vel_ned_valid = false;
		}

		_gps_position->satellites_used	= _buf.payload_rx_nav_pvt.numSV;

		if (_gps_position->fix_type < 6) {
			// When RTK is active and solid (fix=6), these values will be filled by HPPOSLLH:
			_gps_position->latitude_deg		= _buf.payload_rx_nav_pvt.lat * 1e-7;
			_gps_position->longitude_deg		= _buf.payload_rx_nav_pvt.lon * 1e-7;
			_gps_position->altitude_msl_m		= _buf.payload_rx_nav_pvt.hMSL * 1e-3;
			_gps_position->altitude_ellipsoid_m	= _buf.payload_rx_nav_pvt.height * 1e-3;

			_gps_position->eph		= static_cast<float>(_buf.payload_rx_nav_pvt.hAcc) * 1e-3f;
			_gps_position->epv		= static_cast<float>(_buf.payload_rx_nav_pvt.vAcc) * 1e-3f;

			_rate_count_lat_lon++;
			_got_posllh = true;
		}

		_gps_position->s_variance_m_s	= static_cast<float>(_buf.payload_rx_nav_pvt.sAcc) * 1e-3f;

		_gps_position->vel_m_s		= static_cast<float>(_buf.payload_rx_nav_pvt.gSpeed) * 1e-3f;

		_gps_position->vel_n_m_s	= static_cast<float>(_buf.payload_rx_nav_pvt.velN) * 1e-3f;
		_gps_position->vel_e_m_s	= static_cast<float>(_buf.payload_rx_nav_pvt.velE) * 1e-3f;
		_gps_position->vel_d_m_s	= static_cast<float>(_buf.payload_rx_nav_pvt.velD) * 1e-3f;

		_gps_position->cog_rad		= static_cast<float>(_buf.payload_rx_nav_pvt.headMot) * M_DEG_TO_RAD_F * 1e-5f;
		_gps_position->c_variance_rad	= static_cast<float>(_buf.payload_rx_nav_pvt.headAcc) * M_DEG_TO_RAD_F * 1e-5f;

		//Check if time and date fix flags are good
		if ((_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDDATE)
		    && (_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDTIME)
		    && (_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_FULLYRESOLVED)) {
			tm timeinfo{};
			timeinfo.tm_year	= _buf.payload_rx_nav_pvt.year - 1900;
			timeinfo.tm_mon		= _buf.payload_rx_nav_pvt.month - 1;
			timeinfo.tm_mday	= _buf.payload_rx_nav_pvt.day;
			timeinfo.tm_hour	= _buf.payload_rx_nav_pvt.hour;
			timeinfo.tm_min		= _buf.payload_rx_nav_pvt.min;
			timeinfo.tm_sec		= _buf.payload_rx_nav_pvt.sec;
			_gps_position->time_utc_usec = timeFromUtc(timeinfo, _buf.payload_rx_nav_pvt.nano);

		} else {
			// The struct is reused across messages, so without this a receiver
			// that lost time in a reset keeps reporting the last time it knew.
			// 0 is the defined "unavailable" value.
			_gps_position->time_utc_usec = 0;
		}

		_gps_position->timestamp = gps_absolute_time();
		_last_timestamp_time = _gps_position->timestamp;

		_rate_count_vel++;
		_got_velned = true;

		ret = 1;
		break;

	case UBX_MSG_INF_DEBUG:
	case UBX_MSG_INF_NOTICE: {
			uint8_t *p_buf = (uint8_t *)&_buf;
			p_buf[_rx_payload_length] = 0;
			UBX_DEBUG("ubx msg: %s", p_buf);
		}
		break;

	case UBX_MSG_INF_ERROR:
	case UBX_MSG_INF_WARNING: {
			uint8_t *p_buf = (uint8_t *)&_buf;
			p_buf[_rx_payload_length] = 0;
			UBX_WARN("ubx msg: %s", p_buf);

			if (strncmp(reinterpret_cast<const char *>(p_buf), "txbuf", 5) == 0) {
				requestCommsDiagnostics();
			}
		}
		break;

	case UBX_MSG_MON_COMMS:
		logCommsDiagnostics();
		break;

	case UBX_MSG_NAV_POSLLH:
		UBX_TRACE_RXMSG("Rx NAV-POSLLH");

		_gps_position->latitude_deg	= _buf.payload_rx_nav_posllh.lat * 1e-7;
		_gps_position->longitude_deg	= _buf.payload_rx_nav_posllh.lon * 1e-7;
		_gps_position->altitude_msl_m	= _buf.payload_rx_nav_posllh.hMSL * 1e-3;
		_gps_position->altitude_ellipsoid_m = _buf.payload_rx_nav_posllh.height * 1e-3;
		_gps_position->eph	= static_cast<float>(_buf.payload_rx_nav_posllh.hAcc) * 1e-3f; // from mm to m
		_gps_position->epv	= static_cast<float>(_buf.payload_rx_nav_posllh.vAcc) * 1e-3f; // from mm to m

		_gps_position->timestamp = gps_absolute_time();

		_rate_count_lat_lon++;
		_got_posllh = true;

		ret = 1;
		break;

	case UBX_MSG_NAV_HPPOSLLH:
		UBX_TRACE_RXMSG("Rx NAV-HPPOSLLH");

		if (_buf.payload_rx_nav_hpposllh.flags == 0 && _gps_position->fix_type == 6) {
			_gps_position->latitude_deg	= _buf.payload_rx_nav_hpposllh.lat * 1e-7 + _buf.payload_rx_nav_hpposllh.latHp *
							  1e-9;  // regular precision lat/lon (1e7), plus high precision (1e9)
			_gps_position->longitude_deg	= _buf.payload_rx_nav_hpposllh.lon * 1e-7 + _buf.payload_rx_nav_hpposllh.lonHp * 1e-9;
			_gps_position->altitude_msl_m = _buf.payload_rx_nav_hpposllh.hMSL * 1e-3 + _buf.payload_rx_nav_hpposllh.hMSLHp *
							1e-4;	// regular precision altitude, mm, plus high precision components of altitude, 0.1 mm
			_gps_position->altitude_ellipsoid_m = _buf.payload_rx_nav_hpposllh.height * 1e-3 + _buf.payload_rx_nav_hpposllh.heightHp
							      * 1e-4;
			_gps_position->eph	= static_cast<float>(_buf.payload_rx_nav_hpposllh.hAcc) *
						  1e-4f; // Accuracy estimates, convert from 0.1 mm to m
			_gps_position->epv	= static_cast<float>(_buf.payload_rx_nav_hpposllh.vAcc) * 1e-4f;

			_gps_position->timestamp = gps_absolute_time();

			_rate_count_lat_lon++;
			_got_posllh = true;

			ret = 1;
		}

		break;

	case UBX_MSG_NAV_SOL:
		UBX_TRACE_RXMSG("Rx NAV-SOL");

		_gps_position->fix_type		= _buf.payload_rx_nav_sol.gpsFix;
		_gps_position->s_variance_m_s	= static_cast<float>(_buf.payload_rx_nav_sol.sAcc) * 1e-2f;	// from cm to m
		_gps_position->satellites_used	= _buf.payload_rx_nav_sol.numSV;

		ret = 1;
		break;

	case UBX_MSG_NAV_STATUS:
		UBX_TRACE_RXMSG("Rx NAV-STATUS");

		_gps_position->spoofing_state = (_buf.payload_rx_nav_status.flags2 & UBX_RX_NAV_STATUS_SPOOFDETSTATE_MASK) >>
						UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT;

		ret = 1;
		break;

	case UBX_MSG_NAV_DOP:
		UBX_TRACE_RXMSG("Rx NAV-DOP");

		_gps_position->hdop		= _buf.payload_rx_nav_dop.hDOP * 0.01f;	// from cm to m
		_gps_position->vdop		= _buf.payload_rx_nav_dop.vDOP * 0.01f;	// from cm to m

		ret = 1;
		break;

	case UBX_MSG_NAV_TIMEUTC:
		UBX_TRACE_RXMSG("Rx NAV-TIMEUTC");

		if (_buf.payload_rx_nav_timeutc.valid & UBX_RX_NAV_TIMEUTC_VALID_VALIDUTC) {
			tm timeinfo {};
			timeinfo.tm_year	= _buf.payload_rx_nav_timeutc.year - 1900;
			timeinfo.tm_mon		= _buf.payload_rx_nav_timeutc.month - 1;
			timeinfo.tm_mday	= _buf.payload_rx_nav_timeutc.day;
			timeinfo.tm_hour	= _buf.payload_rx_nav_timeutc.hour;
			timeinfo.tm_min		= _buf.payload_rx_nav_timeutc.min;
			timeinfo.tm_sec		= _buf.payload_rx_nav_timeutc.sec;
			_gps_position->time_utc_usec = timeFromUtc(timeinfo, _buf.payload_rx_nav_timeutc.nano);

		} else {
			// The struct is reused across messages, so without this a receiver
			// that lost time in a reset keeps reporting the last time it knew.
			// 0 is the defined "unavailable" value.
			_gps_position->time_utc_usec = 0;
		}

		_last_timestamp_time = gps_absolute_time();

		ret = 1;
		break;

	case UBX_MSG_NAV_SAT:
	case UBX_MSG_NAV_SVINFO:
		UBX_TRACE_RXMSG("Rx NAV-SVINFO");

		// _satellite_info already populated by payload_rx_add_svinfo(), just add a timestamp
		_satellite_info->timestamp = gps_absolute_time();

		ret = 2;
		break;

	case UBX_MSG_NAV_SVIN:
		UBX_TRACE_RXMSG("Rx NAV-SVIN");
		{
			ubx_payload_rx_nav_svin_t &svin = _buf.payload_rx_nav_svin;
			_survey_in_stopped = svin.active == 0 && svin.valid == 0;

			if (!_configured) {
				ret = 1;
				break;
			}

			UBX_DEBUG("Survey-in status: %lus cur accuracy: %lumm nr obs: %lu valid: %i active: %i",
				  svin.dur, svin.meanAcc / 10, svin.obs, static_cast<int>(svin.valid), static_cast<int>(svin.active));

			SurveyInStatus status{};
			double ecef_x = (static_cast<double>(svin.meanX) + static_cast<double>(svin.meanXHP) * 0.01) * 0.01;
			double ecef_y = (static_cast<double>(svin.meanY) + static_cast<double>(svin.meanYHP) * 0.01) * 0.01;
			double ecef_z = (static_cast<double>(svin.meanZ) + static_cast<double>(svin.meanZHP) * 0.01) * 0.01;
			ECEF2lla(ecef_x, ecef_y, ecef_z, status.latitude, status.longitude, status.altitude);
			status.duration = svin.dur;
			status.mean_accuracy = svin.meanAcc / 10;
			status.flags = (svin.valid & 1) | ((svin.active & 1) << 1);
			surveyInStatus(status);

			if (svin.valid == 1 && svin.active == 0) {
				if (activateRTCMOutput(true) != 0) {
					return 0;
				}
			}
		}

		ret = 1;
		break;

	case UBX_MSG_NAV_VELNED:
		UBX_TRACE_RXMSG("Rx NAV-VELNED");

		_gps_position->vel_m_s        = static_cast<float>(_buf.payload_rx_nav_velned.gSpeed) * 1e-2f;
		_gps_position->vel_n_m_s      = static_cast<float>(_buf.payload_rx_nav_velned.velN)  * 1e-2f; // NED NORTH velocity
		_gps_position->vel_e_m_s      = static_cast<float>(_buf.payload_rx_nav_velned.velE)  * 1e-2f; // NED EAST velocity
		_gps_position->vel_d_m_s      = static_cast<float>(_buf.payload_rx_nav_velned.velD)  * 1e-2f; // NED DOWN velocity
		_gps_position->cog_rad        = static_cast<float>(_buf.payload_rx_nav_velned.heading) * M_DEG_TO_RAD_F * 1e-5f;
		_gps_position->c_variance_rad = static_cast<float>(_buf.payload_rx_nav_velned.cAcc)    * M_DEG_TO_RAD_F * 1e-5f;
		_gps_position->vel_ned_valid  = true;

		_rate_count_vel++;
		_got_velned = true;

		ret = 1;
		break;

	case UBX_MSG_NAV_RELPOSNED:
		UBX_TRACE_RXMSG("Rx NAV-RELPOSNED");
		{
			const float rel_length_cm = _buf.payload_rx_nav_relposned.relPosLength + _buf.payload_rx_nav_relposned.relPosHPLength * 1e-2f;
			const float rel_length_m = rel_length_cm * 1e-2f; // cm -> m
			const uint32_t flags = _buf.payload_rx_nav_relposned.flags;
			const bool heading_valid_flag = flags & (1 << 8);
			const bool rel_pos_valid = flags & (1 << 2);
			const bool carrier_solution_fixed = flags & (1 << 4);

			const bool heading_qualified = heading_valid_flag && rel_pos_valid
						       && (rel_length_m < UBX_HEADING_MAX_BASELINE_M) && carrier_solution_fixed;

			float heading_rad = NAN;
			float heading_acc_rad = NAN;

			if (heading_qualified) {
				heading_rad = relPosHeadingToYaw(_buf.payload_rx_nav_relposned.relPosHeading);
				heading_acc_rad = _buf.payload_rx_nav_relposned.accHeading * M_DEG_TO_RAD_F * 1e-5f;
			}

			_gps_position->heading = heading_rad;
			_gps_position->heading_accuracy = heading_acc_rad;

			sensor_gnss_relative_s gps_rel{};

			gps_rel.timestamp_sample = gps_absolute_time(); // TODO: adjust with delay estimate

			gps_rel.time_utc_usec = _buf.payload_rx_nav_relposned.iTOW * 1000; // TODO: convert iTOW ms GPS time of week
			gps_rel.reference_station_id = _buf.payload_rx_nav_relposned.refStationId;

			gps_rel.position[0] = (_buf.payload_rx_nav_relposned.relPosN + _buf.payload_rx_nav_relposned.relPosHPN * 1e-2f) * 1e-2f;
			gps_rel.position[1] = (_buf.payload_rx_nav_relposned.relPosE + _buf.payload_rx_nav_relposned.relPosHPE * 1e-2f) * 1e-2f;
			gps_rel.position[2] = (_buf.payload_rx_nav_relposned.relPosD + _buf.payload_rx_nav_relposned.relPosHPD * 1e-2f) * 1e-2f;

			gps_rel.position_length = rel_length_m;

			gps_rel.heading = heading_rad;
			gps_rel.heading_accuracy = heading_acc_rad;

			gps_rel.position_accuracy[0] = _buf.payload_rx_nav_relposned.accN * 1e-4f; // 0.1mm -> m
			gps_rel.position_accuracy[1] = _buf.payload_rx_nav_relposned.accE * 1e-4f; // 0.1mm -> m
			gps_rel.position_accuracy[2] = _buf.payload_rx_nav_relposned.accD * 1e-4f; // 0.1mm -> m

			gps_rel.accuracy_length = _buf.payload_rx_nav_relposned.accLength * 1e-4f; // 0.1mm -> m;

			gps_rel.gnss_fix_ok                  = flags & (1 << 0);
			gps_rel.differential_solution        = flags & (1 << 1);
			gps_rel.relative_position_valid      = flags & (1 << 2);
			gps_rel.carrier_solution_floating    = flags & (1 << 3);
			gps_rel.carrier_solution_fixed       = flags & (1 << 4);
			gps_rel.moving_base_mode             = flags & (1 << 5);
			gps_rel.reference_position_miss      = flags & (1 << 6);
			gps_rel.reference_observations_miss  = flags & (1 << 7);
			gps_rel.heading_valid                = heading_qualified;
			gps_rel.relative_position_normalized = flags & (1 << 9);

			gotRelativePositionMessage(gps_rel);

			ret = 1;
		}

		break;

	case UBX_MSG_NAV_DAHEADING:
		UBX_TRACE_RXMSG("Rx NAV-DAHEADING");
		{
			const float rel_length_m = _buf.payload_rx_nav_daheading.relPosLength * 1e-3f; // mm -> m
			const uint32_t flags = _buf.payload_rx_nav_daheading.flags;
			const bool heading_valid_flag = flags & (1 << 6); // bit 8 in NAV-RELPOSNED
			const bool rel_pos_valid = flags & (1 << 2);
			const bool carrier_solution_fixed = flags & (1 << 4);

			const bool heading_qualified = heading_valid_flag && rel_pos_valid
						       && (rel_length_m < UBX_HEADING_MAX_BASELINE_M) && carrier_solution_fixed;

			float heading_rad = NAN;
			float heading_acc_rad = NAN;

			if (heading_qualified) {
				heading_rad = relPosHeadingToYaw(_buf.payload_rx_nav_daheading.relPosHeading);
				heading_acc_rad = _buf.payload_rx_nav_daheading.accHeading * M_DEG_TO_RAD_F * 1e-5f;
			}

			_gps_position->heading = heading_rad;
			_gps_position->heading_accuracy = heading_acc_rad;

			sensor_gnss_relative_s gps_rel{};

			gps_rel.timestamp_sample = gps_absolute_time();

			// time_utc_usec is left at 0 (documented as unavailable): NAV-DAHEADING only carries
			// iTOW, which cannot be converted to UTC without the week number and leap seconds.

			gps_rel.position[0] = _buf.payload_rx_nav_daheading.relPosN * 1e-3f; // mm -> m
			gps_rel.position[1] = _buf.payload_rx_nav_daheading.relPosE * 1e-3f;
			gps_rel.position[2] = _buf.payload_rx_nav_daheading.relPosD * 1e-3f;

			gps_rel.position_length = rel_length_m;

			gps_rel.heading = heading_rad;
			gps_rel.heading_accuracy = heading_acc_rad;

			gps_rel.position_accuracy[0] = _buf.payload_rx_nav_daheading.accN * 1e-3f;
			gps_rel.position_accuracy[1] = _buf.payload_rx_nav_daheading.accE * 1e-3f;
			gps_rel.position_accuracy[2] = _buf.payload_rx_nav_daheading.accD * 1e-3f;

			gps_rel.accuracy_length = _buf.payload_rx_nav_daheading.accLength * 1e-3f;

			// NAV-DAHEADING has no reference station, moving base or normalized position flags
			gps_rel.gnss_fix_ok               = flags & (1 << 0);
			gps_rel.differential_solution     = flags & (1 << 1);
			gps_rel.relative_position_valid   = rel_pos_valid;
			gps_rel.carrier_solution_floating = flags & (1 << 3);
			gps_rel.carrier_solution_fixed    = carrier_solution_fixed;
			gps_rel.heading_valid             = heading_qualified;

			gotRelativePositionMessage(gps_rel);

			ret = 1;
		}

		break;

	case UBX_MSG_MON_VER:
		UBX_TRACE_RXMSG("Rx MON-VER");

		// This is polled only on startup, and the startup code waits for an ack
		if (_ack_state == UBX_ACK_WAITING && _ack_waiting_msg == UBX_MSG_MON_VER) {
			_ack_state = UBX_ACK_GOT_ACK;
		}

		ret = 1;
		break;

	case UBX_MSG_MON_HW:
		UBX_TRACE_RXMSG("Rx MON-HW");

		switch (_rx_payload_length) {

		case sizeof(ubx_payload_rx_mon_hw_ubx6_t):	/* u-blox 6 msg format */
			_gps_position->noise_per_ms		= _buf.payload_rx_mon_hw_ubx6.noisePerMS;
			_gps_position->automatic_gain_control   = _buf.payload_rx_mon_hw_ubx6.agcCnt;
			_gps_position->jamming_indicator	= _buf.payload_rx_mon_hw_ubx6.jamInd;

			ret = 1;
			break;

		case sizeof(ubx_payload_rx_mon_hw_ubx7_t):	/* u-blox 7+ msg format */
			_gps_position->noise_per_ms		= _buf.payload_rx_mon_hw_ubx7.noisePerMS;
			_gps_position->automatic_gain_control   = _buf.payload_rx_mon_hw_ubx7.agcCnt;
			_gps_position->jamming_indicator	= _buf.payload_rx_mon_hw_ubx7.jamInd;

			ret = 1;
			break;

		case sizeof(ubx_payload_rx_mon_hw_deprecated_t):	/* u-blox 27+ deprecated, ignore */
			ret = 0;
			break;

		default:		// unexpected payload size:
			ret = 0;	// don't handle message
			break;
		}

		break;

	case UBX_MSG_MON_RF:
		UBX_TRACE_RXMSG("Rx MON-RF");

		// TODO: only block 0 is read. F9P reports 2 blocks, X20 3, each with its own noisePerMS,
		// agcCnt and cwSuppression (jamInd). cwSuppression is the CW notch in effect per front end,
		// i.e. the per-frequency mitigation state GNSS_BANDS wants; the block covering a SEC-SIG
		// center frequency comes from rfBlockGnssBand (HPG 2.10) or blockId on older firmware.
		_gps_position->noise_per_ms		= _buf.payload_rx_mon_rf.block[0].noisePerMS;
		_gps_position->automatic_gain_control	= _buf.payload_rx_mon_rf.block[0].agcCnt;
		_gps_position->jamming_indicator	= _buf.payload_rx_mon_rf.block[0].jamInd;

		if (!_got_sec_sig) {
			_gps_position->jamming_state = _buf.payload_rx_mon_rf.block[0].flags & 0x03;
		}

		ret = 1;
		break;

	case UBX_MSG_SEC_SIG:
		UBX_TRACE_RXMSG("Rx SEC-SIG");

		{
			const uint8_t version = _buf.payload_rx_sec_sig.version;
			uint8_t flag_byte;

			if (version == 1) {
				if (_rx_payload_length < 5) {
					ret = 0;
					break;
				}

				flag_byte = _buf.payload_rx_sec_sig.jamFlags;

			} else {
				flag_byte = _buf.payload_rx_sec_sig.flags;
			}

			uint8_t jamming_state = 0;

			// TODO: bits 6..4 of the same byte are spfState (v1: spfFlags at offset 8, bits 3..1).
			// spoofing_state still comes from NAV-STATUS spoofDetState, whose F9 value 3 means
			// "multiple indications"; SEC-SIG distinguishes indicated/suspected from affirmed/
			// detected, which is the DETECTED vs AFFECTED split the MAVLink GNSS_INTEGRITY rework
			// maps to.
			if (flag_byte & 0x01) {
				const uint8_t jam_state = (flag_byte >> 1) & 0x03;

				// SEC-SIG jamState: 0 unknown, 1 none, 2 warning (jamming indicated).
				// Pre-v2 MON-RF also had 3 = critical. sensor_gps 2 is "mitigated";
				// commander only alerts on 3 (detected).
				if (jam_state >= 2) {
					jamming_state = 3;

				} else {
					jamming_state = jam_state;
				}
			}

			_gps_position->jamming_state = jamming_state;
			_got_sec_sig = true;

			// TODO: v2/v3 carry jamNumCentFreqs X4 groups after the header (bits 23..0 centFreq in
			// kHz, bit 24 jammed), one per in-use band. Not parsed: sensor_gps has nowhere to put
			// per-band state until the GNSS_BANDS message from mavlink/rfcs#30 lands, at which
			// point both the RX struct and payloadRxInit() length check need the repeated group.
		}

		ret = 1;
		break;

	case UBX_MSG_RXM_RTCM:
		UBX_TRACE_RXMSG("Rx RXM-RTCM");

		_gps_position->corrections_protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_RTCM3;
		_gps_position->corrections_crc_failed = (_buf.payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_CRCFAILED_MASK) != 0;
		_gps_position->corrections_msg_used = (_buf.payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_MSGUSED_MASK) >>
						      UBX_RX_RXM_RTCM_MSGUSED_SHIFT;

		ret = 1;
		break;

	case UBX_MSG_RXM_COR:
		UBX_TRACE_RXMSG("Rx RXM-COR");

		{
			const uint32_t status = _buf.payload_rx_rxm_cor.statusInfo;
			uint8_t protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_UNKNOWN;

			switch (status & UBX_RX_RXM_COR_PROTOCOL_MASK) {
			case 1: protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_RTCM3; break;

			case 2: protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_SPARTN; break;

			case 5: protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_HAS; break;

			case 29: protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_PMP; break;

			case 30: protocol = sensor_gps_s::CORRECTIONS_PROTOCOL_QZSS_L6; break;
			}

			_gps_position->corrections_protocol = protocol;
			_gps_position->corrections_crc_failed = ((status & UBX_RX_RXM_COR_ERRSTATUS_MASK) >> UBX_RX_RXM_COR_ERRSTATUS_SHIFT) == 2;
			_gps_position->corrections_msg_used = (status & UBX_RX_RXM_COR_MSGUSED_MASK) >> UBX_RX_RXM_COR_MSGUSED_SHIFT;
		}

		ret = 1;
		break;

	case UBX_MSG_ACK_ACK:
		UBX_TRACE_RXMSG("Rx ACK-ACK");

		if ((_ack_state == UBX_ACK_WAITING) && (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg)) {
			_ack_state = UBX_ACK_GOT_ACK;
		}

		ret = 1;
		break;

	case UBX_MSG_ACK_NAK:
		UBX_TRACE_RXMSG("Rx ACK-NAK");

		if ((_ack_state == UBX_ACK_WAITING) && (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg)) {
			_ack_state = UBX_ACK_GOT_NAK;
		}

		ret = 1;
		break;

	default:
		break;
	}

	if (ret > 0) {
		_gps_position->timestamp_time_relative = (int32_t)(_last_timestamp_time - _gps_position->timestamp);
	}

	return ret;
}

void GPSDriverUBX::requestCommsDiagnostics()
{
	const gps_abstime now = gps_absolute_time();

	if (now < _next_comms_poll) {
		return;
	}

	// A congested receiver must not be flooded with diagnostic requests.
	_next_comms_poll = now + 5000000;
	_comms_poll_deadline = sendMessage(UBX_MSG_MON_COMMS, nullptr, 0) ? now + 2000000 : 0;
}

void GPSDriverUBX::logCommsDiagnostics()
{
	if (_comms_poll_deadline == 0 || gps_absolute_time() > _comms_poll_deadline) {
		return;
	}

	const auto &status = _buf.payload_rx_mon_comms;

	if (status.version != 0 || status.nPorts > UBX_MON_COMMS_MAX_PORTS
	    || _rx_payload_length != 8 + status.nPorts * sizeof(ubx_payload_rx_mon_comms_port_t)) {
		return;
	}

	_comms_poll_deadline = 0;
	UBX_WARN("MON-COMMS after txbuf: txErrors=0x%02x ports=%u (snapshot after warning)",
		 (unsigned)status.txErrors, (unsigned)status.nPorts);

	for (unsigned i = 0; i < status.nPorts; ++i) {
		const auto &port = status.ports[i];
		[[maybe_unused]] const char *name = "unknown";

		switch (port.portId) {
		case 0x0000: name = "I2C"; break;
		case 0x0100: name = "UART1"; break;
		case 0x0201: name = "UART2"; break;
		case 0x0300: name = "USB"; break;
		case 0x0400: name = "SPI"; break;
		default: break;
		}

		UBX_WARN("MON-COMMS %s port=0x%04x txPending=%u txUsage=%u%% txPeakUsage=%u%% "
			 "rxPending=%u rxUsage=%u%% overrunErrs=%u skipped=%lu",
			 name, (unsigned)port.portId, (unsigned)port.txPending, (unsigned)port.txUsage,
			 (unsigned)port.txPeakUsage, (unsigned)port.rxPending, (unsigned)port.rxUsage,
			 (unsigned)port.overrunErrs, (unsigned long)port.skipped);
	}
}

int
GPSDriverUBX::activateRTCMOutput(bool reduce_update_rate)
{
	/* For base stations we switch to 1 Hz update rate, which is enough for RTCM output.
	 * For the survey-in, we still want 5/10 Hz, because this speeds up the process */

	UBX_DEBUG("activateRTCMOutput");

	if (_proto_ver_27_or_higher) {
		initCfgValset();

		if (reduce_update_rate) {
			cfgValset<uint16_t>(UBX_CFG_KEY_RATE_MEAS, 1000);
		}

		cfgValsetPort(RTCM_BASE_MSGOUT_I2C, 1);
		cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 0);

		if (!sendCfgValset()) {
			return -1;
		}

		if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false) < 0) {
			return -1;
		}

	} else {

		if (reduce_update_rate) {
			memset(&_buf.payload_tx_cfg_rate, 0, sizeof(_buf.payload_tx_cfg_rate));
			_buf.payload_tx_cfg_rate.measRate	= 1000;
			_buf.payload_tx_cfg_rate.navRate	= UBX_TX_CFG_RATE_NAVRATE;
			_buf.payload_tx_cfg_rate.timeRef	= UBX_TX_CFG_RATE_TIMEREF;

			if (!sendMessage(UBX_MSG_CFG_RATE, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_rate))) { return -1; }

			// according to the spec we should receive an (N)ACK here, but we don't
		}

		configureMessageRate(UBX_MSG_NAV_SVIN, 0);

		// stationary RTK reference station ARP (can be sent at lower rate)
		if (!configureMessageRate(UBX_MSG_RTCM3_1005, 5)) { return -1; }

		// GPS
		if (!configureMessageRate(UBX_MSG_RTCM3_1077, 1)) { return -1; }

		// GLONASS
		if (!configureMessageRate(UBX_MSG_RTCM3_1087, 1)) { return -1; }

		// GLONASS code-phase biases
		if (!configureMessageRate(UBX_MSG_RTCM3_1230, 1)) { return -1; }

		// Galileo
		if (!configureMessageRate(UBX_MSG_RTCM3_1097, 1)) { return -1; }

		// BeiDou
		if (!configureMessageRate(UBX_MSG_RTCM3_1127, 1)) { return -1; }
	}

	return 0;
}

void
GPSDriverUBX::decodeInit()
{
	_decode_state = UBX_DECODE_SYNC1;
	_rx_ck_a = 0;
	_rx_ck_b = 0;
	_rx_payload_length = 0;
	_rx_payload_index = 0;
}

float
GPSDriverUBX::relPosHeadingToYaw(int32_t heading) const
{
	float heading_rad = heading * M_DEG_TO_RAD_F * 1e-5f;
	heading_rad -= _heading_offset;

	// Normalize to [-pi, pi]
	if (heading_rad > M_PI_F) {
		heading_rad -= 2.f * M_PI_F;

	} else if (heading_rad < -M_PI_F) {
		heading_rad += 2.f * M_PI_F;
	}

	return heading_rad;
}

void
GPSDriverUBX::addByteToChecksum(const uint8_t b)
{
	_rx_ck_a = _rx_ck_a + b;
	_rx_ck_b = _rx_ck_b + _rx_ck_a;
}

void
GPSDriverUBX::calcChecksum(const uint8_t *buffer, const uint16_t length, ubx_checksum_t *checksum)
{
	for (uint16_t i = 0; i < length; i++) {
		checksum->ck_a = checksum->ck_a + buffer[i];
		checksum->ck_b = checksum->ck_b + checksum->ck_a;
	}
}

bool
GPSDriverUBX::configureMessageRate(const uint16_t msg, const uint8_t rate)
{
	if (_proto_ver_27_or_higher) {
		// configureMessageRate() should not be called if _proto_ver_27_or_higher is true.
		// If you see this message the calling code needs to be fixed.
		UBX_WARN("FIXME: use of deprecated msg CFG_MSG (%i %i)", msg, rate);
	}

	ubx_payload_tx_cfg_msg_t cfg_msg;	// don't use _buf (allow interleaved operation)
	memset(&cfg_msg, 0, sizeof(cfg_msg));

	cfg_msg.msg	= msg;
	cfg_msg.rate	= rate;

	return sendMessage(UBX_MSG_CFG_MSG, (uint8_t *)&cfg_msg, sizeof(cfg_msg));
}

bool
GPSDriverUBX::configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error)
{
	if (!configureMessageRate(msg, rate)) {
		return false;
	}

	return waitForAck(UBX_MSG_CFG_MSG, UBX_CONFIG_TIMEOUT, report_ack_error) >= 0;
}

bool
GPSDriverUBX::sendMessage(const uint16_t msg, const uint8_t *payload, const uint16_t length)
{
	ubx_header_t   header = {UBX_SYNC1, UBX_SYNC2, 0, 0};
	ubx_checksum_t checksum = {0, 0};

	// Populate header
	header.msg	= msg;
	header.length	= length;

	// Calculate checksum
	calcChecksum(((uint8_t *)&header) + 2, sizeof(header) - 2, &checksum); // skip 2 sync bytes

	if (payload != nullptr) {
		calcChecksum(payload, length, &checksum);
	}

	// Send message
	if (write((void *)&header, sizeof(header)) != sizeof(header)) {
		return false;
	}

	if (payload && write((void *)payload, length) != length) {
		return false;
	}

	if (write((void *)&checksum, sizeof(checksum)) != sizeof(checksum)) {
		return false;
	}

	return true;
}

uint32_t
GPSDriverUBX::fnv1_32_str(uint8_t *str, uint32_t hval)
{
	uint8_t *s = str;

	/*
	 * FNV-1 hash each octet in the buffer
	 */
	while (*s) {

		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV1_32_PRIME;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif

		/* xor the bottom with the current octet */
		hval ^= (uint32_t) * s++;
	}

	/* return our new hash value */
	return hval;
}

int
GPSDriverUBX::reset(GPSRestartType restart_type)
{
	memset(&_buf.payload_tx_cfg_rst, 0, sizeof(_buf.payload_tx_cfg_rst));
	_buf.payload_tx_cfg_rst.resetMode = UBX_TX_CFG_RST_MODE_SOFTWARE;

	switch (restart_type) {
	case GPSRestartType::Hot:
		_buf.payload_tx_cfg_rst.navBbrMask = UBX_TX_CFG_RST_BBR_MODE_HOT_START;
		break;

	case GPSRestartType::Warm:
		_buf.payload_tx_cfg_rst.navBbrMask = UBX_TX_CFG_RST_BBR_MODE_WARM_START;
		break;

	case GPSRestartType::Cold:
		_buf.payload_tx_cfg_rst.navBbrMask = UBX_TX_CFG_RST_BBR_MODE_COLD_START;
		break;

	default:
		return -2;
	}

	if (sendMessage(UBX_MSG_CFG_RST, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_rst))) {
		_configured = false;
		return 0;
	}

	return -2;
}
